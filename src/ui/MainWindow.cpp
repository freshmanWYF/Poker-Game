#include "MainWindow.h"
#include <QtWidgets/QInputDialog>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSequentialAnimationGroup>
#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QTime>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setWindowTitle("炸金花 - 单机版");
    resize(1000, 700);
}

void MainWindow::setupUI() {
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto globalLayout = new QHBoxLayout(centralWidget); // 改为水平布局以容纳侧边栏
    
    auto mainLayout = new QVBoxLayout(); // 原有的垂直布局作为左侧主体
    globalLayout->addLayout(mainLayout, 3); // 主体占 3 份比例

    // 全局 QSS 样式 (现代扑克风格)
    centralWidget->setStyleSheet(
        "QWidget#centralWidget { "
        "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.8, fx:0.5, fy:0.5, stop:0 #1a4a1a, stop:1 #0d2a0d); "
        "} "
        "QPushButton { "
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5a5a5a, stop:1 #3a3a3a); "
        "  color: #e0e0e0; border: 1px solid #1a1a1a; border-radius: 6px; padding: 10px; font-weight: bold; font-family: 'Segoe UI'; "
        "} "
        "QPushButton:hover { background-color: #6a6a6a; color: white; border: 1px solid #FFD700; } "
        "QPushButton:pressed { background-color: #2a2a2a; } "
        "QPushButton:disabled { background-color: #222222; color: #555555; border: 1px solid #111; } "
        "QLabel { color: #eeeeee; font-family: 'Microsoft YaHei'; } "
        "QListWidget { "
        "  background-color: rgba(0, 0, 0, 120); color: #FFD700; border: 1px solid #555; "
        "  border-radius: 8px; font-family: 'Segoe UI'; font-size: 11px; "
        "}"
    );
    centralWidget->setObjectName("centralWidget");

    // 侧边栏：消费记录
    auto sidePanel = new QVBoxLayout();
    auto sideLabel = new QLabel("💰 资金流水", this);
    sideLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #FFD700;");
    m_consumptionList = new QListWidget(this);
    sidePanel->addWidget(sideLabel);
    sidePanel->addWidget(m_consumptionList);
    globalLayout->addLayout(sidePanel, 1); // 侧边栏占 1 份比例

    // AI 玩家展示区域 (顶部一排，增加间距)
    m_aiAreaLayout = new QHBoxLayout();
    m_aiAreaLayout->setContentsMargins(20, 20, 20, 20);
    m_aiAreaLayout->setSpacing(30);
    mainLayout->addLayout(m_aiAreaLayout);

    mainLayout->addStretch();

    // 游戏状态展示区域 (中间，大字体)
    auto statusArea = new QHBoxLayout();
    m_potLabel = new QLabel("奖池: 0", this);
    m_betLabel = new QLabel("底注: 10", this);
    m_potLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFD700; font-family: 'Impact';");
    m_betLabel->setStyleSheet("font-size: 18px; color: #00FFCC; font-family: 'Segoe UI';");
    
    // 增加一个装饰性的奖池背景
    auto potContainer = new QWidget(this);
    potContainer->setStyleSheet("background: rgba(0,0,0,100); border-radius: 20px; padding: 10px 30px; border: 1px solid #FFD700;");
    auto potLayout = new QVBoxLayout(potContainer);
    potLayout->addWidget(m_potLabel, 0, Qt::AlignCenter);
    potLayout->addWidget(m_betLabel, 0, Qt::AlignCenter);

    statusArea->addStretch();
    statusArea->addWidget(potContainer);
    statusArea->addStretch();
    mainLayout->addLayout(statusArea);

    mainLayout->addStretch();

    // 真人玩家展示区域 (底部中央)
    auto humanArea = new QHBoxLayout();
    m_humanWidget = new PlayerWidget(this);
    m_humanWidget->setFixedWidth(320);
    m_humanWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    humanArea->addStretch();
    humanArea->addWidget(m_humanWidget);
    humanArea->addStretch();
    mainLayout->addLayout(humanArea);

    // 控制按钮区域 (分两组：左侧操作，右侧开始)
    auto bottomLayout = new QHBoxLayout();
    auto btnArea = new QHBoxLayout();
    btnArea->setSpacing(10);
    
    m_btnSee = new QPushButton("看牌", this);
    m_btnFold = new QPushButton("弃牌", this);
    m_btnCall = new QPushButton("跟注", this);
    m_btnRaise = new QPushButton("加注", this);
    m_btnCompare = new QPushButton("比牌", this);
    m_btnStart = new QPushButton("开始游戏", this);
    m_btnPlayerCount = new QPushButton("玩家人数", this);
    m_btnHost = new QPushButton("创建房间", this);
    m_btnJoin = new QPushButton("加入房间", this);

    // 按钮美化
    m_btnStart->setStyleSheet(
        "QPushButton { "
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d4af37, stop:1 #b8860b); "
        "  color: white; border: 2px solid #FFD700; border-radius: 8px; font-size: 16px; padding: 12px 30px; "
        "} "
        "QPushButton:hover { background-color: #daa520; } "
        "QPushButton:pressed { background-color: #8b4513; } "
        "QPushButton:disabled { background-color: #555555; color: #888888; border: 2px solid #333333; }"
    );
    m_btnPlayerCount->setStyleSheet(
        "QPushButton { background-color: #444; border: 1px solid #666; } "
        "QPushButton:disabled { background-color: #222; color: #666; border: 1px solid #333; }"
    );
    QString netBtnStyle = "QPushButton { background-color: #2c3e50; color: #ecf0f1; border: 1px solid #34495e; padding: 10px; } "
                          "QPushButton:hover { background-color: #34495e; } "
                          "QPushButton:disabled { background-color: #1a252f; color: #7f8c8d; }";
    m_btnHost->setStyleSheet(netBtnStyle);
    m_btnJoin->setStyleSheet(netBtnStyle);

    btnArea->addWidget(m_btnSee);
    btnArea->addWidget(m_btnFold);
    btnArea->addWidget(m_btnCall);
    btnArea->addWidget(m_btnRaise);
    btnArea->addWidget(m_btnCompare);
    btnArea->addWidget(m_btnPlayerCount);
    btnArea->addWidget(m_btnHost);
    btnArea->addWidget(m_btnJoin);
    
    bottomLayout->addLayout(btnArea);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnStart);
    mainLayout->addLayout(bottomLayout);

    // 连接信号
    connect(m_btnPlayerCount, &QPushButton::clicked, [this]() {
        if (m_gameRunning) return;
        if (m_hasStartedGame) return; // 游戏已经开始过，不允许修改人数
        if (!m_btnPlayerCount->isEnabled()) return; // 再次确保禁用时不会触发
        if (m_lastEngine && m_lastEngine->getCurrentPhase() != GameConstants::Settlement) {
            return; // 游戏未结束时绝对不允许修改人数
        }
        bool ok;
        int count = QInputDialog::getInt(this, "玩家人数", "选择 AI 玩家数量 (1-5):", 3, 1, 5, 1, &ok);
        if (ok) emit playerCountChanged(count);
    });
    connect(m_btnSee, &QPushButton::clicked, this, &MainWindow::seeCardsClicked);
    connect(m_btnFold, &QPushButton::clicked, this, &MainWindow::foldClicked);
    connect(m_btnCall, &QPushButton::clicked, this, &MainWindow::callClicked);
    connect(m_btnRaise, &QPushButton::clicked, [this]() {
        if (!m_lastEngine) return;
        int currentPlayerId = m_lastEngine->getCurrentTurnIndex();
        int requiredBet = m_lastEngine->calculateRequiredBet(currentPlayerId);
        int playerChips = m_lastEngine->getPlayers()[currentPlayerId]->getChips();

        bool ok;
        // 解除加注金额上限：将最大值设为一个极大的数（如 1,000,000），或者根据玩家实际筹码设定
        // 这里设为 100 万，确保玩家可以自由加注
        int amount = QInputDialog::getInt(this, "加注", "请输入加注金额:", 
                                          requiredBet, requiredBet, 1000000, 10, &ok);
        if (ok) emit raiseClicked(amount);
    });
    connect(m_btnCompare, &QPushButton::clicked, [this]() {
        if (!m_lastEngine) return;
        
        QStringList activeOpponentNames;
        QList<int> opponentIds;
        auto players = m_lastEngine->getPlayers();
        
        for (auto p : players) {
            if (p->isActive() && p->getId() != m_localPlayerId) {
                activeOpponentNames << p->getName();
                opponentIds << p->getId();
            }
        }
        
        if (activeOpponentNames.isEmpty()) {
            addConsumptionLog("当前没有可以比牌的对手", false);
            return;
        }

        bool ok;
        QString selectedName = QInputDialog::getItem(this, "比牌", "请选择比牌对象:", activeOpponentNames, 0, false, &ok);
        if (ok) {
            int idx = activeOpponentNames.indexOf(selectedName);
            emit compareClicked(opponentIds[idx]);
        }
    });
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startGameClicked);
    connect(m_btnHost, &QPushButton::clicked, this, &MainWindow::createRoomClicked);
    connect(m_btnJoin, &QPushButton::clicked, [this]() {
        bool ok;
        QString address = QInputDialog::getText(this, "加入房间", "请输入房主 IP 地址:", QLineEdit::Normal, "127.0.0.1", &ok);
        if (ok && !address.isEmpty()) emit joinRoomClicked(address);
    });

    // 重新启用开始按钮和人数选择按钮
    m_btnStart->setEnabled(true);
    m_btnPlayerCount->setEnabled(true);
    setActionButtonsEnabled(false);
}

void MainWindow::updateUI(const GameEngine* engine) {
    m_lastEngine = engine; // 保存最新的引擎状态，供比牌弹窗使用
    auto players = engine->getPlayers();
    if (players.isEmpty()) return;

    int localId = m_localPlayerId;
    if (localId < 0 || localId >= players.size()) {
        localId = 0;
    }

    int currentPlayerId = engine->getCurrentTurnIndex();
    if (currentPlayerId < 0 || currentPlayerId >= players.size()) {
        currentPlayerId = 0;
    }
    auto currentPlayer = players[currentPlayerId];
    
    // 判断是否游戏结束
    bool isGameOver = (engine->getCurrentPhase() == GameConstants::Settlement);
    bool isLocalTurn = (currentPlayerId == localId) && !isGameOver;

    int opponentIdx = 0;
    for (int i = 0; i < players.size(); ++i) {
        auto player = players[i];
        bool isCurrentTurn = (i == currentPlayerId) && !isGameOver;

        bool revealLocal = isGameOver || ((i == localId) && player->isSeen()) || (player->getStatus() == GameConstants::Winner);
        bool revealOpponent = isGameOver || (player->getStatus() == GameConstants::Winner);

        if (i == localId) {
            m_humanWidget->updatePlayer(player, revealLocal, isCurrentTurn);
            m_btnSee->setEnabled(isLocalTurn && player->isActive() && !player->isSeen());
        } else {
            if (opponentIdx < m_aiWidgets.size()) {
                m_aiWidgets[opponentIdx]->updatePlayer(player, revealOpponent, isCurrentTurn);
                opponentIdx++;
            }
        }
    }

    m_potLabel->setText(QString("奖池: %1").arg(engine->getCurrentPot()));
    
    // 显示当前底注和当前玩家需要跟注的额度
    int requiredBet = engine->calculateRequiredBet(currentPlayerId);
    m_betLabel->setText(QString("当前底注: %1 | 需跟注: %2").arg(engine->getCurrentBet()).arg(requiredBet));
}

void MainWindow::setLocalPlayerId(int playerId) {
    m_localPlayerId = playerId;
}

void MainWindow::setActionButtonsEnabled(bool enabled) {
    // 基础操作按钮（弃牌、跟注等）
    m_btnFold->setEnabled(enabled);
    m_btnCall->setEnabled(enabled);
    m_btnRaise->setEnabled(enabled);
    m_btnCompare->setEnabled(enabled);
    
    // 注意：m_btnSee 的状态在 updateUI 中根据玩家是否看过牌单独控制
}

void MainWindow::setGameRunning(bool running) {
    m_gameRunning = running;
    
    // 如果游戏开始，标记游戏已经开始过
    if (running) {
        m_hasStartedGame = true;
    }
    
    // 游戏运行期间，禁用"开始游戏"、"玩家人数"以及联机按钮
    // 游戏结束后，如果已经开始过游戏，继续禁用"玩家人数"按钮
    m_btnStart->setEnabled(!running);
    m_btnPlayerCount->setEnabled(!running && !m_hasStartedGame);
    m_btnPlayerCount->setAttribute(Qt::WA_TransparentForMouseEvents, running || m_hasStartedGame);
    m_btnHost->setEnabled(!running);
    m_btnJoin->setEnabled(!running);
    
    if (!running) {
        // 游戏结束时，确保所有操作按钮也禁用
        setActionButtonsEnabled(false);
        m_btnSee->setEnabled(false);
    }
}

void MainWindow::reinitAIWidgets(int count) {
    while (auto item = m_aiAreaLayout->takeAt(0)) {
        if (auto w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_aiWidgets.clear();

    m_aiAreaLayout->addStretch();
    const int targetWidth = m_humanWidget ? m_humanWidget->width() : 320;
    for (int i = 0; i < count; ++i) {
        auto pw = new PlayerWidget(this);
        pw->setFixedWidth(targetWidth);
        pw->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        m_aiWidgets.append(pw);
        m_aiAreaLayout->addWidget(pw);
    }
    m_aiAreaLayout->addStretch();
}

void MainWindow::playDealingAnimation() {
    auto group = new QParallelAnimationGroup(this);
    QPoint center = this->rect().center();

    // 动态获取当前玩家总数
    int totalPlayers = m_aiWidgets.size() + 1;

    for (int p = 0; p < totalPlayers; ++p) {
        PlayerWidget* targetWidget = (p == 0) ? m_humanWidget : m_aiWidgets[p-1];
        
        for (int i = 0; i < GameConstants::CARDS_PER_PLAYER; ++i) {
            // 创建临时发牌对象
            CardWidget* animCard = new CardWidget(this);
            animCard->setFaceDown(true);
            animCard->show();
            
            QPoint endPos = mapFromGlobal(targetWidget->getCardGlobalPos(i));
            
            auto anim = new QPropertyAnimation(animCard, "pos");
            anim->setDuration(300 + p * 100 + i * 50); // 错开时间
            anim->setStartValue(center);
            anim->setEndValue(endPos);
            anim->setEasingCurve(QEasingCurve::OutQuad);
            
            connect(anim, &QPropertyAnimation::finished, animCard, &CardWidget::deleteLater);
            group->addAnimation(anim);
        }
    }
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::playChipAnimation(int fromPlayerId, bool toPot) {
    QLabel* chip = new QLabel("💰", this);
    chip->setStyleSheet("font-size: 20px; background: transparent;");
    chip->show();

    QPoint start, end;
    if (toPot) {
        // 从玩家飞向奖池
        PlayerWidget* fromWidget = (fromPlayerId == 0) ? m_humanWidget : m_aiWidgets[fromPlayerId-1];
        start = mapFromGlobal(fromWidget->mapToGlobal(QPoint(fromWidget->width()/2, 0)));
        end = mapFromGlobal(m_potLabel->mapToGlobal(QPoint(0,0)));
    } else {
        // 从奖池飞向赢家
        start = mapFromGlobal(m_potLabel->mapToGlobal(QPoint(0,0)));
        PlayerWidget* toWidget = (fromPlayerId == 0) ? m_humanWidget : m_aiWidgets[fromPlayerId-1];
        end = mapFromGlobal(toWidget->mapToGlobal(QPoint(toWidget->width()/2, 0)));
    }

    auto anim = new QPropertyAnimation(chip, "pos");
    anim->setDuration(600);
    anim->setStartValue(start);
    anim->setEndValue(end);
    anim->setEasingCurve(QEasingCurve::InBack);
    
    connect(anim, &QPropertyAnimation::finished, chip, &QLabel::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::addConsumptionLog(const QString& msg, bool highlight) {
    if (!m_consumptionList) return;
    QString timeStr = QTime::currentTime().toString("HH:mm:ss");
    QString fullMsg = QString("[%1] %2").arg(timeStr).arg(msg);
    auto item = new QListWidgetItem(fullMsg);
    if (highlight) {
        item->setForeground(QColor("#00FF66"));
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
    }
    m_consumptionList->insertItem(0, item);
}

void MainWindow::clearConsumptionLog() {
    if (m_consumptionList) {
        m_consumptionList->clear();
    }
}
