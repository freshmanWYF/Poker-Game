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
    connect(m_engine, &GameEngine::phaseChanged, this, &GameController::handlePhaseChanged);
    connect(m_engine, &GameEngine::playerActed, this, &GameController::handlePlayerActed);
    connect(m_engine, &GameEngine::compareResult, this,
        [this](int winnerId, int loserId, const QString&, const QString&) {
            // 比牌后短暂翻开输家牌面 2 秒
            m_view->revealPlayerTemporarily(loserId, 2000);
        });
    connect(m_engine, &GameEngine::roundCompleted, this, &GameController::handleRoundCompleted);

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

    // WebSocket 手机端：连接/断开事件
    connect(this, &GameController::wsClientJoined, this, &GameController::onWSClientJoined);
    connect(this, &GameController::wsClientAction, this, &GameController::onWSClientAction);
    connect(this, &GameController::wsClientDisconnected, this, &GameController::onWSClientDisconnected);

    // 游戏事件 → WebSocket 广播
    connect(m_engine, &GameEngine::gameStateChanged, this, &GameController::broadcastWebSocketState);
    connect(m_engine, &GameEngine::turnStarted, this, &GameController::broadcastWebSocketState);
    connect(m_engine, &GameEngine::gameOver, this, &GameController::broadcastWebSocketState);

    // 初始化默认玩家 (1真人 + 3AI)
    onPlayerCountChanged(3);
    m_myPlayerId = 0;
    m_view->setLocalPlayerId(0);
}

GameController::~GameController() {
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

void GameController::startCountdown(int playerId) {
    if (playerId < 0) return;
    m_countdownPlayerId = playerId;
    m_countdownSeconds = COUNTDOWN_SECONDS;
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
    if (m_countdownPlayerId < 0) return;

    m_countdownSeconds--;
    if (m_countdownSeconds <= 0) {
        int playerId = m_countdownPlayerId;
        m_countdownPlayerId = -1; // 重置，再 fold（fold 会触发 handleTurnStarted）
        m_countdownTimer->stop();
        m_view->setPlayerCountdown(playerId, -1);
        Logger::instance().log(QString("玩家 %1 超时，自动弃牌")
            .arg(m_engine->getPlayers()[playerId]->getName()));
        m_engine->fold(playerId);
    } else {
        m_view->setPlayerCountdown(m_countdownPlayerId, m_countdownSeconds);
    }
}

void GameController::handleTurnStarted(int playerId) {
    auto players = m_engine->getPlayers();
    auto currentPlayer = players[playerId];

    Logger::instance().log(QString("轮到玩家: %1").arg(currentPlayer->getName()));

    bool isWsPlayer = m_wsClientPlayerMap.values().contains(playerId);
    bool isPendingReplacement = m_pendingReplacements.contains(playerId);

    // 检查玩家筹码是否足够下注，不够则自动弃牌防止死局
    int requiredBet = m_engine->calculateRequiredBet(playerId);
    if (currentPlayer->getChips() < requiredBet) {
        Logger::instance().log(QString("玩家 %1 筹码不足(%2)，自动弃牌").arg(currentPlayer->getName()).arg(currentPlayer->getChips()));
        m_engine->fold(playerId);
        // fold 已调用 checkGameOver 和 nextTurn，会触发新的 turnStarted
        return;
    }

    if (isPendingReplacement) {
        // 玩家正在被替换为 AI，等待转换完成后再处理
        return;
    } else if (currentPlayer->isAI() && !isWsPlayer) {
        // 只有真正的 AI 才启动计时器自动操作
        m_view->setActionButtonsEnabled(false);
        m_aiTimer->start(1500);
        startCountdown(playerId); // AI 也显示倒计时，但超时会自动弃牌
    } else if (playerId == m_myPlayerId) {
        m_view->setActionButtonsEnabled(true);
        startCountdown(playerId);
    } else if (isWsPlayer) {
        // WebSocket 手机玩家：启用桌面操作区占位，按钮由 Web UI 控制
        m_view->setActionButtonsEnabled(false);
        m_view->addConsumptionLog(QString("等待 %1 操作中...").arg(currentPlayer->getName()), true);
        // 立即广播状态让 Web UI 刷新按钮
        broadcastWebSocketState();
        startCountdown(playerId);
    } else {
        m_view->setActionButtonsEnabled(false);
        broadcastWebSocketState();
        startCountdown(playerId);
    }
}

void GameController::handleTurnEnded(int playerId) {
    // 预留接口，处理回合结束逻辑
}

void GameController::processAI() {
    int currentId = m_engine->getCurrentTurnIndex();
    auto player = m_engine->getPlayers()[currentId];

    // 查找该 AI 对应的策略（策略列表索引 = 玩家 ID - 1，因为玩家 0 是真人）
    int strategyIdx = currentId - 1;
    AIStrategy* strategy = nullptr;
    if (strategyIdx >= 0 && strategyIdx < m_aiStrategies.size()) {
        strategy = m_aiStrategies[strategyIdx];
    }
    if (!strategy) return; // 无策略则跳过

    auto action = strategy->decide(player, m_engine);
    
    switch (action) {
        case AIStrategy::Fold:
            stopCountdown();
            m_engine->fold(currentId);
            break;
        case AIStrategy::Call:
            stopCountdown();
            m_engine->bet(currentId, m_engine->calculateRequiredBet(currentId));
            break;
        case AIStrategy::Raise: {
            // 简单的加注逻辑：在当前暗牌底注基础上增加一个最小步长
            stopCountdown();
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

    // 计算实际真人玩家数量（房主 + 手机玩家）
    // 不能把玩家数减少到低于已有真人数量，防止手机玩家失去槽位
    int humanCount = 1 + m_wsClientIds.size();
    if (count < humanCount) {
        count = humanCount;
    }

    m_engine->resetPlayers();
    clearAIStrategies();

    m_hostName = "房主";
    m_engine->addPlayer(m_hostName, false);
    for (int i = 0; i < count - 1; ++i) {
        AIStrategy* strategy = createRandomStrategy();
        m_aiStrategies.append(strategy);
        m_engine->addPlayer(QString("AI %1 %2").arg(strategy->label()).arg(i + 1), true);
    }

    // 如果有手机玩家已加入，重新映射到正确的槽位
    if (!m_wsClientIds.isEmpty()) {
        auto players = m_engine->getPlayers();
        for (int i = 0; i < m_wsClientIds.size() && i + 1 < players.size(); ++i) {
            int clientId = m_wsClientIds[i];
            int playerId = i + 1;
            m_wsClientPlayerMap[clientId] = playerId;
            // 重置该槽位为手机玩家（非 AI）
            players[playerId]->setChips(GameConstants::INITIAL_CHIPS);
            players[playerId]->setStatus(GameConstants::Active);
            players[playerId]->setSeen(false);
            players[playerId]->setHand(Hand());
            // 重新发送 welcome 通知正确的玩家索引
            m_webSocketServer->sendWelcome(clientId, playerId);
            Logger::instance().log(QString("[GC] 重映射手机玩家 clientId=%1 → playerId=%2").arg(clientId).arg(playerId));
        }
    }

    m_view->reinitAIWidgets(count - 1);
    tryRestoreChips();
    updateView();
    broadcastWebSocketState();
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
    stopCountdown();
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
    stopCountdown();
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
    stopCountdown();
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
    stopCountdown();
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
    stopCountdown();
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

    // 保存筹码存档
    GameStore::instance().saveChipState(players);

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
    auto players = m_engine->getPlayers();
    if (winnerId < 0 || winnerId >= players.size()) return;

    // 构建对局记录
    MatchRecord record;
    record.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    record.winner = players[winnerId]->getName();
    record.winnerType = players[winnerId]->getHand().typeName();
    record.pot = pot;

    for (auto p : players) {
        MatchRecord::PlayerEntry entry;
        entry.name = p->getName();
        entry.chipsAfter = p->getChips();
        // chipsBefore = chipsAfter - 赢的 或 + 输的
        if (p->getId() == winnerId) {
            entry.chipsBefore = p->getChips() - pot;
        } else {
            // 估算：从玩家最后筹码和当前状态推算不太精确，
            // 但对局历史主要用于展示趋势，大致准确即可
            entry.chipsBefore = p->getChips();
        }
        entry.handType = (p->getStatus() == GameConstants::Folded) ? "弃牌" : p->getHand().typeName();
        record.players.append(entry);
    }

    GameStore::instance().addMatchRecord(record);

    // 更新战绩
    for (auto p : players) {
        bool won = (p->getId() == winnerId);
        int delta = won ? pot : 0;
        QString handType = (p->getStatus() == GameConstants::Folded) ? "弃牌" : p->getHand().typeName();
        GameStore::instance().updateStats(p->getName(), won, delta, handType);
    }
}

void GameController::onCreateRoom() {
    // 启动 HTTP 服务器（提供手机 Web UI）
    if (!m_httpServer) {
        m_httpServer = new HttpServer(this);
        connect(m_httpServer, &HttpServer::serverStarted, [this](const QString& url) {
            m_view->addConsumptionLog(QString("手机扫码地址: %1").arg(url), true);
            m_view->showQRCode(url);
        });
    }
    if (!m_httpServer->serverPort()) {
        if (!m_httpServer->start(8080)) {
            m_view->addConsumptionLog("HTTP 服务器启动失败，手机端不可用", false);
        }
    }

    // 启动 WebSocket 服务器（手机客户端通信）
    if (!m_webSocketServer) {
        m_webSocketServer = new WebSocketServer(m_engine, this);

        // WebSocket 客户端加入 → 触发 wsClientJoined 信号
        connect(m_webSocketServer, &WebSocketServer::clientConnected,
                [this](int clientId, const QString& name) {
                    emit wsClientJoined(clientId, name);
                });

        // WebSocket 客户端断开 → 替换为 AI
        connect(m_webSocketServer, &WebSocketServer::clientDisconnected,
                [this](int clientId) {
                    emit wsClientDisconnected(clientId);
                });

        // WebSocket 客户端发送动作
        connect(m_webSocketServer, &WebSocketServer::actionReceived,
                [this](int clientId, const QJsonObject& data) {
                    emit wsClientAction(clientId, data);
                });
    }
    if (!m_webSocketServer->start(12347)) {
        m_view->addConsumptionLog("WebSocket 服务器启动失败", false);
    }

    m_myPlayerId = 0;
    m_view->setLocalPlayerId(0);
    m_wsClientIds.clear();
    m_wsClientNames.clear();
    m_hostName = m_engine->getPlayers().isEmpty() ? QString("房主") : m_engine->getPlayers().first()->getName();
    m_view->addConsumptionLog("房间创建成功！手机扫码加入", true);
    m_view->reinitAIWidgets(m_engine->getPlayers().size() - 1);

    // 同时保留旧的 TCP 服务器供老客户端使用
    if (m_network->startServer()) {
        const auto joinAddresses = m_network->getJoinAddresses();
        if (!joinAddresses.isEmpty()) {
            m_view->addConsumptionLog(QString("旧版客户端地址: %1").arg(joinAddresses.first()), false);
        }
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

void GameController::onWSClientJoined(int clientId, const QString& name) {
    m_view->addConsumptionLog(QString("手机玩家 %1 已加入").arg(name), true);

    // 替换一个 AI 占位，或者追加新玩家
    bool replaced = false;
    auto players = m_engine->getPlayers();
    for (int i = 1; i < players.size(); ++i) { // 跳过 index 0（房主）
        if (players[i]->isAI()) {
            m_wsClientIds.append(clientId);
            m_wsClientNames.append(name);
            m_wsClientPlayerMap[clientId] = i;
            players[i]->setChips(GameConstants::INITIAL_CHIPS);
            players[i]->setStatus(GameConstants::Active);
            players[i]->setSeen(false);
            players[i]->setHand(Hand());
            players[i]->setName(name);
            // 手机玩家不消耗策略槽位，strategy[i-1] 对应的是当前 AI 的策略
            replaced = true;
            // 告知客户端它的游戏玩家索引（字段名 id 与 Web UI 期望一致）
            m_webSocketServer->sendWelcome(clientId, i);
            Logger::instance().log(QString("[GC] 替换 AI 玩家 index=%1, clientId=%2").arg(i).arg(clientId));
            break;
        }
    }

    if (!replaced) {
        int playerId = m_engine->getPlayers().size();
        m_engine->addPlayer(name, false);
        m_wsClientIds.append(clientId);
        m_wsClientNames.append(name);
        m_wsClientPlayerMap[clientId] = playerId;
        m_webSocketServer->sendWelcome(clientId, playerId);
        Logger::instance().log(QString("[GC] 新增玩家 index=%1, clientId=%2").arg(playerId).arg(clientId));
    }

    m_view->reinitAIWidgets(m_engine->getPlayers().size() - 1);
    updateView();
    broadcastWebSocketState();
}

void GameController::onWSClientDisconnected(int clientId) {
    int wsIdx = m_wsClientIds.indexOf(clientId);
    if (wsIdx < 0) return;

    int playerId = m_wsClientPlayerMap.value(clientId, -1);
    if (playerId < 0) return;

    auto players = m_engine->getPlayers();
    if (playerId >= players.size()) return;

    QString disconnectedName = m_wsClientNames.value(wsIdx, "玩家");
    m_view->addConsumptionLog(QString("%1 已掉线，由 AI 接管").arg(disconnectedName), false);

    // 标记为正在替换，防止竞态
    m_pendingReplacements.insert(playerId);

    // 清理 WebSocket 相关映射
    m_wsClientIds.removeAt(wsIdx);
    m_wsClientNames.removeAt(wsIdx);
    m_wsClientPlayerMap.remove(clientId);

    // 将掉线玩家转为 AI
    players[playerId]->setIsAI(true);
    players[playerId]->setSeen(false);
    players[playerId]->setHand(Hand());

    // 重建策略数组：为所有 AI 玩家创建策略
    clearAIStrategies();
    for (int i = 1; i < players.size(); ++i) {
        if (players[i]->isAI()) {
            AIStrategy* strategy = createRandomStrategy();
            m_aiStrategies.append(strategy);
            players[i]->setName(QString("AI %1").arg(strategy->label()));
        }
    }

    // 移除待替换标记
    m_pendingReplacements.remove(playerId);

    m_view->reinitAIWidgets(players.size() - 1);
    updateView();
    broadcastWebSocketState();
}

void GameController::onWSClientAction(int clientId, const QJsonObject& data) {
    int wsIdx = m_wsClientIds.indexOf(clientId);
    if (wsIdx < 0) return;

    // WebSocket 客户端直接用玩家索引（之前已告知）
    int playerId = m_wsClientPlayerMap.value(clientId, -1);
    if (playerId < 0) return;
    if (m_engine->getCurrentTurnIndex() != playerId) return;

    stopCountdown();

    QString action = data["action"].toString();
    if (action == "fold") m_engine->fold(playerId);
    else if (action == "call") m_engine->bet(playerId, m_engine->calculateRequiredBet(playerId));
    else if (action == "see") m_engine->seeCards(playerId);
    else if (action == "raise") m_engine->bet(playerId, data["amount"].toInt());
    else if (action == "compare") m_engine->compare(playerId, data["targetId"].toInt());

    broadcastWebSocketState();
}

void GameController::broadcastWebSocketState() {
    if (!m_webSocketServer || m_webSocketServer->clientCount() == 0) return;

    auto players = m_engine->getPlayers();

    // 为每个 WebSocket 客户端发送个性化状态（只显示自己的手牌）
    for (int i = 0; i < m_wsClientIds.size(); ++i) {
        int clientId = m_wsClientIds[i];
        int playerId = m_wsClientPlayerMap.value(clientId, -1);
        if (playerId < 0) continue;

        QJsonObject sync;
        sync["type"] = "sync";
        sync["pot"] = m_engine->getCurrentPot();
        sync["bet"] = m_engine->getCurrentBet();
        sync["turn"] = m_engine->getCurrentTurnIndex();
        sync["phase"] = static_cast<int>(m_engine->getCurrentPhase());

        QJsonArray playersArr;
        for (int j = 0; j < players.size(); ++j) {
            auto* p = players[j];
            QJsonObject pObj;
            pObj["name"] = p->getName();
            pObj["chips"] = p->getChips();
            pObj["isAI"] = p->isAI();
            pObj["isSeen"] = p->isSeen();
            pObj["status"] = static_cast<int>(p->getStatus());

            // 只发给该客户端对应的玩家手牌
            if (j == playerId) {
                QJsonArray handArr;
                for (const auto& card : p->getHand().getCards()) {
                    QJsonObject cardObj;
                    cardObj["s"] = static_cast<int>(card.getSuit());
                    cardObj["r"] = static_cast<int>(card.getRank());
                    handArr.append(cardObj);
                }
                pObj["hand"] = handArr;
            }
            playersArr.append(pObj);
        }
        sync["players"] = playersArr;
        m_webSocketServer->sendToClient(clientId, sync);
    }
}
