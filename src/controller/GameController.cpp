#include "GameController.h"
#include "Logger.h"
#include "../ai/SimpleAI.h"
#include "../ai/CautiousAI.h"
#include "../ai/AggressiveAI.h"
#include "../ai/AdaptiveAI.h"
#include "../utils/GameStore.h"
#include <QtCore/QRandomGenerator>
#include <QtCore/QDateTime>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

GameController::GameController(GameEngine* engine, MainWindow* view)
    : m_engine(engine), m_view(view),
      m_network(nullptr), m_httpServer(nullptr), m_webSocketServer(nullptr) {

    m_network = new NetworkManager(this);
    m_aiTimer = new QTimer(this);
    m_aiTimer->setSingleShot(true);
    connect(m_aiTimer, &QTimer::timeout, this, &GameController::processAI);

    // 倒计时定时器：每秒更新一次
    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, &GameController::onCountdownTick);

    connect(m_engine, &GameEngine::turnStarted, this, &GameController::handleTurnStarted);
    connect(m_engine, &GameEngine::gameStateChanged, this, &GameController::updateView);
    connect(m_engine, &GameEngine::gameOver, this, &GameController::handleGameOver);
    connect(m_engine, &GameEngine::playerActed, this, &GameController::handlePlayerActed);
    connect(m_engine, &GameEngine::compareResult, this,
        [this](int, int loserId, const QString&, const QString&) {
            // 比牌后短暂翻开输家牌面 2 秒
            m_view->revealPlayerTemporarily(loserId, 2000);
        });
    connect(m_engine, &GameEngine::roundCompleted, this, &GameController::handleRoundCompleted);

    connect(m_view, &MainWindow::startGameClicked, this, &GameController::onStartGame);
    connect(m_view, &MainWindow::restartTableClicked, this, &GameController::onRestartTable);
    connect(m_view, &MainWindow::foldClicked, this, &GameController::onFold);
    connect(m_view, &MainWindow::callClicked, this, &GameController::onCall);
    connect(m_view, &MainWindow::raiseClicked, this, &GameController::onRaise);
    connect(m_view, &MainWindow::compareClicked, this, &GameController::onCompare);
    connect(m_view, &MainWindow::seeCardsClicked, this, &GameController::onSeeCards);
    connect(m_view, &MainWindow::playerCountChanged, this, &GameController::onPlayerCountChanged);

    // 联机信号连接
    connect(m_view, &MainWindow::createRoomClicked, this, &GameController::onCreateRoom);
    connect(m_view, &MainWindow::joinRoomClicked, this, &GameController::onJoinRoom);
    connect(m_network, &NetworkManager::dataReceivedFromServer, this, &GameController::onNetworkDataReceived);
    connect(m_network, &NetworkManager::dataReceivedFromPlayer, this, &GameController::onClientDataReceived);
    connect(m_network, &NetworkManager::errorOccurred, this, [this](const QString& err) {
        m_view->addConsumptionLog(QString("网络错误: %1").arg(err), false);
    });

    // WebSocket 手机端：连接/断开事件
    connect(this, &GameController::wsClientJoined, this, &GameController::onWSClientJoined);
    connect(this, &GameController::wsClientAction, this, &GameController::onWSClientAction);
    connect(this, &GameController::wsClientDisconnected, this, &GameController::onWSClientDisconnected);

    connect(m_network, &NetworkManager::playerDisconnected, this, [this](int clientId) {
        if (!m_tcpClientPlayerMap.contains(clientId)) return;
        replaceWithAI(m_tcpClientPlayerMap.take(clientId));
    });
    connect(m_network, &NetworkManager::disconnected, this, [this]() {
        if (!m_isRemoteClient) return;
        m_isRemoteClient = false;
        m_myPlayerId = 0;
        m_view->setLocalPlayerId(0);
        m_view->setRemoteClient(false);
        m_engine->resetPlayers();
        onPlayerCountChanged(4);
        m_view->setGameRunning(false);
        m_view->addConsumptionLog("与房主的连接已断开，已返回本地房间。", true);
    });
    connect(m_engine, &GameEngine::gameStateChanged, this, &GameController::sendStateToAll);
    connect(m_engine, &GameEngine::actionRejected, this, [this](int playerId, const QString& reason) {
        if (playerId == m_myPlayerId) m_view->addConsumptionLog(reason, true);
        const QJsonObject error{{"type", "error"}, {"message", reason}};
        for (auto it = m_wsClientPlayerMap.cbegin(); it != m_wsClientPlayerMap.cend(); ++it)
            if (it.value() == playerId && m_webSocketServer) m_webSocketServer->sendToClient(it.key(), error);
        for (auto it = m_tcpClientPlayerMap.cbegin(); it != m_tcpClientPlayerMap.cend(); ++it)
            if (it.value() == playerId) m_network->sendToPlayer(it.key(), error);
    });

    // 游戏事件 → WebSocket 广播
    connect(m_engine, &GameEngine::gameStateChanged, this, &GameController::broadcastWebSocketState);
    connect(m_engine, &GameEngine::turnStarted, this, &GameController::broadcastWebSocketState);
    connect(m_engine, &GameEngine::gameOver, this, &GameController::broadcastWebSocketState);

    // 初始化默认玩家 (1真人 + 3AI)
    m_engine->resetTable(GameStore::instance().loadStartingChips());
    onPlayerCountChanged(4);
    m_myPlayerId = 0;
    m_view->setLocalPlayerId(0);
}

GameController::~GameController() {
    m_network->disconnect(this);
    m_aiTimer->stop();
    stopCountdown();
    clearAIStrategies();
    if (m_httpServer) { m_httpServer->stop(); delete m_httpServer; }
    if (m_webSocketServer) { m_webSocketServer->stop(); delete m_webSocketServer; }
}

void GameController::clearAIStrategies() {
    qDeleteAll(m_aiStrategies);
    m_aiStrategies.clear();
}

AIStrategy* GameController::createRandomStrategy() {
    int roll = QRandomGenerator::global()->bounded(100);
    if (roll < 30) return new CautiousAI();
    if (roll < 60) return new AggressiveAI();
    if (roll < 80) return new AdaptiveAI();
    return new SimpleAI();
}

void GameController::updateView() {
    m_view->updateUI(m_engine);
}

void GameController::onStartGame() {
    if (m_isRemoteClient || m_engine->getCurrentPhase() != GameConstants::Settlement) return;
    m_roundStartingChips.clear();
    for (const auto* player : m_engine->getPlayers()) m_roundStartingChips.append(player->getChips());
    m_view->clearConsumptionLog();
    if (!m_engine->startGame()) {
        m_view->addConsumptionLog("至少需要两位筹码足够支付底注的玩家。点击“重开牌桌”可重新设置起始筹码。", true);
        return;
    }
    m_view->setGameRunning(m_engine->getCurrentPhase() != GameConstants::Settlement);
    m_view->playDealingAnimation();
}

void GameController::onRestartTable(int startingChips) {
    if (m_isRemoteClient) return;
    if (startingChips < GameConstants::MIN_STARTING_CHIPS ||
        startingChips > GameConstants::MAX_STARTING_CHIPS) {
        m_view->addConsumptionLog(QString("起始筹码需在 %1 至 %2 之间。")
            .arg(GameConstants::MIN_STARTING_CHIPS).arg(GameConstants::MAX_STARTING_CHIPS), true);
        return;
    }
    m_aiTimer->stop();
    stopCountdown();
    ++m_turnSerial;
    ++m_tableSerial;
    m_roundStartingChips.clear();
    m_view->resetForNewTable();
    m_engine->resetTable(startingChips);
    GameStore::instance().saveChipState(m_engine->getPlayers(), startingChips);
    m_view->addConsumptionLog(QString("牌桌已重开：每位玩家 %1 筹码。点击“开始游戏”发牌。")
        .arg(startingChips), true);
}

void GameController::startCountdown(int playerId) {
    if (playerId < 0) return;
    m_countdownPlayerId = playerId;
    m_countdownSeconds = COUNTDOWN_SECONDS;
    m_turnClock.start();
    m_view->setPlayerCountdown(playerId, m_countdownSeconds);
    m_countdownTimer->start();
}

void GameController::stopCountdown() {
    m_countdownTimer->stop();
    if (m_countdownPlayerId >= 0) {
        m_view->setPlayerCountdown(m_countdownPlayerId, -1);
    }
    m_countdownPlayerId = -1;
    m_countdownSeconds = 0;
}

void GameController::onCountdownTick() {
    if (!m_engine->canAct(m_countdownPlayerId)) { stopCountdown(); return; }
    m_countdownSeconds = qMax(0, COUNTDOWN_SECONDS - int(m_turnClock.elapsed() / 1000));
    if (m_countdownSeconds == 0) {
        const int playerId = m_countdownPlayerId;
        stopCountdown();
        m_engine->fold(playerId);
    } else {
        m_view->setPlayerCountdown(m_countdownPlayerId, m_countdownSeconds);
        broadcastWebSocketState();
        sendStateToAll();
    }
}

void GameController::handleTurnStarted(int playerId) {
    ++m_turnSerial;
    m_aiTimer->stop();
    stopCountdown();
    m_view->resetAllCountdowns();
    if (m_isRemoteClient || !m_engine->canAct(playerId)) return;
    const auto* player = m_engine->getPlayers()[playerId];
    if (player->getChips() < m_engine->calculateRequiredBet(playerId)) {
        m_view->addConsumptionLog(QString("%1 筹码不足，自动弃牌").arg(player->getName()), false);
        m_engine->fold(playerId);
        return;
    }
    startCountdown(playerId);
    m_view->setActionButtonsEnabled(playerId == m_myPlayerId);
    if (player->isAI()) m_aiTimer->start(1500);
}

void GameController::processAI() {
    const int id = m_engine->getCurrentTurnIndex();
    if (m_isRemoteClient || !m_engine->canAct(id)) return;
    auto* player = m_engine->getPlayers()[id];
    if (!player->isAI()) return;
    const int index = id - 1;
    if (index < 0 || index >= m_aiStrategies.size() || !m_aiStrategies[index]) {
        m_engine->fold(id);
        return;
    }
    const auto action = m_aiStrategies[index]->decide(player, m_engine);
    const int required = m_engine->calculateRequiredBet(id);
    if (action == AIStrategy::Fold || player->getChips() < required) {
        m_engine->fold(id);
        return;
    }
    const qint64 raise = qint64(required) + GameConstants::MIN_BET * (player->isSeen() ? 2 : 1);
    const int amount = action == AIStrategy::Raise && raise <= player->getChips() ? int(raise) : required;
    if (!m_engine->bet(id, amount)) m_engine->fold(id);
}

void GameController::onPlayerCountChanged(int count) {
    if (m_isRemoteClient || m_engine->getCurrentPhase() != GameConstants::Settlement) return;
    m_aiTimer->stop();
    stopCountdown();
    const int humanCount = 1 + m_wsClientPlayerMap.size() + m_tcpClientPlayerMap.size();
    count = qBound(qMax(2, humanCount), count, GameConstants::MAX_PLAYERS);
    const auto oldPlayers = m_engine->getPlayers();
    const bool initialSetup = oldPlayers.isEmpty();
    QList<QPair<QString, int>> humans;
    QList<QPair<QString, int>> bots;
    humans.append({oldPlayers.isEmpty() ? QString("房主") : oldPlayers[0]->getName(),
                   oldPlayers.isEmpty() ? m_engine->getStartingChips() : oldPlayers[0]->getChips()});
    for (auto it = m_tcpClientPlayerMap.cbegin(); it != m_tcpClientPlayerMap.cend(); ++it)
        humans.append({oldPlayers[it.value()]->getName(), oldPlayers[it.value()]->getChips()});
    for (auto it = m_wsClientPlayerMap.cbegin(); it != m_wsClientPlayerMap.cend(); ++it)
        humans.append({oldPlayers[it.value()]->getName(), oldPlayers[it.value()]->getChips()});
    for (const auto* player : oldPlayers)
        if (player->isAI()) bots.append({player->getName(), player->getChips()});

    m_engine->resetPlayers();
    clearAIStrategies();
    for (int id = 0; id < count; ++id) {
        const bool isAI = id >= humans.size();
        AIStrategy* strategy = id > 0 ? createRandomStrategy() : nullptr;
        if (id > 0) m_aiStrategies.append(strategy);
        const int botIndex = id - humans.size();
        const QString name = !isAI ? humans[id].first : botIndex < bots.size() ? bots[botIndex].first :
                             QString("AI %1 %2").arg(strategy->label()).arg(id);
        m_engine->addPlayer(name, isAI);
        const int chips = !isAI ? humans[id].second : botIndex < bots.size() ? bots[botIndex].second : m_engine->getStartingChips();
        m_engine->getPlayers()[id]->setChips(chips);
    }
    int id = 1;
    for (auto it = m_tcpClientPlayerMap.begin(); it != m_tcpClientPlayerMap.end(); ++it) {
        it.value() = id++;
        m_network->sendToPlayer(it.key(), {{"type", "welcome"}, {"id", it.value()}});
    }
    for (auto it = m_wsClientPlayerMap.begin(); it != m_wsClientPlayerMap.end(); ++it) {
        it.value() = id++;
        if (m_webSocketServer) m_webSocketServer->sendWelcome(it.key(), it.value());
    }
    m_view->reinitAIWidgets(count - 1);
    if (initialSetup) tryRestoreChips();
    updateView();
    broadcastWebSocketState();
    sendStateToAll();
}

void GameController::tryRestoreChips() {
    if (!GameStore::instance().hasChipSave()) return;

    QList<QPair<QString, int>> savedChips;
    GameStore::instance().loadChipState(savedChips);

    auto players = m_engine->getPlayers();
    if (savedChips.size() != players.size()) return; // 人数不匹配，跳过

    // 匹配名字恢复筹码（允许 AI 标签变化，按顺序匹配）
    for (int i = 0; i < players.size() && i < savedChips.size(); ++i) {
        if (players[i]->getName() == savedChips[i].first) {
            players[i]->setChips(savedChips[i].second);
        }
    }
}

void GameController::onFold() {
    if (!localCanAct()) return;
    const QJsonObject action{{"type", "action"}, {"action", "fold"}};
    if (m_isRemoteClient) m_network->sendToServer(action);
    else applyAction(m_myPlayerId, action);
}

void GameController::onCall() {
    if (!localCanAct()) return;
    const QJsonObject action{{"type", "action"}, {"action", "call"}};
    if (m_isRemoteClient) m_network->sendToServer(action);
    else applyAction(m_myPlayerId, action);
}

void GameController::onRaise(int amount) {
    if (!localCanAct()) return;
    const QJsonObject action{{"type", "action"}, {"action", "raise"}, {"amount", amount}};
    if (m_isRemoteClient) m_network->sendToServer(action);
    else applyAction(m_myPlayerId, action);
}

void GameController::onCompare(int targetId) {
    if (!localCanAct()) return;
    const QJsonObject action{{"type", "action"}, {"action", "compare"}, {"targetId", targetId}};
    if (m_isRemoteClient) m_network->sendToServer(action);
    else applyAction(m_myPlayerId, action);
}

void GameController::onSeeCards() {
    if (!localCanAct()) return;
    const QJsonObject action{{"type", "action"}, {"action", "see"}};
    if (m_isRemoteClient) m_network->sendToServer(action);
    else applyAction(m_myPlayerId, action);
}

void GameController::handlePlayerActed(int playerId, const QString& action, int amount) {
    auto player = m_engine->getPlayers()[playerId];
    QString logMsg;
    if (amount > 0) {
        QString sign = (action == "赢得奖池") ? "+" : "-";
        logMsg = QString("[%1] %2 %3%4 (余:%5)")
                            .arg(player->getName())
                            .arg(action)
                            .arg(sign)
                            .arg(amount)
                            .arg(player->getChips());
    } else {
        logMsg = QString("[%1] %2 (余:%3)")
                            .arg(player->getName())
                            .arg(action)
                            .arg(player->getChips());
    }
    
    // 如果不是 AI，则视为玩家自己，进行高亮
    m_view->addConsumptionLog(logMsg, !player->isAI());
}

void GameController::handleGameOver(int winnerId) {
    m_aiTimer->stop();
    stopCountdown();
    m_view->resetAllCountdowns();
    auto players = m_engine->getPlayers();
    m_view->playChipAnimation(winnerId, false); // 播放奖池筹码飞向赢家动画
    m_view->setActionButtonsEnabled(false);
    m_view->setGameRunning(false);
    updateView();

    // 保存筹码存档
    GameStore::instance().saveChipState(players, m_engine->getStartingChips());

    // 广播结算结果给 WebSocket 手机玩家
    if (m_webSocketServer && m_webSocketServer->clientCount() > 0) {
        QJsonObject result;
        result["type"] = "result";
        result["winnerId"] = winnerId;
        result["winnerName"] = players[winnerId]->getName();
        result["pot"] = m_engine->getCurrentPot();
        QJsonArray winners;
        QJsonObject w;
        w["id"] = winnerId;
        w["name"] = players[winnerId]->getName();
        w["chips"] = players[winnerId]->getChips();
        winners.append(w);
        result["winners"] = winners;
        for (int i = 0; i < m_wsClientIds.size(); ++i) {
            m_webSocketServer->sendToClient(m_wsClientIds[i], result);
        }
    }

    if (m_network->isHost()) {
        sendStateToAll();
    }
}

void GameController::handleRoundCompleted(int winnerId, int pot) {
    const auto players = m_engine->getPlayers();
    if (winnerId < 0 || winnerId >= players.size() || m_roundStartingChips.size() != players.size()) return;
    MatchRecord record;
    record.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    record.winner = players[winnerId]->getName();
    record.winnerType = players[winnerId]->getHand().typeName();
    record.pot = pot;
    for (const auto* player : players) {
        if (player->getHand().getCards().isEmpty()) continue;
        MatchRecord::PlayerEntry entry;
        entry.name = player->getName();
        entry.chipsBefore = m_roundStartingChips[player->getId()];
        entry.chipsAfter = player->getChips();
        entry.handType = player->getHand().typeName();
        record.players.append(entry);
        GameStore::instance().updateStats(entry.name, player->getId() == winnerId,
                                         entry.chipsAfter - entry.chipsBefore, entry.handType);
    }
    GameStore::instance().addMatchRecord(record);
}

void GameController::onCreateRoom() {
    if (m_isRemoteClient || m_engine->getCurrentPhase() != GameConstants::Settlement) return;
    if (!m_webSocketServer) {
        m_webSocketServer = new WebSocketServer(m_engine, this);
        connect(m_webSocketServer, &WebSocketServer::clientConnected, this, &GameController::onWSClientJoined);
        connect(m_webSocketServer, &WebSocketServer::clientDisconnected, this, &GameController::onWSClientDisconnected);
        connect(m_webSocketServer, &WebSocketServer::actionReceived, this, &GameController::onWSClientAction);
    }
    if (!m_webSocketServer->start(12347)) {
        m_view->addConsumptionLog("房间创建失败：WebSocket 端口 12347 被占用。", true);
        return;
    }
    if (!m_httpServer) m_httpServer = new HttpServer(this);
    if (!m_httpServer->start(8080)) {
        m_view->addConsumptionLog("房间创建失败：HTTP 端口 8080 被占用。", true);
        m_webSocketServer->stop();
        return;
    }
    m_myPlayerId = 0;
    m_view->setLocalPlayerId(0);
    m_view->setRoomCreated(true);
    if (!m_network->startServer())
        m_view->addConsumptionLog("桌面联机端口 12345 不可用，手机联机仍可使用。", false);
    m_view->addConsumptionLog("房间已就绪，同一 WiFi 下打开二维码地址即可加入。", true);
    // 所有服务启动后再展示二维码，扫码时 WebSocket 已可连接。
    m_view->showQRCode(m_httpServer->serverUrl());
}

void GameController::onJoinRoom(const QString& address) {
    if (m_engine->getCurrentPhase() != GameConstants::Settlement || m_network->isHost() ||
        (m_httpServer && m_httpServer->serverPort())) return;
    QString host = address.trimmed();
    int port = 12345;
    const int lastColon = host.lastIndexOf(':');
    if (lastColon > 0 && host.indexOf(':') == lastColon) {
        bool ok = false;
        int parsedPort = host.mid(lastColon + 1).toInt(&ok);
        if (ok) {
            port = parsedPort;
            host = host.left(lastColon);
        }
    }

    if (m_network->connectToHost(host, port)) {
        m_aiTimer->stop();
        stopCountdown();
        m_isRemoteClient = true;
        m_myPlayerId = -1;
        m_view->setLocalPlayerId(-1);
        m_view->setRemoteClient(true);
        m_view->setActionButtonsEnabled(false);
        m_view->addConsumptionLog(QString("成功连接到房主: %1:%2").arg(host).arg(port), true);
        // 发送加入请求
        QJsonObject joinMsg;
        joinMsg["type"] = "join";
        joinMsg["name"] = "玩家_" + QString::number(QRandomGenerator::global()->bounded(1000, 9999));
        m_network->sendToServer(joinMsg);
    } else {
        m_view->addConsumptionLog(QString("连接失败: %1:%2").arg(host).arg(port), false);
        m_view->addConsumptionLog("本机双开测试请用: 127.0.0.1:12345", false);
    }
}

void GameController::onNetworkDataReceived(const QJsonObject& data) {
    const QString type = data["type"].toString();
    if (type == "welcome") {
        m_myPlayerId = data["id"].toInt(-1);
        m_view->setLocalPlayerId(m_myPlayerId);
    } else if (type == "error") {
        m_view->addConsumptionLog(data["message"].toString(), true);
        if (m_myPlayerId < 0) m_network->disconnectFromHost();
    } else if (type == "sync" && m_isRemoteClient) {
        const auto array = data["players"].toArray();
        if (array.size() < 2 || array.size() > GameConstants::MAX_PLAYERS) return;
        const int tableSerial = data["tableId"].toInt();
        if (m_tableSerial != tableSerial) {
            m_tableSerial = tableSerial;
            m_view->resetForNewTable();
            m_view->addConsumptionLog("房主已重开牌桌，请等待开始游戏。", true);
        }
        if (array.size() != m_engine->getPlayers().size()) {
            m_engine->resetPlayers();
            for (const auto& value : array) m_engine->addPlayer(value.toObject()["name"].toString());
            m_view->reinitAIWidgets(array.size() - 1);
        }
        const auto players = m_engine->getPlayers();
        for (int i = 0; i < array.size(); ++i) {
            const auto obj = array[i].toObject();
            auto* player = players[i];
            player->setName(obj["name"].toString());
            player->setIsAI(obj["isAI"].toBool());
            player->setChips(obj["chips"].toInt());
            player->setSeen(obj["isSeen"].toBool());
            player->setStatus(static_cast<GameConstants::PlayerStatus>(obj["status"].toInt()));
            QList<Card> cards;
            for (const auto& value : obj["hand"].toArray()) {
                const auto card = value.toObject();
                const int suit = card["s"].toInt(-1), rank = card["r"].toInt(-1);
                if (suit < 0 || suit > 3 || rank < 2 || rank > 14) continue;
                cards.append(Card(static_cast<GameConstants::Suit>(suit), static_cast<GameConstants::Rank>(rank)));
            }
            player->setHand(Hand(cards));
        }
        const auto phase = static_cast<GameConstants::GamePhase>(data["phase"].toInt());
        m_engine->applyNetworkSnapshot(data["pot"].toInt(), data["bet"].toInt(), data["turn"].toInt(-1), phase);
        m_view->setGameRunning(phase != GameConstants::Settlement);
        m_view->resetAllCountdowns();
        if (phase == GameConstants::Betting)
            m_view->setPlayerCountdown(data["turn"].toInt(-1), data["countdown"].toInt());
    }
}

void GameController::onClientDataReceived(int clientId, const QJsonObject& data) {
    if (!m_network->isHost()) return;
    if (data["type"] == "join") {
        if (m_tcpClientPlayerMap.contains(clientId)) return;
        const int id = claimSeat(data["name"].toString());
        if (id < 0) {
            m_network->sendToPlayer(clientId, {{"type", "error"}, {"message", "本局进行中或房间已满，请等待下一局再加入。"}});
            return;
        }
        m_tcpClientPlayerMap[clientId] = id;
        m_network->sendToPlayer(clientId, {{"type", "welcome"}, {"id", id}});
        updateView();
        sendStateToAll();
        broadcastWebSocketState();
    } else if (data["type"] == "action" && m_tcpClientPlayerMap.contains(clientId)) {
        applyAction(m_tcpClientPlayerMap[clientId], data);
        sendStateToAll();
    }
}

void GameController::sendStateToAll() {
    if (!m_network->isHost()) return;
    for (auto it = m_tcpClientPlayerMap.cbegin(); it != m_tcpClientPlayerMap.cend(); ++it)
        m_network->sendToPlayer(it.key(), stateForPlayer(it.value()));
}

void GameController::onWSClientJoined(int clientId, const QString& name) {
    if (m_wsClientPlayerMap.contains(clientId)) return;
    const int id = claimSeat(name);
    if (id < 0) {
        if (m_webSocketServer) m_webSocketServer->rejectClient(clientId,
            "本局进行中或房间已满，请等待下一局再加入。");
        return;
    }
    m_wsClientIds.append(clientId);
    m_wsClientNames.append(m_engine->getPlayers()[id]->getName());
    m_wsClientPlayerMap[clientId] = id;
    if (m_webSocketServer) m_webSocketServer->sendWelcome(clientId, id);
    updateView();
    broadcastWebSocketState();
    sendStateToAll();
}

void GameController::onWSClientDisconnected(int clientId) {
    if (!m_wsClientPlayerMap.contains(clientId)) return;
    const int playerId = m_wsClientPlayerMap.take(clientId);
    const int index = m_wsClientIds.indexOf(clientId);
    if (index >= 0) {
        m_wsClientIds.removeAt(index);
        m_wsClientNames.removeAt(index);
    }
    replaceWithAI(playerId);
}

void GameController::onWSClientAction(int clientId, const QJsonObject& data) {
    if (!m_wsClientPlayerMap.contains(clientId)) return;
    const bool currentTurn = !data.contains("turnId") || data["turnId"].toInt(-1) == m_turnSerial;
    const bool accepted = currentTurn && applyAction(m_wsClientPlayerMap[clientId], data);
    if (m_webSocketServer) m_webSocketServer->sendToClient(clientId,
        {{"type", "actionResult"}, {"requestId", data["requestId"]}, {"accepted", accepted}});
    broadcastWebSocketState();
}

void GameController::broadcastWebSocketState() {
    if (!m_webSocketServer) return;
    for (auto it = m_wsClientPlayerMap.cbegin(); it != m_wsClientPlayerMap.cend(); ++it)
        m_webSocketServer->sendToClient(it.key(), stateForPlayer(it.value()));
}

bool GameController::localCanAct() const {
    return (!m_isRemoteClient || m_network->isConnected()) && m_engine->canAct(m_myPlayerId);
}

bool GameController::applyAction(int playerId, const QJsonObject& data) {
    if (!m_engine->canAct(playerId)) return false;
    const QString action = data["action"].toString();
    if (action == "fold") return m_engine->fold(playerId);
    if (action == "see") return m_engine->seeCards(playerId);
    if (action == "call") return m_engine->bet(playerId, m_engine->calculateRequiredBet(playerId));
    if (action == "raise") return m_engine->bet(playerId, data["amount"].toInt(-1));
    if (action == "compare") return m_engine->compare(playerId, data["targetId"].toInt(-1));
    return false;
}

int GameController::claimSeat(const QString& requestedName) {
    if (m_engine->getCurrentPhase() != GameConstants::Settlement) return -1;
    const auto players = m_engine->getPlayers();
    int id = -1;
    for (int i = 1; i < players.size(); ++i)
        if (players[i]->isAI()) { id = i; break; }
    if (id < 0 && players.size() >= GameConstants::MAX_PLAYERS) return -1;
    QString name = requestedName.simplified().left(24);
    if (name.isEmpty()) name = "玩家";
    const QString baseName = name;
    int suffix = 2;
    const auto nameTaken = [&players](const QString& candidate) {
        for (const auto* player : players) if (player->getName() == candidate) return true;
        return false;
    };
    while (nameTaken(name)) name = baseName + QString(" (%1)").arg(suffix++);
    if (id < 0) {
        id = players.size();
        m_engine->addPlayer(name, false);
        m_aiStrategies.append(createRandomStrategy());
        m_view->reinitAIWidgets(id);
    }
    auto* player = m_engine->getPlayers()[id];
    player->setIsAI(false);
    player->setName(name);
    player->setStatus(GameConstants::Waiting);
    player->setSeen(false);
    player->setHand(Hand());
    // 接替座位时保留筹码，重新加入不能凭空刷新筹码。
    m_view->addConsumptionLog(QString("%1 已加入座位 %2").arg(name).arg(id + 1), true);
    return id;
}

void GameController::replaceWithAI(int playerId) {
    const auto players = m_engine->getPlayers();
    if (playerId <= 0 || playerId >= players.size()) return;
    auto* player = players[playerId];
    player->setIsAI(true);
    while (m_aiStrategies.size() < playerId) m_aiStrategies.append(createRandomStrategy());
    if (!m_aiStrategies[playerId - 1]) m_aiStrategies[playerId - 1] = createRandomStrategy();
    m_view->addConsumptionLog(QString("%1 已掉线，由 AI 接管当前手牌和筹码").arg(player->getName()), true);
    // 保留名字、牌、看牌状态及筹码，让本局记账和下注倍率继续保持一致。
    if (m_engine->canAct(playerId)) m_aiTimer->start(1500);
    updateView();
    broadcastWebSocketState();
    sendStateToAll();
}

QJsonObject GameController::stateForPlayer(int playerId) const {
    QJsonObject sync{{"type", "sync"}, {"pot", m_engine->getCurrentPot()},
                     {"bet", m_engine->getCurrentBet()}, {"turn", m_engine->getCurrentTurnIndex()},
                     {"phase", int(m_engine->getCurrentPhase())}, {"countdown", m_countdownSeconds},
                     {"turnId", m_turnSerial}, {"tableId", m_tableSerial}};
    QJsonArray array;
    for (const auto* player : m_engine->getPlayers()) {
        QJsonObject obj{{"name", player->getName()}, {"chips", player->getChips()},
                        {"isAI", player->isAI()}, {"isSeen", player->isSeen()}, {"status", int(player->getStatus())}};
        const bool reveal = m_engine->getCurrentPhase() == GameConstants::Settlement ||
                            (player->getId() == playerId && player->isSeen());
        if (reveal) {
            QJsonArray hand;
            for (const auto& card : player->getHand().getCards())
                hand.append(QJsonObject{{"s", int(card.getSuit())}, {"r", int(card.getRank())}});
            obj["hand"] = hand;
        }
        array.append(obj);
    }
    sync["players"] = array;
    return sync;
}
