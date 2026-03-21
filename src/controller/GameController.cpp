#include "GameController.h"
#include "Logger.h"
#include <QtCore/QRandomGenerator>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

GameController::GameController(GameEngine* engine, MainWindow* view)
    : m_engine(engine), m_view(view) {
    
    m_network = new NetworkManager(this);
    m_aiTimer = new QTimer(this);
    m_aiTimer->setSingleShot(true);
    connect(m_aiTimer, &QTimer::timeout, this, &GameController::processAI);

    connect(m_engine, &GameEngine::turnStarted, this, &GameController::handleTurnStarted);
    connect(m_engine, &GameEngine::gameStateChanged, this, &GameController::updateView);
    connect(m_engine, &GameEngine::gameOver, this, &GameController::handleGameOver);
    connect(m_engine, &GameEngine::phaseChanged, this, &GameController::handlePhaseChanged);
    connect(m_engine, &GameEngine::playerActed, this, &GameController::handlePlayerActed);

    connect(m_view, &MainWindow::startGameClicked, this, &GameController::onStartGame);
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
    connect(m_network, &NetworkManager::errorOccurred, [this](const QString& err) {
        m_view->addConsumptionLog(QString("网络错误: %1").arg(err), false);
    });

    // 初始化默认玩家 (1真人 + 3AI)
    onPlayerCountChanged(3);
    m_view->setLocalPlayerId(0);
}

void GameController::updateView() {
    m_view->updateUI(m_engine);
}

void GameController::onStartGame() {
    if (m_network->isConnected() && !m_network->isHost()) {
        return; // 客户端不能点开始，只有房主能开始
    }
    
    m_view->setGameRunning(true);
    m_view->clearConsumptionLog(); // 每次开始游戏清空流水记录
    m_view->playDealingAnimation(); // 播放发牌动画
    m_engine->startGame();
    
    if (m_network->isHost()) {
        sendStateToAll();
    }
}

void GameController::handleTurnStarted(int playerId) {
    auto players = m_engine->getPlayers();
    auto currentPlayer = players[playerId];
    
    Logger::instance().log(QString("轮到玩家: %1").arg(currentPlayer->getName()));

    if (currentPlayer->isAI()) {
        m_view->setActionButtonsEnabled(false);
        if (!(m_network->isConnected() && !m_network->isHost())) {
            m_aiTimer->start(1500); // AI 思考 1.5 秒
        }
    } else {
        m_view->setActionButtonsEnabled(playerId == m_myPlayerId);
    }
}

void GameController::handleTurnEnded(int playerId) {
    // 预留接口，处理回合结束逻辑
}

void GameController::processAI() {
    int currentId = m_engine->getCurrentTurnIndex();
    auto player = m_engine->getPlayers()[currentId];
    
    auto action = m_ai.decide(player, m_engine);
    
    switch (action) {
        case AIStrategy::Fold:
            m_engine->fold(currentId);
            break;
        case AIStrategy::Call:
            m_engine->bet(currentId, m_engine->calculateRequiredBet(currentId));
            break;
        case AIStrategy::Raise: {
            // 简单的加注逻辑：在当前暗牌底注基础上增加一个最小步长
            int darkBet = m_engine->getCurrentBet() + GameConstants::MIN_BET;
            int totalBet = player->isSeen() ? (2 * darkBet) : darkBet;
            m_engine->bet(currentId, totalBet);
            break;
        }
    }
}

void GameController::onPlayerCountChanged(int count) {
    if (m_engine->getCurrentPhase() != GameConstants::Settlement) {
        return;
    }
    m_engine->resetPlayers();
    m_engine->addPlayer("张三 (我)", false);
    for (int i = 0; i < count; ++i) {
        m_engine->addPlayer(QString("AI 玩家 %1").arg(i + 1), true);
    }
    
    m_view->reinitAIWidgets(count);
    updateView();
}

void GameController::onFold() {
    if (m_network->isConnected() && !m_network->isHost()) {
        QJsonObject action;
        action["type"] = "action";
        action["action"] = "fold";
        m_network->sendToServer(action);
        return;
    }
    m_engine->fold(m_engine->getCurrentTurnIndex());
    if (m_network->isHost()) sendStateToAll();
}

void GameController::onCall() {
    int currentId = m_engine->getCurrentTurnIndex();
    if (m_network->isConnected() && !m_network->isHost()) {
        QJsonObject action;
        action["type"] = "action";
        action["action"] = "call";
        m_network->sendToServer(action);
        return;
    }
    int amount = m_engine->calculateRequiredBet(currentId);
    m_view->playChipAnimation(currentId, true); // 播放筹码飞向奖池动画
    m_engine->bet(currentId, amount);
    if (m_network->isHost()) sendStateToAll();
}

void GameController::onRaise(int amount) {
    int currentId = m_engine->getCurrentTurnIndex();
    if (m_network->isConnected() && !m_network->isHost()) {
        QJsonObject action;
        action["type"] = "action";
        action["action"] = "raise";
        action["amount"] = amount;
        m_network->sendToServer(action);
        return;
    }
    m_view->playChipAnimation(currentId, true); // 播放筹码飞向奖池动画
    m_engine->bet(currentId, amount);
    if (m_network->isHost()) sendStateToAll();
}

void GameController::onCompare(int targetId) {
    if (m_network->isConnected() && !m_network->isHost()) {
        QJsonObject action;
        action["type"] = "action";
        action["action"] = "compare";
        action["targetId"] = targetId;
        m_network->sendToServer(action);
        return;
    }
    if (targetId >= 0 && targetId < m_engine->getPlayers().size() && targetId != m_engine->getCurrentTurnIndex()) {
        m_engine->compare(m_engine->getCurrentTurnIndex(), targetId);
        if (m_network->isHost()) sendStateToAll();
    }
}

void GameController::onSeeCards() {
    if (m_network->isConnected() && !m_network->isHost()) {
        QJsonObject action;
        action["type"] = "action";
        action["action"] = "see";
        m_network->sendToServer(action);
        return;
    }
    m_engine->seeCards(m_engine->getCurrentTurnIndex());
    if (m_network->isHost()) sendStateToAll();
}

void GameController::handlePhaseChanged(GameConstants::GamePhase phase) {
    // 界面底部日志已移除，不再处理阶段日志
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
    auto players = m_engine->getPlayers();
    m_view->playChipAnimation(winnerId, false); // 播放奖池筹码飞向赢家动画
    m_view->setActionButtonsEnabled(false);
    m_view->setGameRunning(false);
    updateView();

    if (m_network->isHost()) {
        sendStateToAll();
    }
}

void GameController::onCreateRoom() {
    if (m_network->startServer()) {
        m_myPlayerId = 0;
        m_view->setLocalPlayerId(0);
        m_joinClientIds.clear();
        m_joinClientNames.clear();
        m_hostName = m_engine->getPlayers().isEmpty() ? QString("房主") : m_engine->getPlayers().first()->getName();
        m_view->addConsumptionLog("房间创建成功，等待玩家加入...", true);
        m_view->reinitAIWidgets(m_engine->getPlayers().size() - 1);

        const auto joinAddresses = m_network->getJoinAddresses();
        if (!joinAddresses.isEmpty()) {
            const QString primary = joinAddresses.first();
            QGuiApplication::clipboard()->setText(primary);
            m_view->addConsumptionLog(QString("加入地址(已复制): %1").arg(primary), true);
            if (joinAddresses.size() > 1) {
                QStringList more = joinAddresses;
                more.removeFirst();
                m_view->addConsumptionLog(QString("其他可用地址: %1").arg(more.join(", ")), false);
            }
        } else {
            m_view->addConsumptionLog("未识别到可用地址，请在 Tailscale 查看房主 IP", false);
        }
    } else {
        m_view->addConsumptionLog("房间创建失败，端口可能被占用", false);
    }
}

void GameController::onJoinRoom(const QString& address) {
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
    QString type = data["type"].toString();
    
    if (type == "welcome") {
        m_myPlayerId = data["id"].toInt();
        m_view->setLocalPlayerId(m_myPlayerId);
        m_view->addConsumptionLog(QString("成功加入，你是玩家 %1").arg(m_myPlayerId + 1), true);
    } else if (type == "sync") {
        const int pot = data["pot"].toInt();
        const int bet = data["bet"].toInt();
        const int turn = data["turn"].toInt();
        const auto phase = static_cast<GameConstants::GamePhase>(data["phase"].toInt());
        
        QJsonArray playersArray = data["players"].toArray();
        if (playersArray.size() > 0 && playersArray.size() != m_engine->getPlayers().size()) {
             // 如果人数变了，重置本地显示
             m_engine->resetPlayers();
             for (int i = 0; i < playersArray.size(); ++i) {
                 QJsonObject pObj = playersArray[i].toObject();
                 m_engine->addPlayer(pObj["name"].toString(), pObj["isAI"].toBool());
             }
             m_view->reinitAIWidgets(playersArray.size() - 1);
        }

        m_engine->applyNetworkSnapshot(pot, bet, turn, phase);

        auto players = m_engine->getPlayers();
        for (int i = 0; i < playersArray.size(); ++i) {
            QJsonObject pObj = playersArray[i].toObject();
            auto p = players[i];
            // 更新筹码、状态、是否看牌
            // 需要在 Player 类中增加对应的 setter (这里暂用已有逻辑)
            if (p->getChips() != pObj["chips"].toInt()) {
                p->removeChips(p->getChips() - pObj["chips"].toInt());
            }
            p->setSeen(pObj["isSeen"].toBool());
            p->setStatus(static_cast<GameConstants::PlayerStatus>(pObj["status"].toInt()));
            
            // 如果是自己的牌，同步手牌
            if (i == m_myPlayerId && pObj.contains("hand")) {
                QJsonArray cardsArr = pObj["hand"].toArray();
                QList<Card> cards;
                for (auto cVal : cardsArr) {
                    QJsonObject cObj = cVal.toObject();
                    cards.append(Card(static_cast<GameConstants::Suit>(cObj["s"].toInt()), 
                                    static_cast<GameConstants::Rank>(cObj["r"].toInt())));
                }
                p->setHand(Hand(cards));
            }
        }
        
        m_view->setActionButtonsEnabled((turn == m_myPlayerId) && (phase == GameConstants::Betting));
        updateView();
    }
}

void GameController::onClientDataReceived(int clientId, const QJsonObject& data) {
    QString type = data["type"].toString();
    int senderPlayerId = clientId + 1;

    if (type == "join") {
        QString name = data["name"].toString();
        if (!m_joinClientIds.contains(clientId)) {
            m_joinClientIds.append(clientId);
            m_joinClientNames.append(name);
        }

        bool hasAI = false;
        for (auto p : m_engine->getPlayers()) {
            if (p->isAI()) {
                hasAI = true;
                break;
            }
        }

        if (hasAI && m_engine->getCurrentPhase() == GameConstants::Settlement) {
            m_engine->resetPlayers();
            m_engine->addPlayer(m_hostName, false);
            for (const auto& n : m_joinClientNames) {
                m_engine->addPlayer(n, false);
            }
        } else if (!hasAI) {
            int expectedPlayerId = m_joinClientIds.indexOf(clientId) + 1;
            if (expectedPlayerId == m_engine->getPlayers().size()) {
                m_engine->addPlayer(name, false);
            }
        }

        m_view->addConsumptionLog(QString("玩家 %1 已加入").arg(name), false);
        m_view->reinitAIWidgets(m_engine->getPlayers().size() - 1);

        QJsonObject welcome;
        welcome["type"] = "welcome";
        welcome["id"] = m_joinClientIds.indexOf(clientId) + 1;
        m_network->sendToPlayer(clientId, welcome);
        
        updateView();
        sendStateToAll();
    } else if (type == "action") {
        senderPlayerId = m_joinClientIds.indexOf(clientId) + 1;
        if (senderPlayerId <= 0) return;
        if (m_engine->getCurrentTurnIndex() != senderPlayerId) return;

        QString action = data["action"].toString();
        if (action == "fold") m_engine->fold(senderPlayerId);
        else if (action == "call") m_engine->bet(senderPlayerId, m_engine->calculateRequiredBet(senderPlayerId));
        else if (action == "see") m_engine->seeCards(senderPlayerId);
        else if (action == "raise") m_engine->bet(senderPlayerId, data["amount"].toInt());
        else if (action == "compare") m_engine->compare(senderPlayerId, data["targetId"].toInt());

        sendStateToAll();
    }
}

void GameController::sendStateToAll() {
    if (!m_network->isHost()) return;
    
    QJsonObject sync;
    sync["type"] = "sync";
    sync["pot"] = m_engine->getCurrentPot();
    sync["bet"] = m_engine->getCurrentBet();
    sync["turn"] = m_engine->getCurrentTurnIndex();
    sync["phase"] = static_cast<int>(m_engine->getCurrentPhase());
    
    QJsonArray playersArr;
    auto players = m_engine->getPlayers();
    for (int i = 0; i < players.size(); ++i) {
        auto p = players[i];
        QJsonObject pObj;
        pObj["name"] = p->getName();
        pObj["chips"] = p->getChips();
        pObj["isAI"] = p->isAI();
        pObj["isSeen"] = p->isSeen();
        pObj["status"] = static_cast<int>(p->getStatus());
        
        // 重要：房主端根据连接 ID 发送对应的私有手牌
        // 房主自己的牌 (index 0)
        // 客户端 0 (index 1), 客户端 1 (index 2)...
        
        playersArr.append(pObj);
    }
    sync["players"] = playersArr;
    
    // 房主自己的 UI 更新已经在本地完成，这里只需要广播
    // 为了让每个客户端只能看到自己的牌，我们需要循环发送
    auto clientSockets = m_network->getClientSockets();
    for (int i = 0; i < clientSockets.size(); ++i) {
        int targetPlayerId = i + 1;
        QJsonObject personalSync = sync;
        QJsonArray personalPlayers = sync["players"].toArray();
        
        // 为该特定玩家填充他的手牌数据
        if (targetPlayerId < players.size()) {
            QJsonObject pObj = personalPlayers[targetPlayerId].toObject();
            QJsonArray handArr;
            for (const auto& card : players[targetPlayerId]->getHand().getCards()) {
                QJsonObject cObj;
                cObj["s"] = static_cast<int>(card.getSuit());
                cObj["r"] = static_cast<int>(card.getRank());
                handArr.append(cObj);
            }
            pObj["hand"] = handArr;
            personalPlayers[targetPlayerId] = pObj;
        }
        personalSync["players"] = personalPlayers;
        
        m_network->sendToPlayer(i, personalSync);
    }
}
