#include "PlayerWidget.h"
#include <QtWidgets/QFrame>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

PlayerWidget::PlayerWidget(QWidget* parent) : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);

    // 顶部：玩家信息行 (头像 + 姓名 + 筹码)
    auto infoLayout = new QHBoxLayout();

    // 模拟头像
    auto avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(40, 40);
    avatarLabel->setStyleSheet("background-color: #444; border-radius: 20px; border: 1px solid #666;");
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setText("👤");

    auto nameChipLayout = new QVBoxLayout();
    m_nameLabel = new QLabel(this);
    m_chipsLabel = new QLabel(this);
    m_countdownLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #FFD700;");
    m_chipsLabel->setStyleSheet("font-size: 11px; color: #00FF00;");
    m_countdownLabel->setStyleSheet("font-size: 11px; color: #FF6600; font-weight: bold;");
    nameChipLayout->addWidget(m_nameLabel);
    nameChipLayout->addWidget(m_chipsLabel);
    nameChipLayout->addWidget(m_countdownLabel);
    
    infoLayout->addWidget(avatarLabel);
    infoLayout->addLayout(nameChipLayout);
    infoLayout->addStretch();
    
    // 中间：手牌区域
    auto cardLayout = new QHBoxLayout();
    cardLayout->setSpacing(-30); // 卡牌重叠效果
    for (int i = 0; i < GameConstants::CARDS_PER_PLAYER; ++i) {
        auto cw = new CardWidget(this);
        m_cardWidgets.append(cw);
        cardLayout->addWidget(cw);
    }

    // 底部：状态气泡
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("background: rgba(0,0,0,150); border-radius: 10px; padding: 2px; font-size: 11px; color: white;");

    mainLayout->addLayout(infoLayout);
    mainLayout->addLayout(cardLayout);
    mainLayout->addWidget(m_statusLabel);

    // 初始化倒计时定时器
    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    m_countdownRemaining = 0;
    m_countdownLabel->setText("");
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        if (m_countdownRemaining > 0) {
            --m_countdownRemaining;
            setCountdown(m_countdownRemaining);
        } else {
            m_countdownTimer->stop();
        }
    });

    // 初始状态
    updateStyle(false);
}

void PlayerWidget::paintEvent(QPaintEvent*) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void PlayerWidget::updateStyle(bool isCurrentTurn) {
    if (isCurrentTurn) {
        // 德州扑克高亮：暗色半透明背景 + 亮金色边框
        setStyleSheet("PlayerWidget { "
                      "  border: 3px solid #FFD700; "
                      "  background-color: rgba(255, 255, 255, 30); "
                      "  border-radius: 12px; "
                      "} "
                      "QLabel { background: transparent; color: #FFFFFF; }");
        m_nameLabel->setStyleSheet("font-weight: bold; font-size: 15px; color: #FFD700;");
    } else {
        // 普通状态：更透明的背景
        setStyleSheet("PlayerWidget { "
                      "  border: 1px solid rgba(255, 255, 255, 50); "
                      "  background-color: rgba(0, 0, 0, 40); "
                      "  border-radius: 10px; "
                      "} "
                      "QLabel { background: transparent; color: #CCCCCC; }");
        m_nameLabel->setStyleSheet("font-weight: normal; color: #EEEEEE;");
    }
}

void PlayerWidget::updatePlayer(const Player* player, bool revealCards, bool isCurrentTurn) {
    m_nameLabel->setText(player->getName());
    m_chipsLabel->setText(QString("筹码: %1").arg(player->getChips()));
    
    QString statusText;
    switch (player->getStatus()) {
        case GameConstants::Active:  
            if (isCurrentTurn) {
                statusText = player->isAI() ? "思考中..." : "请操作";
            } else {
                statusText = player->isSeen() ? "【已看牌】" : "【蒙牌中】";
            }
            break;
        case GameConstants::Folded:  statusText = "已弃牌"; break;
        case GameConstants::Lost:    statusText = "比牌输"; break;
        case GameConstants::Winner:  statusText = "★ 赢家 ★"; break;
        default:                     statusText = ""; break;
    }
    m_statusLabel->setText(statusText);

    // 更新背景加深效果
    updateStyle(isCurrentTurn);

    auto cards = player->getHand().getCards();
    if (cards.size() == GameConstants::CARDS_PER_PLAYER) {
        for (int i = 0; i < GameConstants::CARDS_PER_PLAYER; ++i) {
            m_cardWidgets[i]->setCard(cards[i]);
            m_cardWidgets[i]->setFaceDown(!revealCards);
        }
    }
}

void PlayerWidget::setCountdown(int seconds) {
    if (seconds < 0) {
        m_countdownLabel->setText("");
        return;
    }
    QString color = "#FF6600";
    if (seconds <= 3) {
        color = "#FF0000"; // 红色警告
        m_countdownLabel->setStyleSheet(
            QString("font-size: 13px; font-weight: bold; color: %1;").arg(color));
    } else if (seconds <= 5) {
        m_countdownLabel->setStyleSheet(
            QString("font-size: 11px; font-weight: bold; color: %1;").arg(color));
    } else {
        m_countdownLabel->setStyleSheet(
            QString("font-size: 11px; color: %1;").arg(color));
    }
    m_countdownLabel->setText(QString("⏱ %1s").arg(seconds));
}

void PlayerWidget::startCountdown(int seconds) {
    m_countdownTimer->stop();
    m_countdownRemaining = seconds;
    setCountdown(seconds);
    m_countdownTimer->start();
}

void PlayerWidget::resetCountdown() {
    m_countdownTimer->stop();
    m_countdownRemaining = 0;
    m_countdownLabel->setText("");
}

void PlayerWidget::stopCountdown() {
    m_countdownTimer->stop();
    m_countdownRemaining = 0;
    m_countdownLabel->setText("");
}

QPoint PlayerWidget::getCardGlobalPos(int cardIndex) const {
    if (cardIndex >= 0 && cardIndex < m_cardWidgets.size()) {
        return m_cardWidgets[cardIndex]->mapToGlobal(QPoint(0,0));
    }
    return QPoint(0,0);
}
