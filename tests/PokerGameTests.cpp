#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QInputDialog>
#include <QtCore/QTemporaryDir>
#include <QtCore/QDir>
#include <QtNetwork/QTcpSocket>
#include <QtWebSockets/QWebSocket>
#include <limits>
#include "GameController.h"
#include "GameStore.h"

using namespace GameConstants;

class PokerGameTests : public QObject {
    Q_OBJECT
private:
    static Hand hand(std::initializer_list<QPair<int, int>> values) {
        QList<Card> cards;
        for (const auto& value : values) cards.append(Card(Suit(value.first), Rank(value.second)));
        return Hand(cards);
    }
    static int totalChips(const GameEngine& engine) {
        int total = engine.getCurrentPhase() == Settlement ? 0 : engine.getCurrentPot();
        for (const auto* player : engine.getPlayers()) total += player->getChips();
        return total;
    }
    static void send(QWebSocket& socket, const QJsonObject& data) {
        socket.sendTextMessage(QJsonDocument(data).toJson(QJsonDocument::Compact));
    }
    static QJsonObject latest(const QSignalSpy& messages, const QString& type) {
        for (int i = messages.size() - 1; i >= 0; --i) {
            const auto obj = QJsonDocument::fromJson(messages[i][0].toString().toUtf8()).object();
            if (obj["type"] == type) return obj;
        }
        return {};
    }
private slots:
    void init() { GameStore::instance().clearChipSave(); }

    void handOrderingAndEmptyHands() {
        auto a = hand({{0,5},{1,3},{2,2}});
        auto b = hand({{1,5},{2,3},{3,2}});
        QCOMPARE(a.getType(), SPECIAL_235);
        QCOMPARE(Hand::compare(a, a), 0);
        QCOMPARE(Hand::compare(a, b), -Hand::compare(b, a));
        QVERIFY(Hand::compare(a, b) > 0);
        auto triple = hand({{0,14},{1,14},{2,14}});
        QVERIFY(Hand::compare(a, triple) > 0);
        QVERIFY(Hand::compare(a, hand({{0,8},{1,4},{2,2}})) < 0);
        QVERIFY(Hand::compare(hand({{0,14},{1,3},{2,2}}), hand({{0,4},{1,3},{2,2}})) < 0);
        auto permuted = hand({{2,14},{0,14},{1,14}});
        QCOMPARE(Hand::compare(triple, permuted), 0);
        QCOMPARE(Hand::compare(Hand(), Hand()), 0);
        QVERIFY(Hand::compare(Hand(), triple) < 0);
        triple.setCards({});
        QCOMPARE(triple.getType(), HighCard);
        QCOMPARE(Hand(QList<Card>{}).getType(), HighCard);
    }

    void allHandCategories() {
        Deck deck;
        QList<Card> cards;
        for (int i = 0; i < TOTAL_CARDS; ++i) cards.append(deck.draw());
        QMap<int,int> counts;
        for (int a = 0; a < cards.size(); ++a)
            for (int b = a + 1; b < cards.size(); ++b)
                for (int c = b + 1; c < cards.size(); ++c)
                    ++counts[Hand({cards[a], cards[b], cards[c]}).getType()];
        QCOMPARE(counts[Triple], 52);
        QCOMPARE(counts[FlushStraight], 48);
        QCOMPARE(counts[Flush], 1096);
        QCOMPARE(counts[Straight], 720);
        QCOMPARE(counts[Pair], 3744);
        QCOMPARE(counts[SPECIAL_235], 24);
        QCOMPARE(counts[HighCard], 16416);
    }

    void invalidActionsDoNotMutateState() {
        GameEngine engine;
        engine.addPlayer("host"); engine.addPlayer("guest");
        QVERIFY(!engine.fold(0));
        QVERIFY(!engine.seeCards(-1));
        QVERIFY(!engine.compare(0, 99));
        QVERIFY(engine.startGame());
        QVERIFY(!engine.startGame());
        QVERIFY(!engine.bet(1, 10));
        QVERIFY(!engine.bet(0, -100));
        QVERIFY(!engine.bet(0, 0));
        QVERIFY(!engine.bet(0, 9));
        QVERIFY(!engine.bet(0, 10000));
        QVERIFY(!engine.compare(0, -1));
        QVERIFY(!engine.compare(0, 0));
        QVERIFY(!engine.compare(0, 20));
        QCOMPARE(engine.getCurrentTurnIndex(), 0);
        QCOMPARE(engine.getCurrentPot(), 20);
        QCOMPARE(totalChips(engine), 2000);
        QVERIFY(engine.seeCards(0));
        QCOMPARE(engine.calculateRequiredBet(0), 20);
        QVERIFY(!engine.bet(0, 21));
        QVERIFY(!engine.bet(0, 10));
        QCOMPARE(engine.getCurrentPot(), 20);
        QCOMPARE(engine.calculateRequiredBet(99), 0);
    }

    void betsAreLabelledAndSettlementIsFinal() {
        GameEngine engine;
        engine.addPlayer("a"); engine.addPlayer("b");
        QSignalSpy acted(&engine, &GameEngine::playerActed);
        QSignalSpy ended(&engine, &GameEngine::roundCompleted);
        QVERIFY(engine.startGame());
        QVERIFY(engine.seeCards(0));
        QVERIFY(engine.bet(0, 20));
        QCOMPARE(acted.last()[1].toString(), QString("跟注"));
        QVERIFY(engine.bet(1, 20));
        QCOMPARE(acted.last()[1].toString(), QString("加注"));
        QCOMPARE(totalChips(engine), 2000);
        QVERIFY(engine.fold(0));
        QCOMPARE(ended.size(), 1);
        QCOMPARE(engine.getCurrentPhase(), Settlement);
        QCOMPARE(engine.getCurrentTurnIndex(), -1);
        QCOMPARE(totalChips(engine), 2000);
        QVERIFY(!engine.fold(1));
        QVERIFY(!engine.bet(0, 10));
        engine.nextTurn();
        QCOMPARE(ended.size(), 1);
        QCOMPARE(totalChips(engine), 2000);
    }

    void brokePlayersAndPlayerLimit() {
        GameEngine engine;
        engine.addPlayer("a"); engine.addPlayer("b"); engine.addPlayer("c");
        engine.getPlayers()[0]->setChips(0);
        QVERIFY(engine.startGame());
        QCOMPARE(engine.getCurrentTurnIndex(), 1);
        QCOMPARE(engine.getPlayers()[0]->getStatus(), Waiting);
        QCOMPARE(engine.getCurrentPot(), 20);
        QVERIFY(engine.getPlayers()[0]->getHand().getCards().isEmpty());
        engine.fold(1);
        engine.getPlayers()[1]->setChips(0);
        QVERIFY(!engine.startGame());
        engine.resetPlayers();
        QCOMPARE(engine.getCurrentPot(), 0);
        for (int i = 0; i < 20; ++i) engine.addPlayer(QString::number(i));
        QCOMPARE(engine.getPlayers().size(), MAX_PLAYERS);
        QVERIFY(engine.startGame());
        QSet<QString> unique;
        for (const auto* player : engine.getPlayers())
            for (const auto& card : player->getHand().getCards())
                unique.insert(QString("%1/%2").arg(card.getSuit()).arg(card.getRank()));
        QCOMPARE(unique.size(), MAX_PLAYERS * 3);
    }

    void comparePreservesChips() {
        GameEngine engine;
        engine.addPlayer("a"); engine.addPlayer("b"); engine.startGame();
        engine.getPlayers()[0]->setHand(hand({{0,5},{1,3},{2,2}}));
        engine.getPlayers()[1]->setHand(hand({{0,14},{1,14},{2,14}}));
        QVERIFY(engine.compare(0, 1));
        QCOMPARE(engine.getPlayers()[0]->getStatus(), Winner);
        QCOMPARE(engine.getCurrentPot(), 40);
        QCOMPARE(totalChips(engine), 2000);
        QVERIFY(!engine.compare(0, 1));
    }

    void restartTableClearsRoundWithoutAwardingPot() {
        GameEngine engine;
        engine.addPlayer("host"); engine.addPlayer("guest");
        engine.startGame(); engine.seeCards(0); engine.bet(0, 40);
        const auto players = engine.getPlayers();
        QSignalSpy ended(&engine, &GameEngine::roundCompleted);
        QVERIFY(!engine.resetTable(-1));
        QVERIFY(!engine.resetTable(MIN_STARTING_CHIPS - 1));
        QVERIFY(!engine.resetTable(MAX_STARTING_CHIPS + 1));
        QCOMPARE(engine.getCurrentPot(), 60);
        QCOMPARE(engine.getCurrentPhase(), Betting);
        QVERIFY(engine.resetTable(5000));
        QCOMPARE(engine.getPlayers(), players);
        QCOMPARE(engine.getCurrentPot(), 0);
        QCOMPARE(engine.getCurrentBet(), MIN_BET);
        QCOMPARE(engine.getCurrentTurnIndex(), -1);
        QCOMPARE(engine.getCurrentPhase(), Settlement);
        QCOMPARE(ended.size(), 0);
        for (const auto* player : players) {
            QCOMPARE(player->getChips(), 5000);
            QCOMPARE(player->getStatus(), Waiting);
            QVERIFY(!player->isSeen());
            QVERIFY(player->getHand().getCards().isEmpty());
        }
        QVERIFY(engine.startGame());
        QCOMPARE(engine.getCurrentPot(), 20);
        QCOMPARE(players[0]->getChips(), 4990);
        engine.fold(0);
        QVERIFY(engine.startGame());
        QCOMPARE(players[0]->getChips(), 4980); // 普通下一局不会补充筹码。
    }

    void restartDialogCanCancelOrSetStartingChips() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        controller.onWSClientJoined(1, "guest");
        controller.onStartGame();
        auto* button = view.findChild<QPushButton*>("restartTableButton");
        QVERIFY(button && button->isEnabled());
        QSignalSpy restarted(&view, &MainWindow::restartTableClicked);
        QTimer::singleShot(0, &view, [&view]() {
            if (auto* dialog = view.findChild<QInputDialog*>("restartTableDialog")) dialog->reject();
        });
        button->click();
        QCOMPARE(restarted.size(), 0);
        QCOMPARE(engine.getCurrentPhase(), Betting);
        QCOMPARE(engine.getCurrentPot(), 40);
        controller.onRestartTable(0);
        QCOMPARE(engine.getCurrentPhase(), Betting);
        bool timerActive = false;
        for (const auto* timer : controller.findChildren<QTimer*>()) timerActive |= timer->isActive();
        QVERIFY(timerActive);

        const int historySize = GameStore::instance().getMatchHistory(50).size();
        QTimer::singleShot(0, &view, [&view]() {
            if (auto* dialog = view.findChild<QInputDialog*>("restartTableDialog")) {
                dialog->setIntValue(5000);
                dialog->accept();
            }
        });
        button->click();
        QCOMPARE(restarted.size(), 1);
        QCOMPARE(engine.getStartingChips(), 5000);
        QCOMPARE(engine.getCurrentPhase(), Settlement);
        QCOMPARE(engine.getPlayers()[1]->getName(), QString("guest"));
        QVERIFY(!engine.getPlayers()[1]->isAI());
        for (const auto* timer : controller.findChildren<QTimer*>()) QVERIFY(!timer->isActive());
        QCOMPARE(GameStore::instance().getMatchHistory(50).size(), historySize);
        QList<QPair<QString, int>> saved;
        GameStore::instance().loadChipState(saved);
        QCOMPARE(saved.size(), 4);
        for (const auto& player : saved) QCOMPARE(player.second, 5000);
        QCOMPARE(GameStore::instance().loadStartingChips(), 5000);
        controller.onPlayerCountChanged(5);
        QCOMPARE(engine.getPlayers().last()->getChips(), 5000);
        GameEngine restoredEngine; MainWindow restoredView; GameController restored(&restoredEngine, &restoredView);
        QCOMPARE(restoredEngine.getStartingChips(), 5000);
        for (const auto* player : restoredEngine.getPlayers()) QCOMPARE(player->getChips(), 5000);
    }

    void distinctSeatsAndDisconnectKeepsHand() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        QCOMPARE(engine.getPlayers().size(), 4);
        controller.onWSClientJoined(10, "Alice");
        controller.onWSClientJoined(11, "Bob");
        QCOMPARE(engine.getPlayers()[1]->getName(), QString("Alice"));
        QCOMPARE(engine.getPlayers()[2]->getName(), QString("Bob"));
        QVERIFY(!engine.getPlayers()[1]->isAI());
        QVERIFY(!engine.getPlayers()[2]->isAI());
        engine.getPlayers()[1]->setChips(777);
        controller.onPlayerCountChanged(2);
        QCOMPARE(engine.getPlayers().size(), 3);
        QCOMPARE(engine.getPlayers()[1]->getChips(), 777);
        QCOMPARE(engine.getPlayers()[2]->getName(), QString("Bob"));
        controller.onStartGame();
        controller.onFold();
        QCOMPARE(engine.getCurrentTurnIndex(), 1);
        controller.onWSClientAction(10, {{"action", "see"}});
        const auto before = engine.getPlayers()[1]->getHand();
        const int chips = engine.getPlayers()[1]->getChips();
        controller.onWSClientDisconnected(10);
        QCOMPARE(Hand::compare(before, engine.getPlayers()[1]->getHand()), 0);
        QCOMPARE(engine.getPlayers()[1]->getHand().getCards().size(), 3);
        QCOMPARE(engine.getPlayers()[1]->getChips(), chips);
        QVERIFY(engine.getPlayers()[1]->isSeen());
        QVERIFY(engine.getPlayers()[1]->isAI());
        controller.onWSClientJoined(12, "Late");
        QCOMPARE(engine.getPlayers()[1]->getName(), QString("Alice"));
        controller.processAI();
        QVERIFY(engine.getCurrentTurnIndex() != 1);
        // A stale AI timer must never act for the next human.
        const int bobChips = engine.getPlayers()[2]->getChips();
        controller.processAI();
        QCOMPARE(engine.getPlayers()[2]->getChips(), bobChips);
    }

    void seeingAndRejectedActionsKeepTimeout() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        controller.onPlayerCountChanged(3);
        controller.onWSClientJoined(1, "guest");
        controller.onWSClientJoined(2, "guest2");
        controller.onStartGame(); controller.onFold();
        QCOMPARE(engine.getCurrentTurnIndex(), 1);
        controller.onWSClientAction(1, {{"action", "see"}});
        controller.onWSClientAction(1, {{"action", "raise"}, {"amount", -1}});
        controller.onWSClientAction(1, {{"action", "compare"}, {"targetId", -1}});
        controller.onWSClientAction(1, {{"action", "unknown"}});
        bool clockRunning = false;
        for (const auto* timer : controller.findChildren<QTimer*>())
            if (timer->interval() == 1000 && timer->isActive()) clockRunning = true;
        QVERIFY(clockRunning);
        QTRY_COMPARE_WITH_TIMEOUT(engine.getCurrentPhase(), Settlement, 17000);
        QCOMPARE(engine.getPlayers()[1]->getStatus(), Folded);
        for (const auto* timer : controller.findChildren<QTimer*>()) QVERIFY(!timer->isActive());
    }

    void statisticsUseActualNetChange() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        controller.onPlayerCountChanged(2);
        controller.onWSClientJoined(1, "stats-guest");
        const int before0 = engine.getPlayers()[0]->getChips();
        const int before1 = engine.getPlayers()[1]->getChips();
        controller.onStartGame(); controller.onFold();
        const auto history = GameStore::instance().getMatchHistory(1);
        QCOMPARE(history.size(), 1);
        QCOMPARE(history.first().players[0].chipsBefore, before0);
        QCOMPARE(history.first().players[1].chipsBefore, before1);
        QCOMPARE(history.first().players[0].chipsAfter - before0, -10);
        QCOMPARE(history.first().players[1].chipsAfter - before1, 10);
        GameStore::instance().updateStats("best-hand-test", true, 10, "豹子");
        GameStore::instance().updateStats("best-hand-test", false, -10, "对子");
        QCOMPARE(GameStore::instance().getStats("best-hand-test").bestHand, QString("豹子"));
    }

    void desktopLayoutAndLocalSeatMapping() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        controller.onPlayerCountChanged(6);
        view.show(); view.resize(1000, 800);
        QTest::qWait(100);
        QVERIFY2(view.width() <= 1000, qPrintable(QString("Unexpected minimum width: %1").arg(view.width())));
        QVERIFY(view.findChild<QScrollArea*>());
        view.setLocalPlayerId(2);
        view.updateUI(&engine);
        view.setPlayerCountdown(0, 8);
        view.playChipAnimation(0, true);
        view.playChipAnimation(2, true);
        view.revealPlayerTemporarily(0, 10);
        QTest::qWait(20);
        const QString path = qEnvironmentVariable("POKER_SCREENSHOT_DIR");
        if (!path.isEmpty()) {
            QDir().mkpath(path);
            view.setLocalPlayerId(0);
            view.resize(1200, 820);
            controller.onStartGame();
            QTest::qWait(1100);
            QVERIFY(view.grab().save(path + "/desktop.png"));
        }
    }

    void newRoundHidesPreviousTemporaryReveal() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        controller.onStartGame();
        view.revealPlayerTemporarily(0, 5000);
        while (engine.getCurrentPhase() == Betting) engine.fold(engine.getCurrentTurnIndex());
        controller.onStartGame();
        QVERIFY(!engine.getPlayers()[0]->isSeen());
        for (auto* widget : view.findChildren<PlayerWidget*>())
            for (auto* card : widget->findChildren<CardWidget*>()) QVERIFY(card->isFaceDown());
    }

    void httpHandlesFragmentedRequestsAndRestart() {
        HttpServer server;
        QVERIFY(server.start(0));
        const int port = server.serverPort();
        QVERIFY(port > 0);
        QVERIFY(server.start(0));
        QCOMPARE(server.serverPort(), port);
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, port);
        QTRY_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        socket.write("GET /index.html?test=1 HTTP/1.1\r\nHost: local");
        QTest::qWait(30);
        QCOMPARE(socket.bytesAvailable(), 0);
        socket.write("host\r\n\r\n");
        QTRY_VERIFY(socket.bytesAvailable() > 0);
        const auto response = socket.readAll();
        QVERIFY(response.startsWith("HTTP/1.1 200 OK"));
        QVERIFY(response.contains("/game.js"));
        server.stop();
        QCOMPARE(server.serverPort(), 0);
        QVERIFY(server.start(0));
    }

    void websocketDuplicateJoinAndHiddenCards() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        WebSocketServer server(&engine);
        connect(&server, &WebSocketServer::clientConnected, &controller, &GameController::onWSClientJoined);
        connect(&server, &WebSocketServer::clientDisconnected, &controller, &GameController::onWSClientDisconnected);
        QSignalSpy joined(&server, &WebSocketServer::clientConnected);
        QVERIFY(server.start(0));
        const int port = server.serverPort();
        QVERIFY(server.start(0)); QCOMPARE(server.serverPort(), port);
        QWebSocket alice, bob;
        alice.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
        bob.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
        QTRY_COMPARE(alice.state(), QAbstractSocket::ConnectedState);
        QTRY_COMPARE(bob.state(), QAbstractSocket::ConnectedState);
        send(alice, {{"type", "join"}, {"name", "Alice"}});
        QTRY_COMPARE(joined.size(), 1);
        send(alice, {{"type", "join"}, {"name", "Duplicate"}});
        send(bob, {{"type", "join"}, {"name", "Bob"}});
        QTRY_COMPARE(joined.size(), 2);
        QCOMPARE(server.clientCount(), 2);
        QCOMPARE(engine.getPlayers()[1]->getName(), QString("Alice"));
        QCOMPARE(engine.getPlayers()[2]->getName(), QString("Bob"));
        controller.onStartGame();
        alice.close();
        QTRY_VERIFY(engine.getPlayers()[1]->isAI());
        QCOMPARE(engine.getPlayers()[1]->getHand().getCards().size(), 3);
        QWebSocket late;
        QSignalSpy messages(&late, &QWebSocket::textMessageReceived);
        late.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
        QTRY_COMPARE(late.state(), QAbstractSocket::ConnectedState);
        send(late, {{"type", "join"}, {"name", "Late"}});
        QTRY_VERIFY(!latest(messages, "error").isEmpty());
        QCOMPARE(joined.size(), 2);
    }

    void tcpClientIdsSurviveDisconnect() {
        NetworkManager server;
        QVERIFY(server.startServer(0));
        QSignalSpy joined(&server, &NetworkManager::playerConnected);
        QSignalSpy received(&server, &NetworkManager::dataReceivedFromPlayer);
        QSignalSpy disconnected(&server, &NetworkManager::playerDisconnected);
        QTcpSocket a, b, c;
        a.connectToHost(QHostAddress::LocalHost, server.listenPort());
        QTRY_COMPARE(joined.size(), 1);
        b.connectToHost(QHostAddress::LocalHost, server.listenPort());
        QTRY_COMPARE(joined.size(), 2);
        const int bId = joined[1][0].toInt();
        a.disconnectFromHost();
        QTRY_COMPARE(disconnected.size(), 1);
        b.write("{\"type\":\"action\",\"action\":\"see\"}\n");
        QTRY_COMPARE(received.size(), 1);
        QCOMPARE(received[0][0].toInt(), bId);
        c.connectToHost(QHostAddress::LocalHost, server.listenPort());
        QTRY_COMPARE(joined.size(), 3);
        QVERIFY(joined[2][0].toInt() != bId);
        server.sendToPlayer(bId, {{"type", "test"}});
        QTRY_VERIFY(b.bytesAvailable() > 0);
        QVERIFY(b.readAll().contains("test"));
    }

    void roomRoundTripProtectsCardsAndState() {
        GameEngine engine; MainWindow view; GameController controller(&engine, &view);
        // Close only the QR dialog; services must already be live before it appears.
        QTimer::singleShot(0, &view, [] {
            for (auto* widget : QApplication::topLevelWidgets())
                if (auto* box = qobject_cast<QMessageBox*>(widget)) box->accept();
        });
        controller.onCreateRoom();
        auto* server = controller.findChild<WebSocketServer*>();
        QVERIFY(server && server->serverPort() == 12347);
        QWebSocket a, b;
        QSignalSpy messagesA(&a, &QWebSocket::textMessageReceived);
        QSignalSpy messagesB(&b, &QWebSocket::textMessageReceived);
        a.open(QUrl("ws://127.0.0.1:12347"));
        b.open(QUrl("ws://127.0.0.1:12347"));
        QTRY_COMPARE(a.state(), QAbstractSocket::ConnectedState);
        QTRY_COMPARE(b.state(), QAbstractSocket::ConnectedState);
        send(a, {{"type", "join"}, {"name", "A"}});
        QTRY_VERIFY(!latest(messagesA, "welcome").isEmpty());
        send(b, {{"type", "join"}, {"name", "B"}});
        QTRY_VERIFY(!latest(messagesB, "welcome").isEmpty());
        QTimer::singleShot(0, &view, [] {
            for (auto* widget : QApplication::topLevelWidgets())
                if (auto* box = qobject_cast<QMessageBox*>(widget)) box->accept();
        });
        controller.onCreateRoom();
        QCOMPARE(server->clientCount(), 2);
        QCOMPARE(engine.getPlayers()[1]->getName(), QString("A"));
        controller.onPlayerCountChanged(3);
        controller.onStartGame(); controller.onFold();
        QTRY_COMPARE(latest(messagesA, "sync")["turn"].toInt(), 1);
        auto sync = latest(messagesA, "sync");
        QVERIFY(!sync["players"].toArray()[1].toObject().contains("hand"));
        send(a, {{"type", "action"}, {"action", "see"}, {"requestId", 1}, {"turnId", sync["turnId"]}});
        QTRY_VERIFY(latest(messagesA, "sync")["players"].toArray()[1].toObject()["isSeen"].toBool());
        QCOMPARE(latest(messagesA, "sync")["players"].toArray()[1].toObject()["hand"].toArray().size(), 3);
        QVERIFY(!latest(messagesB, "sync")["players"].toArray()[1].toObject().contains("hand"));
        const int chips = engine.getPlayers()[1]->getChips();
        send(a, {{"type", "action"}, {"action", "raise"}, {"amount", 20}, {"requestId", 2}, {"turnId", -1}});
        QTRY_COMPARE(latest(messagesA, "actionResult")["requestId"].toInt(), 2);
        QVERIFY(!latest(messagesA, "actionResult")["accepted"].toBool());
        QCOMPARE(engine.getPlayers()[1]->getChips(), chips);
        send(a, {{"type", "action"}, {"action", "fold"}, {"requestId", 3}, {"turnId", sync["turnId"]}});
        QTRY_COMPARE(engine.getCurrentPhase(), Settlement);
        QTRY_COMPARE(latest(messagesA, "sync")["phase"].toInt(), int(Settlement));
        for (const auto& player : latest(messagesA, "sync")["players"].toArray())
            QCOMPARE(player.toObject()["hand"].toArray().size(), 3);
        QTRY_VERIFY(!latest(messagesB, "result").isEmpty());
        const int tableId = latest(messagesA, "sync")["tableId"].toInt();
        controller.onRestartTable(3000);
        QTRY_COMPARE(latest(messagesA, "sync")["tableId"].toInt(), tableId + 1);
        QTRY_COMPARE(latest(messagesB, "sync")["tableId"].toInt(), tableId + 1);
        QCOMPARE(server->clientCount(), 2);
        QCOMPARE(engine.getPlayers()[1]->getName(), QString("A"));
        for (const auto& value : latest(messagesA, "sync")["players"].toArray()) {
            const auto player = value.toObject();
            QCOMPARE(player["chips"].toInt(), 3000);
            QCOMPARE(player["status"].toInt(), int(Waiting));
            QVERIFY(player["hand"].toArray().isEmpty());
        }
        controller.onStartGame(); controller.onFold();
        QTRY_COMPARE(latest(messagesA, "sync")["turn"].toInt(), 1);
        send(a, {{"type", "action"}, {"action", "fold"}, {"requestId", 4}, {"turnId", sync["turnId"]}});
        QTRY_COMPARE(latest(messagesA, "actionResult")["requestId"].toInt(), 4);
        QVERIFY(!latest(messagesA, "actionResult")["accepted"].toBool());
        QCOMPARE(engine.getCurrentTurnIndex(), 1);
    }
    void desktopAndPhoneShareRoom() {
        GameEngine hostEngine; MainWindow hostView; GameController host(&hostEngine, &hostView);
        QTimer::singleShot(0, &hostView, [] {
            for (auto* widget : QApplication::topLevelWidgets())
                if (auto* box = qobject_cast<QMessageBox*>(widget)) box->accept();
        });
        host.onCreateRoom();
        GameEngine guestEngine; MainWindow guestView; GameController guest(&guestEngine, &guestView);
        guest.onJoinRoom("127.0.0.1:12345");
        QTRY_VERIFY(!hostEngine.getPlayers()[1]->isAI());
        QWebSocket phone;
        QSignalSpy messages(&phone, &QWebSocket::textMessageReceived);
        phone.open(QUrl("ws://127.0.0.1:12347"));
        QTRY_COMPARE(phone.state(), QAbstractSocket::ConnectedState);
        send(phone, {{"type", "join"}, {"name", "phone"}});
        QTRY_COMPARE(latest(messages, "welcome")["id"].toInt(), 2);
        host.onPlayerCountChanged(3);
        QTRY_COMPARE(guestEngine.getPlayers().size(), 3);
        auto* restartButton = guestView.findChild<QPushButton*>("restartTableButton");
        QVERIFY(restartButton && !restartButton->isEnabled());
        guest.onRestartTable(10000);
        QCOMPARE(hostEngine.getPlayers()[1]->getChips(), 1000);
        QCOMPARE(guestEngine.getPlayers()[1]->getChips(), 1000);
        guest.onStartGame();
        QCOMPARE(hostEngine.getCurrentPhase(), Settlement);
        host.onStartGame(); host.onFold();
        QTRY_COMPARE(guestEngine.getCurrentTurnIndex(), 1);
        QVERIFY(guestEngine.getPlayers()[1]->getHand().getCards().isEmpty());
        guest.onSeeCards();
        QTRY_COMPARE(guestEngine.getPlayers()[1]->getHand().getCards().size(), 3);
        QVERIFY(guestEngine.getPlayers()[0]->getHand().getCards().isEmpty());
        QVERIFY(guestEngine.getPlayers()[2]->getHand().getCards().isEmpty());
        guest.onCall();
        QTRY_COMPARE(hostEngine.getCurrentTurnIndex(), 2);
        QCOMPARE(hostEngine.getPlayers()[1]->getChips(), 970);
        send(phone, {{"type", "action"}, {"action", "fold"}});
        QTRY_COMPARE(guestEngine.getCurrentPhase(), Settlement);
        QCOMPARE(guestEngine.getPlayers()[1]->getStatus(), Winner);
        for (const auto* player : guestEngine.getPlayers()) QCOMPARE(player->getHand().getCards().size(), 3);
        host.onStartGame();
        QTRY_COMPARE(guestEngine.getCurrentPhase(), Betting);
        host.onRestartTable(7500);
        QTRY_COMPARE(guestEngine.getCurrentPhase(), Settlement);
        QTRY_COMPARE(guestEngine.getPlayers()[1]->getChips(), 7500);
        QTRY_COMPARE(latest(messages, "sync")["players"].toArray()[2].toObject()["chips"].toInt(), 7500);
        QCOMPARE(guestEngine.getCurrentPot(), 0);
        for (const auto* player : guestEngine.getPlayers()) QVERIFY(player->getHand().getCards().isEmpty());
        host.findChild<NetworkManager*>()->stopServer();
        QTRY_COMPARE(guestEngine.getPlayers().size(), 4);
        QCOMPARE(guestEngine.getCurrentPhase(), Settlement);
    }

};

int main(int argc, char** argv) {
    QTemporaryDir data;
    qputenv("XDG_DATA_HOME", data.path().toUtf8());
    QApplication app(argc, argv);
    app.setApplicationName("PokerGameTests");
    PokerGameTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "PokerGameTests.moc"
