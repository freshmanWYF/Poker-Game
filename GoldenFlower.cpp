//
// Created for GoldenFlower Game Implementation
// 炸金花游戏实现文件
// 本文件实现了炸金花游戏的核心逻辑和界面交互功能
//

#define _USE_MATH_DEFINES  // 定义数学常量，用于后面的极坐标计算
#include "GoldenFlower.h"  // 包含游戏头文件
#include "Card.h"          // 包含扑克牌类定义
#include <QEvent>           // 包含Qt事件类，用于处理窗口事件
#include <algorithm>       // 包含算法库，用于洗牌和排序
#include <cmath>           // 包含数学库，用于计算玩家位置
#include <QPixmap>         // 包含Qt图像处理类，用于显示扑克牌图片
#include <QDir>            // 包含Qt目录操作类，用于查找图片文件
#include <QGroupBox>       // 包含Qt分组框类，用于界面布局

// Player类实现
// 玩家类构造函数
Player::Player(const string& name, int initialMoney) 
    : name(name),                    // 初始化玩家名称
      money(initialMoney),           // 初始化玩家初始金额
      currentBet(0),                 // 初始化当前下注金额为0
      currentRoundBet(0),            // 初始化当前轮次下注金额为0
      status(PlayerStatus::BLIND),   // 初始化玩家状态为蒙牌
      isDealer(false) {}             // 初始化玩家不是庄家

// 重置玩家状态，用于开始新一局游戏
void Player::reset() {
    cards.clear();                   // 清空玩家手牌
    currentBet = 0;                  // 重置当前总下注额
    currentRoundBet = 0;             // 重置当前轮次下注额
    status = PlayerStatus::BLIND;    // 重置玩家状态为蒙牌
}

// 玩家下注方法
void Player::placeBet(int amount) {
    if (amount > money) amount = money; // 防止超额下注，最多下注玩家所有剩余金额
    money -= amount;                   // 从玩家余额中扣除下注金额
    currentBet += amount;              // 增加玩家当前总下注额
    currentRoundBet = amount;          // 记录当前轮次下注金额
}

// 玩家接收一张牌
void Player::receiveCard(const string& card) {
    cards.push_back(card);             // 将牌添加到玩家手牌中
}

// GoldenFlowerWindow类实现
// 游戏主窗口构造函数
GoldenFlowerWindow::GoldenFlowerWindow(QWidget *parent)
    : QMainWindow(parent),            // 调用父类构造函数
      currentPlayerIndex(0),          // 初始化当前玩家索引为0
      pot(0),                         // 初始化奖池金额为0
      minBet(10),                     // 初始化最小下注额为10
      entranceFee(10),                // 初始化入场费为10
      gameInProgress(false),          // 初始化游戏未进行中
      tableWidth(550),                // 初始化牌桌宽度
      tableHeight(350),               // 初始化牌桌高度
      playerInfoDistance(50),         // 初始化玩家信息距离
      cardDistance(50),               // 初始化卡牌距离
      scaleFactor(1.0) {              // 初始化界面缩放因子
    initializeUI();                   // 初始化用户界面
    
    // 启用窗口大小变化事件追踪，以便实现响应式布局
    this->installEventFilter(this);
}

// 事件过滤器，用于处理窗口大小变化事件
bool GoldenFlowerWindow::eventFilter(QObject *watched, QEvent *event) {
    // 只处理当前窗口的事件
    if (watched == this) {
        // 处理窗口大小变化事件
        if (event->type() == QEvent::Resize) {
            // 获取当前窗口大小
            QSize newSize = this->size();
            
            // 计算缩放因子 - 基于初始窗口大小900x700
            float widthScale = newSize.width() / 900.0f;
            float heightScale = newSize.height() / 700.0f;
            
            // 使用宽度和高度的缩放因子，确保横向放大时元素也随之放大
            // 不再使用较小值，而是分别应用宽度和高度的缩放因子
            float newScaleFactor = widthScale;
            
            // 计算新的牌桌尺寸，直接基于初始尺寸和新的缩放因子计算
            // 牌桌宽度使用宽度缩放因子，高度使用高度缩放因子，保持比例
            int newTableWidth = int(550 * widthScale);
            int newTableHeight = int(350 * heightScale);
            
            // 距离参数也需要随着缩放比例调整，使用综合缩放因子
            int newPlayerInfoDistance = int(50 * newScaleFactor);
            int newCardDistance = int(50 * newScaleFactor);
            
            // 调整布局参数
            adjustLayoutParameters(newTableWidth, newTableHeight, newPlayerInfoDistance, newCardDistance, newScaleFactor);
        }
    }
    
    // 继续处理其他事件
    return QMainWindow::eventFilter(watched, event);
}

// 初始化用户界面
void GoldenFlowerWindow::initializeUI() {
    // 初始化缩放因子为1.0，表示原始大小
    scaleFactor = 1.0;
    
    // 创建中央窗口部件
    centralWidget = new QWidget();                // 创建中央窗口部件
    setCentralWidget(centralWidget);             // 设置为主窗口的中央部件
    
    // 设置窗口背景颜色为深蓝色
    centralWidget->setStyleSheet("background-color: #0a1f44;");  // 设置深蓝色背景
    
    // 创建主布局
    mainLayout = new QVBoxLayout(centralWidget);  // 创建垂直布局作为主布局
    mainLayout->setContentsMargins(20, 20, 20, 20);  // 设置布局边距
    mainLayout->setSpacing(10);                      // 设置布局间距
    
    // 创建顶部信息布局（显示底注和总奖池）
    QHBoxLayout* topInfoLayout = new QHBoxLayout();  // 创建水平布局用于顶部信息
    QLabel* potInfoLabel = new QLabel("底 : 0\n总 : 0");  // 创建标签显示底注和奖池
    potInfoLabel->setAlignment(Qt::AlignCenter);  // 设置文本居中对齐
    potInfoLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");  // 设置文本样式
    topInfoLayout->addStretch();  // 添加弹性空间使标签居中
    topInfoLayout->addWidget(potInfoLabel);  // 添加标签到布局
    topInfoLayout->addStretch();  // 添加弹性空间使标签居中
    mainLayout->addLayout(topInfoLayout);  // 将顶部信息布局添加到主布局
    
    // 创建牌桌布局 - 使用网格布局使牌桌位于中央
    QGridLayout* tableLayout = new QGridLayout();  // 创建网格布局用于牌桌
    tableLayout->setSpacing(20);  // 设置网格间距
    tableLayout->setAlignment(Qt::AlignCenter);  // 设置内容居中对齐
    tableLayout->setContentsMargins(30, 30, 30, 100);  // 设置外边距，适当减小底部边距使牌桌更居中
    
    // 创建牌桌背景 - 使用两边加半圆的形状，更符合专业赌桌样式
    QLabel* tableBackground = new QLabel();  // 创建标签作为牌桌背景
    tableBackground->setFixedSize(tableWidth, tableHeight);  // 设置牌桌固定大小
    // 设置绿色背景和棕色边框，使用border-radius在两端添加半圆效果
    QString tableStyle = QString("background-color: #0a6e31; border: 8px solid #8B4513; border-radius: %1px;").arg(tableHeight / 2);  // 半圆半径为高度的一半
    tableBackground->setStyleSheet(tableStyle);  // 应用样式
    tableBackground->setObjectName("tableBackground");  // 设置对象名，便于后续查找
    
    // 创建玩家区域布局（使用绝对定位布局放置玩家）
    playerLayout = new QGridLayout(tableBackground);  // 创建网格布局用于放置玩家
    playerLayout->setContentsMargins(10, 10, 10, 10);  // 设置内边距
    
    // 不再创建中央区域标签，移除中心标记
    
    // 将牌桌背景添加到表格布局
    tableLayout->addWidget(tableBackground, 0, 0, 1, 1, Qt::AlignCenter);  // 添加牌桌背景到表格布局，确保居中
    
    // 将表格布局添加到主布局
    mainLayout->addLayout(tableLayout);  // 将牌桌布局添加到主布局
    
    // 创建按钮布局
    buttonLayout = new QHBoxLayout();  // 创建水平布局用于按钮
    buttonLayout->setSpacing(10);      // 设置按钮间距
    mainLayout->addLayout(buttonLayout);  // 将按钮布局添加到主布局
    
    // 创建按钮
    startButton = new QPushButton("开始游戏", this);  // 创建开始游戏按钮
    lookButton = new QPushButton("看牌", this);       // 创建看牌按钮
    betButton = new QPushButton("下注", this);        // 创建下注按钮
    foldButton = new QPushButton("弃牌", this);       // 创建弃牌按钮
    requestShowdownButton = new QPushButton("请求开牌", this);  // 创建请求开牌按钮
    
    // 设置按钮样式
    QString buttonStyle = "QPushButton { background-color: #FFA500; color: white; font-weight: bold; padding: 8px 16px; border-radius: 5px; } "
                         "QPushButton:hover { background-color: #FF8C00; } "
                         "QPushButton:disabled { background-color: #A9A9A9; }";  // 定义按钮样式
    startButton->setStyleSheet(buttonStyle);  // 应用样式到开始按钮
    lookButton->setStyleSheet(buttonStyle);   // 应用样式到看牌按钮
    betButton->setStyleSheet(buttonStyle);    // 应用样式到下注按钮
    foldButton->setStyleSheet(buttonStyle);   // 应用样式到弃牌按钮
    requestShowdownButton->setStyleSheet(buttonStyle);  // 应用样式到请求开牌按钮
    
    // 添加按钮到布局
    buttonLayout->addWidget(startButton);  // 添加开始游戏按钮
    buttonLayout->addWidget(lookButton);   // 添加看牌按钮
    buttonLayout->addWidget(betButton);    // 添加下注按钮
    buttonLayout->addWidget(foldButton);   // 添加弃牌按钮
    buttonLayout->addWidget(requestShowdownButton);  // 添加请求开牌按钮
    
    // 连接信号和槽
    connect(startButton, &QPushButton::clicked, this, &GoldenFlowerWindow::startNewGame);  // 连接开始游戏按钮点击信号
    connect(lookButton, &QPushButton::clicked, this, &GoldenFlowerWindow::lookCards);       // 连接看牌按钮点击信号
    connect(betButton, &QPushButton::clicked, this, &GoldenFlowerWindow::placeBet);         // 连接下注按钮点击信号
    connect(foldButton, &QPushButton::clicked, this, &GoldenFlowerWindow::fold);            // 连接弃牌按钮点击信号
    connect(requestShowdownButton, &QPushButton::clicked, this, &GoldenFlowerWindow::requestShowdown);  // 连接请求开牌按钮点击信号
    
    // 初始禁用游戏按钮
    lookButton->setEnabled(false);  // 初始禁用看牌按钮
    betButton->setEnabled(false);   // 初始禁用下注按钮
    foldButton->setEnabled(false);  // 初始禁用弃牌按钮
    requestShowdownButton->setEnabled(false);  // 初始禁用请求开牌按钮
    
    // 保存奖池信息标签的引用
    potInfoLabel->setObjectName("potInfoLabel");  // 设置标签对象名，便于后续查找
    
    // 设置窗口标题和大小
    setWindowTitle("炸金花游戏");  // 设置窗口标题
    resize(900, 700);              // 设置窗口大小（增大以适应更大的牌桌）
}

// 调整布局参数
// 调整布局参数的公共接口，允许外部调用修改玩家信息和扑克牌与交点之间的距离
void GoldenFlowerWindow::setDistanceParameters(int newPlayerInfoDistance, int newCardDistance) {
    playerInfoDistance = newPlayerInfoDistance;
    cardDistance = newCardDistance;
    
    // 如果游戏正在进行中，更新UI以反映新的布局参数
    if (gameInProgress) {
        updateUI();  // 更新UI
    }
}

void GoldenFlowerWindow::adjustLayoutParameters(int newTableWidth, int newTableHeight, int newPlayerInfoDistance, int newCardDistance, float newScaleFactor) {
    // 更新布局参数
    tableWidth = newTableWidth;
    tableHeight = newTableHeight;
    playerInfoDistance = newPlayerInfoDistance;
    cardDistance = newCardDistance;
    scaleFactor = newScaleFactor;
    
    // 更新牌桌大小和样式
    QLabel* tableBackground = centralWidget->findChild<QLabel*>("tableBackground");
    if (tableBackground) {
        tableBackground->setFixedSize(tableWidth, tableHeight);  // 更新牌桌大小
        // 更新样式，保持两端半圆效果，边框宽度也随缩放调整
        int borderWidth = int(8 * scaleFactor);
        // 使用椭圆形边框，确保在横向放大时保持正确的圆角效果
        // 半径应该与高度成比例，但不应超过宽度的一半
        int borderRadius = qMin(tableHeight / 2, tableWidth / 2);
        QString tableStyle = QString("background-color: #0a6e31; border: %1px solid #8B4513; border-radius: %2px;").arg(borderWidth).arg(borderRadius);  // 使用计算的边框半径
        tableBackground->setStyleSheet(tableStyle);  // 应用新样式
        
        // 中央标签已移除，不再需要更新
    }
    
    // 调整按钮大小和字体
    QFont buttonFont;
    buttonFont.setPointSizeF(10 * scaleFactor); // 设置按钮字体大小
    
    // 更新所有按钮的字体大小和内边距
    QList<QPushButton*> buttons = centralWidget->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        button->setFont(buttonFont);
        // 调整按钮内边距
        int paddingH = int(16 * scaleFactor);
        int paddingV = int(8 * scaleFactor);
        button->setStyleSheet(QString("QPushButton { background-color: #FFA500; color: white; font-weight: bold; padding: %1px %2px; border-radius: %3px; } "
                             "QPushButton:hover { background-color: #FF8C00; } "
                             "QPushButton:disabled { background-color: #A9A9A9; }").arg(paddingV).arg(paddingH).arg(int(5 * scaleFactor)));
    }
    
    // 调整布局间距
    mainLayout->setContentsMargins(int(20 * scaleFactor), int(20 * scaleFactor), int(20 * scaleFactor), int(20 * scaleFactor));
    mainLayout->setSpacing(int(10 * scaleFactor));
    
    if (buttonLayout) {
        buttonLayout->setSpacing(int(10 * scaleFactor));
    }
    
    // 获取奖池信息标签并调整字体大小
    QLabel* potInfoLabel = centralWidget->findChild<QLabel*>("potInfoLabel");
    if (potInfoLabel) {
        QFont font = potInfoLabel->font();
        font.setPointSizeF(16 * scaleFactor);
        potInfoLabel->setFont(font);
    }
    
    // 如果游戏正在进行中，更新UI以反映新的布局参数
    if (gameInProgress) {
        updateUI();  // 更新UI
    }
}

// 开始新游戏
void GoldenFlowerWindow::startNewGame() {
    // 获取玩家数量、初始金额和入场费
    bool ok;  // 用于存储对话框结果
    // 弹出对话框获取玩家数量，默认4人，范围2-17人
    int numPlayers = QInputDialog::getInt(this, "新游戏", "请输入玩家数量(2-17):",
                                        4, 2, 17, 1, &ok);
    if (!ok) return;  // 用户取消，直接返回
    
    // 弹出对话框获取初始金额，默认1000，范围100-100000，步长100
    int initialMoney = QInputDialog::getInt(this, "新游戏", "请输入初始金额:",
                                          1000, 100, 100000, 100, &ok);
    if (!ok) return;  // 用户取消，直接返回
    
    // 弹出对话框获取入场费，默认为最小下注额，范围1-初始金额的十分之一
    entranceFee = QInputDialog::getInt(this, "新游戏", "请输入入场费:",
                                     minBet, 1, initialMoney / 10, 1, &ok);
    if (!ok) return;  // 用户取消，直接返回
    
    // 初始化玩家
    players.clear();  // 清空玩家列表
    for (int i = 0; i < numPlayers; ++i) {
        players.emplace_back("玩家" + to_string(i+1), initialMoney);  // 创建玩家对象并添加到列表
    }
    
    // 随机选择庄家
    int dealerIndex = rand() % numPlayers;  // 随机生成庄家索引
    players[dealerIndex].isDealer = true;   // 设置选中的玩家为庄家
    currentPlayerIndex = dealerIndex;       // 设置当前玩家为庄家
    
    // 启用游戏按钮
    lookButton->setEnabled(true);           // 启用看牌按钮
    betButton->setEnabled(true);            // 启用下注按钮
    foldButton->setEnabled(true);           // 启用弃牌按钮
    requestShowdownButton->setEnabled(true); // 启用请求开牌按钮
    startButton->setEnabled(false);         // 禁用开始游戏按钮
    
    gameInProgress = true;  // 设置游戏进行中标志
    setupGame();            // 设置游戏（发牌等）
}

// 设置游戏（初始化牌局）
void GoldenFlowerWindow::setupGame() {
    // 清理上一局数据
    pot = 0;  // 清空奖池
    for (auto& player : players) {
        player.reset();  // 重置玩家状态（清空手牌、下注额等）
        // 收取入场费，但不计入下注总额
        player.money -= entranceFee;  // 从玩家余额中扣除入场费
    }
    
    // 创建一副完整的扑克牌
    vector<string> deck;  // 牌组
    vector<string> suits = {"Hearts", "Spades", "Diamonds", "Clubs"};  // 花色：红桃、黑桃、方块、梅花
    vector<string> ranks = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};  // 点数：2-A
    
    // 生成52张牌
    for (const auto& suit : suits) {
        for (const auto& rank : ranks) {
            deck.push_back(rank + " of " + suit);  // 组合花色和点数生成牌
        }
    }
    
    // 洗牌
    random_shuffle(deck.begin(), deck.end());  // 随机打乱牌组顺序
    
    // 发牌
    for (int i = 0; i < 3; ++i) { // 每个玩家发3张牌
        for (auto& player : players) {
            if (!deck.empty()) {  // 确保牌组不为空
                player.receiveCard(deck.back());  // 从牌组末尾取一张牌给玩家
                deck.pop_back();  // 从牌组中移除已发出的牌
            }
        }
    }
    
    updateUI();  // 更新用户界面
}

// 更新用户界面
void GoldenFlowerWindow::updateUI() {
    // 清理现有标签
    for (auto label : playerLabels) {
        delete label;  // 删除之前创建的玩家标签
    }
    playerLabels.clear();  // 清空玩家标签列表
    
    // 清理现有卡牌标签
    for (auto label : cardLabels) {
        delete label;  // 删除之前创建的卡牌标签
    }
    cardLabels.clear();  // 清空卡牌标签列表
    
    // 获取奖池信息标签
    QLabel* potInfoLabel = centralWidget->findChild<QLabel*>("potInfoLabel");  // 查找奖池信息标签
    if (potInfoLabel) {
        potInfoLabel->setText(QString("底 : %1\n总 : %2").arg(entranceFee).arg(pot));  // 更新奖池信息文本
        // 应用缩放因子到字体大小
        QFont font = potInfoLabel->font();
        font.setPointSizeF(16 * scaleFactor);
        potInfoLabel->setFont(font);
    }
    
    // 获取牌桌背景标签
    QLabel* tableBackground = centralWidget->findChild<QLabel*>("tableBackground");  // 查找牌桌背景标签
    if (!tableBackground) {
        return;  // 如果找不到牌桌背景，直接返回
    }
    
    // 获取牌桌中心点坐标
    QPoint tableCenter = tableBackground->mapTo(centralWidget, QPoint(tableWidth/2, tableHeight/2));  // 计算牌桌中心点在主窗口中的坐标
    
    // 获取玩家数量
    int numPlayers = static_cast<int>(players.size());  // 获取当前玩家数量
    
    // 计算玩家位置，确保均匀分布在矩形牌桌周围
    for (int i = 0; i < numPlayers; ++i) {
        const Player& player = players[i];  // 获取当前玩家引用
        QString statusText;  // 玩家状态文本
        switch (player.status) {
            case PlayerStatus::LOOKED: statusText = "已看牌"; break;  // 玩家已看牌
            case PlayerStatus::FOLDED: statusText = "已弃牌"; break;  // 玩家已弃牌
            case PlayerStatus::BLIND: statusText = "蒙牌"; break;    // 玩家蒙牌中
            default: statusText = "等待操作"; break;                // 玩家等待操作
        }
        
        // 使用极坐标计算玩家在牌桌周围的均匀分布位置
        double angle = 2.0 * M_PI * i / numPlayers; // 计算玩家的角度位置（0-2π）
        int infoX = 0, infoY = 0;
        int markerX = 0, markerY = 0;
        int cardX = 0, cardY = 0;
        
        // 创建玩家信息标签
        QString info = QString("%1\n￥%2")  // 格式化玩家信息文本
                      .arg(QString::fromStdString(player.name))  // 添加玩家名称
                      .arg(player.money);  // 添加玩家金额
        
        // 如果是庄家，添加庄家标识
        if (player.isDealer) {
            info += "\n(庄家)";
        }
        
        // 添加玩家状态
        info += "\n" + statusText;
        
        // 创建玩家信息标签
        QLabel* infoLabel = new QLabel(info, centralWidget);  // 创建标签，设置父对象为中央窗口部件
        infoLabel->setAlignment(Qt::AlignCenter);  // 设置文本居中对齐
        
        // 应用缩放因子到字体大小
        QFont font = infoLabel->font();
        font.setPointSizeF(font.pointSizeF() * scaleFactor);
        infoLabel->setFont(font);
        
        // 设置标签样式
        if (static_cast<int>(i) == currentPlayerIndex) {
            infoLabel->setStyleSheet("color: yellow; background-color: rgba(0, 0, 0, 100); padding: 5px; border-radius: 5px;");  // 当前玩家使用黄色文字
        } else {
            infoLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 100); padding: 5px; border-radius: 5px;");  // 其他玩家使用白色文字
        }
        
        // 重新计算玩家在牌桌周围的位置，使用极坐标算法
        // 计算牌桌的半宽和半高
        double halfWidth = tableWidth / 2.0;
        double halfHeight = tableHeight / 2.0;
        
        // 使用极坐标计算玩家在牌桌边缘的位置
        // 计算椭圆方程上的点 (x = a*cos(t), y = b*sin(t))
        double t = angle;
        
        // 计算椭圆上的点坐标，考虑到宽度和高度可能有不同的缩放因子
        // 这确保了玩家位置在横向放大时也能正确定位
        double x = halfWidth * cos(t);
        double y = halfHeight * sin(t);
        
        // 将计算的点坐标转换为屏幕坐标
        markerX = tableCenter.x() + x;
        markerY = tableCenter.y() + y;
        
        // 计算玩家信息和卡牌的位置
        // 根据玩家在牌桌周围的位置计算信息标签和卡牌的位置
        double dx = markerX - tableCenter.x();
        double dy = markerY - tableCenter.y();
        double distance = sqrt(dx * dx + dy * dy);
        
        if (distance > 0) {
            // 归一化方向向量
            double nx = dx / distance;
            double ny = dy / distance;
            
            // 创建临时标签来计算尺寸
            QLabel tempInfoLabel;
            tempInfoLabel.setText(info);
            tempInfoLabel.setAlignment(Qt::AlignCenter);
            tempInfoLabel.adjustSize();
            QFont tempFont = tempInfoLabel.font();
            tempFont.setPointSizeF(tempFont.pointSizeF() * scaleFactor);
            tempInfoLabel.setFont(tempFont);
            tempInfoLabel.adjustSize();
            
            // 计算玩家信息框的宽度和高度
            int infoWidth = tempInfoLabel.width();
            int infoHeight = tempInfoLabel.height();
            
            // 计算信息标签位置（向外偏移）
            // 根据方向向量确定玩家信息框靠近桌子的边
            // 计算从桌子边缘到玩家信息框边缘的距离
            infoX = markerX + nx * playerInfoDistance;
            infoY = markerY + ny * playerInfoDistance;
            
            // 计算卡牌容器的尺寸（3张牌加间距）
            // 使用scaleFactor确保在横向放大时卡牌也能正确缩放
            int cardContainerWidth = (30 * 3 + 2 * 2) * scaleFactor;
            int cardContainerHeight = 45 * scaleFactor;
            
            // 计算卡牌位置（向内偏移）
            // 根据方向向量确定卡牌框靠近桌子的边
            cardX = markerX - nx * cardDistance;
            cardY = markerY - ny * cardDistance;
        } else {
            // 防止除零错误，使用默认位置
            infoX = markerX;
            infoY = markerY - playerInfoDistance;
            cardX = markerX;
            cardY = markerY + cardDistance;
        }
        
        // 调整标签位置，使其居中显示在计算的位置上
        infoLabel->adjustSize();  // 调整标签大小以适应内容
        
        // 计算标签位置，考虑到标签的大小和方向向量
        // 确保玩家信息框的靠近桌子的边与焦点的距离保持为playerInfoDistance
        dx = markerX - tableCenter.x();
        dy = markerY - tableCenter.y();
        distance = sqrt(dx * dx + dy * dy);
        
        if (distance > 0) {
            // 归一化方向向量
            double nx = dx / distance;
            double ny = dy / distance;
            
            // 计算玩家信息框的位置，使其靠近桌子的边与焦点的距离为playerInfoDistance
            infoX = tableCenter.x() + (distance + playerInfoDistance) * nx;
            infoY = tableCenter.y() + (distance + playerInfoDistance) * ny;
        }
        
        infoLabel->move(infoX - infoLabel->width() / 2, infoY - infoLabel->height() / 2);  // 移动标签到计算的位置
        infoLabel->show();  // 显示标签
        
        playerLabels.push_back(infoLabel);  // 将标签添加到玩家标签列表
        
        // 创建不可见的记号点 - 在牌桌边缘
        QLabel* markerLabel = new QLabel(centralWidget);  // 创建标签作为记号点
        markerLabel->setFixedSize(1 * scaleFactor, 1 * scaleFactor);  // 设置记号点大小，应用缩放因子
        markerLabel->setStyleSheet("background-color: transparent;");  // 设置透明背景
        
        // 移动记号点到计算的位置
        markerLabel->move(markerX, markerY);  // 移动记号点到计算的位置
        markerLabel->show();  // 显示记号点（虽然不可见）
        
        playerLabels.push_back(markerLabel);  // 将记号点添加到玩家标签列表
        
        // 创建玩家卡牌容器
        QWidget* cardContainer = new QWidget(centralWidget);  // 创建卡牌容器，设置父对象为中央窗口部件
        cardContainer->setStyleSheet("background-color: transparent;");  // 设置透明背景
        
        // 创建卡牌布局
        QHBoxLayout* cardLayout = new QHBoxLayout(cardContainer);  // 创建水平布局用于卡牌
        cardLayout->setSpacing(2 * scaleFactor);  // 减小牌之间的间距，应用缩放因子
        cardLayout->setContentsMargins(0, 0, 0, 0);  // 移除内边距
        cardLayout->setAlignment(Qt::AlignCenter);  // 设置卡牌居中对齐
        
        // 为每个玩家创建3张牌
        for (int j = 0; j < 3; ++j) {
            QLabel* cardLabel = new QLabel();  // 创建标签用于显示卡牌
            cardLabel->setFixedSize(30 * scaleFactor, 45 * scaleFactor);  // 设置卡牌大小，应用缩放因子
            cardLabel->setStyleSheet("border: none;");  // 移除边框
            
            // 如果是当前玩家且已看牌，或者游戏已结束，显示实际牌面
            if ((static_cast<int>(i) == currentPlayerIndex && player.status == PlayerStatus::LOOKED) || 
                !gameInProgress) {
                if (j < player.cards.size()) {  // 确保玩家有足够的牌
                    // 从卡牌字符串创建Card对象
                    Card card(player.cards[j]);  // 创建卡牌对象
                    // 获取卡牌图片路径
                    QString imagePath = QString("d:/PokerServer/高清全套扑克牌/PNG/%1")
                                       .arg(QString::fromStdString(card.getImageFileName()));  // 构建图片路径
                    
                    // 加载卡牌图片
                    QPixmap cardPixmap(imagePath);  // 加载卡牌图片
                    if (!cardPixmap.isNull()) {  // 如果图片加载成功
                        cardPixmap = cardPixmap.scaled(30 * scaleFactor, 45 * scaleFactor, Qt::KeepAspectRatio, Qt::SmoothTransformation);  // 缩放图片，应用缩放因子
                        cardLabel->setPixmap(cardPixmap);  // 设置图片到标签
                    } else {  // 如果图片加载失败
                        // 如果图片加载失败，显示文本
                        cardLabel->setText(QString::fromStdString(card.toString()));  // 显示卡牌文本
                        cardLabel->setStyleSheet("background-color: white; color: black; border: none;");  // 设置白底黑字
                    }
                }
            } else {  // 如果不是当前玩家或当前玩家未看牌
                // 显示牌背
                QString backImagePath = "d:/PokerServer/高清全套扑克牌/PNG/Background.png";  // 牌背图片路径
                QPixmap backPixmap(backImagePath);  // 加载牌背图片
                if (!backPixmap.isNull()) {  // 如果图片加载成功
                    backPixmap = backPixmap.scaled(30 * scaleFactor, 45 * scaleFactor, Qt::KeepAspectRatio, Qt::SmoothTransformation);  // 缩放图片，应用缩放因子
                    cardLabel->setPixmap(backPixmap);  // 设置图片到标签
                } else {  // 如果图片加载失败
                    // 如果图片加载失败，显示红色背景
                    cardLabel->setStyleSheet("background-color: red; border: none;");  // 设置红色背景
                }
            }
            
            cardLayout->addWidget(cardLabel);  // 将卡牌标签添加到布局
            cardLabels.push_back(cardLabel);  // 将卡牌标签添加到列表
        }
        
        // 调整卡牌容器大小
        cardContainer->adjustSize();  // 调整容器大小以适应内容
        
        // 重新计算卡牌位置，确保卡牌框靠近桌子的边与焦点的距离保持为cardDistance
        dx = markerX - tableCenter.x();
        dy = markerY - tableCenter.y();
        distance = sqrt(dx * dx + dy * dy);
        
        if (distance > 0) {
            // 归一化方向向量
            double nx = dx / distance;
            double ny = dy / distance;
            
            // 计算卡牌容器的位置，使其靠近桌子的边与焦点的距离为cardDistance
            // 卡牌位置在牌桌边缘和中心点之间
            cardX = tableCenter.x() + (distance - cardDistance) * nx;
            cardY = tableCenter.y() + (distance - cardDistance) * ny;
        }
        
        // 移动卡牌容器到计算的位置，使其居中显示
        cardContainer->move(cardX - cardContainer->width() / 2, cardY - cardContainer->height() / 2);  // 移动容器到计算的位置
        cardContainer->show();  // 显示容器
    }
}

// 看牌功能实现 - 当玩家点击看牌按钮时调用
void GoldenFlowerWindow::lookCards() {
    Player& currentPlayer = players[currentPlayerIndex];  // 获取当前玩家引用
    if (currentPlayer.status == PlayerStatus::BLIND) {  // 如果当前玩家处于蒙牌状态
        currentPlayer.status = PlayerStatus::LOOKED;    // 将玩家状态更改为已看牌
        
        // 创建一个对话框来显示牌的图片
        QDialog cardDialog(this);                       // 创建对话框
        cardDialog.setWindowTitle("您的牌");            // 设置对话框标题
        
        // 根据缩放因子调整对话框大小
        int dialogWidth = int(500 * scaleFactor);
        int dialogHeight = int(300 * scaleFactor);
        cardDialog.setFixedSize(dialogWidth, dialogHeight);  // 设置对话框大小，应用缩放因子
        
        QHBoxLayout* layout = new QHBoxLayout(&cardDialog);  // 创建水平布局用于显示卡牌
        layout->setAlignment(Qt::AlignCenter);              // 设置卡牌居中对齐
        layout->setSpacing(int(20 * scaleFactor));          // 设置卡牌间距，应用缩放因子
        
        // 显示玩家的牌
        for (const auto& cardStr : currentPlayer.cards) {    // 遍历玩家手中的每张牌
            Card card(cardStr);                             // 从字符串创建Card对象
            QLabel* cardLabel = new QLabel();               // 创建标签用于显示卡牌
            
            // 根据缩放因子调整卡牌大小
            int cardWidth = int(81 * scaleFactor);
            int cardHeight = int(120 * scaleFactor);
            cardLabel->setFixedSize(cardWidth, cardHeight);  // 设置卡牌大小，应用缩放因子
            cardLabel->setStyleSheet("border: none;");      // 移除边框
            
            // 获取卡牌图片路径
            QString imagePath = QString("d:/PokerServer/高清全套扑克牌/PNG/%1")
                              .arg(QString::fromStdString(card.getImageFileName()));  // 构建图片完整路径
            
            // 加载卡牌图片
            QPixmap cardPixmap(imagePath);                  // 从路径加载图片
            if (!cardPixmap.isNull()) {                     // 如果图片加载成功
                // 缩放图片并保持比例，使用平滑变换提高质量
                cardPixmap = cardPixmap.scaled(cardWidth, cardHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                cardLabel->setPixmap(cardPixmap);           // 设置图片到标签
            } else {                                        // 如果图片加载失败
                // 如果图片加载失败，显示文本
                cardLabel->setText(QString::fromStdString(card.toString()));  // 显示卡牌文本表示
                cardLabel->setStyleSheet("background-color: white; color: black; border: none;");  // 设置白底黑字
                
                // 调整字体大小以适应缩放
                QFont font = cardLabel->font();
                font.setPointSizeF(font.pointSizeF() * scaleFactor);
                cardLabel->setFont(font);
            }
            
            layout->addWidget(cardLabel);                    // 将卡牌标签添加到布局
        }
        
        cardDialog.exec();                                  // 显示对话框并等待用户关闭
        updateUI();                                         // 更新游戏界面
    }
}

// 下注功能实现 - 当玩家点击下注按钮时调用
void GoldenFlowerWindow::placeBet() {
    Player& currentPlayer = players[currentPlayerIndex];  // 获取当前玩家引用
    
    // 如果玩家没有选择看牌，则默认为继续蒙牌状态
    if (currentPlayer.status == PlayerStatus::WAITING) {
        currentPlayer.status = PlayerStatus::BLIND;      // 设置玩家状态为蒙牌
    }
    
    // 获取上家索引（跳过已弃牌的玩家）
    int prevPlayerIndex = (currentPlayerIndex - 1 + players.size()) % players.size();  // 计算上一个玩家的索引
    while (players[prevPlayerIndex].status == PlayerStatus::FOLDED) {  // 如果上家已弃牌，继续向前查找
        prevPlayerIndex = (prevPlayerIndex - 1 + players.size()) % players.size();  // 继续向前查找未弃牌的玩家
    }
    
    // 计算最小下注额
    int minBetAmount;  // 声明最小下注额变量
    
    // 如果是庄家且是第一次下注
    if (currentPlayer.isDealer && currentPlayer.currentBet == 0) {
        minBetAmount = entranceFee;  // 庄家首次下注至少等于入场费
    } else {
        Player& prevPlayer = players[prevPlayerIndex];  // 获取上家引用
        // 根据上家和当前玩家的状态决定最小下注额
        if (prevPlayer.status == PlayerStatus::BLIND && currentPlayer.status == PlayerStatus::LOOKED) {
            // 上家蒙牌，当前玩家看牌，需要下注大于等于上家当轮下注的两倍
            minBetAmount = prevPlayer.currentRoundBet * 2;  // 计算最小下注额为上家下注的两倍
        } else if (prevPlayer.status == PlayerStatus::LOOKED && currentPlayer.status == PlayerStatus::BLIND) {
            // 上家看牌，当前玩家蒙牌，需要下注大于等于上家当轮下注的一半
            minBetAmount = (prevPlayer.currentRoundBet + 1) / 2;  // 计算最小下注额为上家下注的一半（向上取整）
        } else {
            // 其他情况（都看牌或都蒙牌），需要下注大于等于上家当轮下注的金额
            minBetAmount = prevPlayer.currentRoundBet;  // 计算最小下注额为上家下注额
        }
    }
    
    // 弹出对话框让玩家输入下注金额
    bool ok;  // 用于存储对话框结果
    int betAmount = QInputDialog::getInt(this, "下注",  // 创建整数输入对话框
                                       QString("请输入下注金额(最小%1):").arg(minBetAmount),  // 设置提示文本
                                       minBetAmount, minBetAmount, currentPlayer.money, 1, &ok);  // 设置默认值、最小值、最大值和步长
    
    if (ok) {  // 如果玩家确认下注
        currentPlayer.placeBet(betAmount);  // 玩家下注指定金额
        pot += betAmount;                   // 将下注金额添加到奖池
        nextPlayer();                       // 切换到下一个玩家
        updateUI();                         // 更新游戏界面
    }
}

// 弃牌功能实现 - 当玩家点击弃牌按钮时调用
void GoldenFlowerWindow::fold() {
    players[currentPlayerIndex].status = PlayerStatus::FOLDED;  // 将当前玩家状态设置为已弃牌
    nextPlayer();  // 切换到下一个玩家
    updateUI();    // 更新游戏界面
    
    // 检查是否只剩一个玩家（未弃牌）
    int activePlayers = 0;       // 初始化活跃玩家计数器
    int lastActivePlayer = -1;   // 初始化最后一个活跃玩家的索引
    for (size_t i = 0; i < players.size(); ++i) {  // 遍历所有玩家
        if (players[i].status != PlayerStatus::FOLDED) {  // 如果玩家未弃牌
            activePlayers++;                              // 活跃玩家数量加1
            lastActivePlayer = i;                         // 记录该玩家索引
        }
    }
    
    if (activePlayers == 1) {  // 如果只剩一个活跃玩家
        // 游戏结束，最后一个玩家获胜
        players[lastActivePlayer].money += players[lastActivePlayer].currentBet;  // 返还玩家自己的下注
        players[lastActivePlayer].money += pot;  // 将奖池金额给予获胜者
        pot = 0;  // 清空奖池
        
        // 显示游戏结束消息
        QMessageBox::information(this, "游戏结束",
                               QString::fromStdString(players[lastActivePlayer].name + " 获胜!"));
        
        // 重置游戏状态
        gameInProgress = false;                 // 设置游戏未进行中
        startButton->setEnabled(true);          // 启用开始游戏按钮
        lookButton->setEnabled(false);          // 禁用看牌按钮
        betButton->setEnabled(false);           // 禁用下注按钮
        foldButton->setEnabled(false);          // 禁用弃牌按钮
        requestShowdownButton->setEnabled(false);  // 禁用请求开牌按钮
        
        updateUI();  // 更新游戏界面
        return;      // 游戏已结束，直接返回
    }
}

// 切换到下一个玩家 - 跳过已弃牌的玩家
void GoldenFlowerWindow::nextPlayer() {
    do {
        // 循环到下一个玩家，使用取模运算确保索引在有效范围内
        currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
    } while (players[currentPlayerIndex].status == PlayerStatus::FOLDED);  // 如果下一个玩家已弃牌，继续循环
}

// 评估手牌类型 - 判断玩家手中的牌属于哪种牌型
CardType GoldenFlowerWindow::evaluateHand(const vector<string>& cardStrs) {
    vector<Card> cards;  // 创建Card对象数组
    for (const auto& cardStr : cardStrs) {  // 遍历所有牌字符串
        cards.push_back(Card(cardStr));     // 将字符串转换为Card对象并添加到数组
    }
    
    // 检查特殊235牌型（特殊规则：2、3、5不同花色组成的牌）
    if (Card::isSpecial235(cards)) {
        return CardType::SPECIAL_235;  // 返回特殊235牌型
    }
    
    // 检查豹子（三张相同点数的牌）
    if (Card::isThreeOfAKind(cards)) {
        return CardType::THREE_OF_KIND;  // 返回豹子牌型
    }
    
    // 检查是否同花和顺子
    bool isFlush = Card::isFlush(cards);      // 检查是否为同花（三张相同花色的牌）
    bool isStraight = Card::isStraight(cards);  // 检查是否为顺子（三张连续点数的牌）
    
    // 检查同花顺（既是同花又是顺子）
    if (isFlush && isStraight) {
        return CardType::STRAIGHT_FLUSH;  // 返回同花顺牌型
    }
    
    // 检查同花
    if (isFlush) {
        return CardType::FLUSH;  // 返回同花牌型
    }
    
    // 检查顺子
    if (isStraight) {
        return CardType::STRAIGHT;  // 返回顺子牌型
    }
    
    // 检查对子（两张相同点数的牌）
    if (Card::isPair(cards)) {
        return CardType::PAIR;  // 返回对子牌型
    }
    
    // 如果以上都不是，则为高牌
    return CardType::HIGH_CARD;  // 返回高牌牌型
}

// 比较两手牌的大小 - 返回true表示hand1大于hand2，返回false表示hand1小于等于hand2
bool GoldenFlowerWindow::compareHands(const vector<string>& hand1Strs, const vector<string>& hand2Strs) {
    vector<Card> hand1, hand2;  // 创建两个Card对象数组
    
    // 将第一手牌的字符串转换为Card对象
    for (const auto& cardStr : hand1Strs) {
        hand1.push_back(Card(cardStr));  // 将字符串转换为Card对象并添加到数组
    }
    
    // 将第二手牌的字符串转换为Card对象
    for (const auto& cardStr : hand2Strs) {
        hand2.push_back(Card(cardStr));  // 将字符串转换为Card对象并添加到数组
    }
    
    // 评估两手牌的牌型
    CardType type1 = evaluateHand(hand1Strs);  // 评估第一手牌的牌型
    CardType type2 = evaluateHand(hand2Strs);  // 评估第二手牌的牌型
    
    // 如果牌型不同，直接比较牌型大小
    if (type1 != type2) {
        return static_cast<int>(type1) > static_cast<int>(type2);  // 将枚举转换为整数进行比较
    }
    
    // 牌型相同时的比较逻辑
    sort(hand1.begin(), hand1.end());  // 对第一手牌进行排序
    sort(hand2.begin(), hand2.end());  // 对第二手牌进行排序
    
    switch (type1) {  // 根据牌型选择不同的比较策略
        case CardType::THREE_OF_KIND: {
            // 豹子比较第一张牌即可（因为三张牌点数相同，只需比较花色）
            return hand1[0] < hand2[0];  // 比较第一张牌
        }
            
        case CardType::STRAIGHT_FLUSH:
        case CardType::STRAIGHT: {
            // 顺子或同花顺比较最大牌
            if (hand1[2].getRank() == hand2[2].getRank()) {  // 如果最大牌点数相同
                return hand1[2].getSuit() < hand2[2].getSuit();  // 比较最大牌的花色
            }
            return hand1[2] < hand2[2];  // 比较最大牌的点数
        }
            
        case CardType::FLUSH: {
            // 同花从大到小比较每张牌
            for (int i = 2; i >= 0; --i) {  // 从最大牌开始比较
                if (hand1[i].getRank() != hand2[i].getRank()) {  // 如果点数不同
                    return hand1[i] < hand2[i];  // 比较点数大小
                }
            }
            return hand1[2].getSuit() < hand2[2].getSuit();  // 如果所有点数都相同，比较最大牌的花色
        }
            
        case CardType::PAIR: {
            // 找出对子和单牌
            Card pair1, pair2, single1, single2;  // 声明变量用于存储对子和单牌
            for (int i = 0; i < 2; ++i) {  // 遍历前两张牌
                if (hand1[i].getRank() == hand1[i+1].getRank()) {  // 如果找到对子
                    pair1 = hand1[i];  // 记录对子
                    single1 = (i == 0) ? hand1[2] : hand1[0];  // 记录单牌
                }
                if (hand2[i].getRank() == hand2[i+1].getRank()) {  // 如果找到对子
                    pair2 = hand2[i];  // 记录对子
                    single2 = (i == 0) ? hand2[2] : hand2[0];  // 记录单牌
                }
            }
            if (pair1.getRank() != pair2.getRank()) {  // 如果对子点数不同
                return pair1 < pair2;  // 比较对子点数
            }
            if (single1.getRank() != single2.getRank()) {  // 如果单牌点数不同
                return single1 < single2;  // 比较单牌点数
            }
            return pair1.getSuit() < pair2.getSuit();  // 如果对子和单牌点数都相同，比较对子的花色
        }
            
        case CardType::HIGH_CARD: {
            // 从大到小比较每张牌
            for (int i = 2; i >= 0; --i) {  // 从最大牌开始比较
                if (hand1[i].getRank() != hand2[i].getRank()) {  // 如果点数不同
                    return hand1[i] < hand2[i];  // 比较点数大小
                }
            }
            return hand1[2].getSuit() < hand2[2].getSuit();  // 如果所有点数都相同，比较最大牌的花色
        }
            
        default: {
            return false;  // 默认情况下返回false
        }
    }
}

// 开牌功能实现 - 所有玩家同时亮牌并比较牌型大小
void GoldenFlowerWindow::showdown() {
    // 收集所有未弃牌的玩家索引
    vector<int> activePlayers;  // 创建活跃玩家索引数组
    for (size_t i = 0; i < players.size(); ++i) {  // 遍历所有玩家
        if (players[i].status != PlayerStatus::FOLDED) {  // 如果玩家未弃牌
            activePlayers.push_back(i);  // 将玩家索引添加到活跃玩家列表
        }
    }
    
    if (activePlayers.size() < 2) {  // 如果活跃玩家少于2人
        return; // 不足两个玩家，无需比牌
    }
    
    // 找出最大牌的玩家
    int winnerIndex = activePlayers[0];  // 初始假设第一个活跃玩家为赢家
    for (size_t i = 1; i < activePlayers.size(); ++i) {  // 遍历其余活跃玩家
        int currentPlayer = activePlayers[i];  // 获取当前玩家索引
        if (compareHands(players[winnerIndex].cards, players[currentPlayer].cards)) {  // 比较牌型大小
            winnerIndex = currentPlayer;  // 如果当前玩家牌型更大，更新赢家索引
        }
    }
    
    // 创建一个对话框来显示结果
    QDialog resultDialog(this);                      // 创建对话框
    resultDialog.setWindowTitle("开牌结果");         // 设置对话框标题
    resultDialog.setMinimumSize(500, 400);          // 设置对话框最小尺寸
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&resultDialog);  // 创建垂直布局作为主布局
    
    // 添加游戏结果标题
    QLabel* titleLabel = new QLabel("游戏结果");     // 创建标题标签
    titleLabel->setAlignment(Qt::AlignCenter);      // 设置标题居中对齐
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");  // 设置标题样式
    mainLayout->addWidget(titleLabel);              // 将标题添加到主布局
    
    // 为每个玩家创建一个区域显示牌
    for (int playerIndex : activePlayers) {          // 遍历所有活跃玩家
        // 创建玩家分组框，显示玩家名称
        QGroupBox* playerBox = new QGroupBox(QString::fromStdString(players[playerIndex].name));
        QVBoxLayout* playerLayout = new QVBoxLayout(playerBox);  // 创建玩家区域的垂直布局
        
        // 创建卡牌布局
        QHBoxLayout* cardLayout = new QHBoxLayout();  // 创建水平布局用于显示卡牌
        cardLayout->setAlignment(Qt::AlignCenter);    // 设置卡牌居中对齐
        cardLayout->setSpacing(10);                   // 设置卡牌间距
        
        // 显示玩家的牌
        for (const auto& cardStr : players[playerIndex].cards) {  // 遍历玩家的每张牌
            Card card(cardStr);                       // 从字符串创建Card对象
            QLabel* cardLabel = new QLabel();         // 创建标签用于显示卡牌
            cardLabel->setFixedSize(27, 40);          // 设置卡牌大小
            cardLabel->setStyleSheet("border: none;");  // 移除边框
            
            // 获取卡牌图片路径
            QString imagePath = QString("d:/PokerServer/高清全套扑克牌/PNG/%1")
                              .arg(QString::fromStdString(card.getImageFileName()));  // 构建图片路径
            
            // 加载卡牌图片
            QPixmap cardPixmap(imagePath);            // 从路径加载图片
            if (!cardPixmap.isNull()) {               // 如果图片加载成功
                // 缩放图片并保持比例，使用平滑变换提高质量
                cardPixmap = cardPixmap.scaled(27, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                cardLabel->setPixmap(cardPixmap);     // 设置图片到标签
            } else {                                  // 如果图片加载失败
                // 如果图片加载失败，显示文本
                cardLabel->setText(QString::fromStdString(card.toString()));  // 显示卡牌文本表示
                cardLabel->setStyleSheet("background-color: white; color: black; border: none;");  // 设置白底黑字
            }
            
            cardLayout->addWidget(cardLabel);          // 将卡牌标签添加到布局
        }
        
        playerLayout->addLayout(cardLayout);           // 将卡牌布局添加到玩家布局
        
        // 如果是获胜者，添加标记
        if (playerIndex == winnerIndex) {             // 如果当前玩家是获胜者
            QLabel* winnerLabel = new QLabel("获胜者");  // 创建获胜者标签
            winnerLabel->setAlignment(Qt::AlignCenter);  // 设置标签居中对齐
            winnerLabel->setStyleSheet("color: gold; font-weight: bold;");  // 设置金色加粗文本
            playerLayout->addWidget(winnerLabel);     // 将获胜者标签添加到玩家布局
        }
        
        mainLayout->addWidget(playerBox);              // 将玩家分组框添加到主布局
    }
    
    // 添加确认按钮
    QPushButton* okButton = new QPushButton("确定");  // 创建确定按钮
    okButton->setStyleSheet("padding: 8px 16px;");    // 设置按钮样式
    // 连接按钮点击信号到对话框的接受槽，点击按钮时关闭对话框
    connect(okButton, &QPushButton::clicked, &resultDialog, &QDialog::accept);
    mainLayout->addWidget(okButton, 0, Qt::AlignCenter);  // 将按钮添加到主布局并居中
    
    // 将奖池金额给予获胜者
    players[winnerIndex].money += pot;                // 将奖池金额加到获胜者的余额中
    pot = 0;                                         // 清空奖池
    
    // 显示对话框
    resultDialog.exec();                             // 显示对话框并等待用户关闭
    
    // 重置游戏状态
    gameInProgress = false;                          // 设置游戏未进行中
    startButton->setEnabled(true);                   // 启用开始游戏按钮
    lookButton->setEnabled(false);                   // 禁用看牌按钮
    betButton->setEnabled(false);                    // 禁用下注按钮
    foldButton->setEnabled(false);                   // 禁用弃牌按钮
    requestShowdownButton->setEnabled(false);        // 禁用请求开牌按钮
    
    updateUI();                                      // 更新游戏界面
}

// 请求开牌功能实现 - 当玩家点击请求开牌按钮时调用，用于与指定玩家比牌
void GoldenFlowerWindow::requestShowdown() {
    Player& currentPlayer = players[currentPlayerIndex];  // 获取当前玩家引用
    // 检查玩家资金是否足够请求开牌（需要最小下注额的两倍）
    if (currentPlayer.money < minBet * 2) {
        QMessageBox::warning(this, "资金不足", "您的资金不足以请求开牌。");  // 显示警告消息
        return;  // 资金不足，直接返回
    }

    // 获取可选择的玩家列表（排除当前玩家和已弃牌的玩家）
    QStringList playerOptions;  // 创建玩家选项列表
    for (size_t i = 0; i < players.size(); ++i) {  // 遍历所有玩家
        if (i != currentPlayerIndex && players[i].status != PlayerStatus::FOLDED) {  // 如果不是当前玩家且未弃牌
            playerOptions << QString::fromStdString(players[i].name);  // 将玩家名称添加到选项列表
        }
    }
    
    // 检查是否有可选择的玩家
    if (playerOptions.isEmpty()) {  // 如果没有可选择的玩家
        QMessageBox::warning(this, "无效操作", "没有可选择的目标玩家。");  // 显示警告消息
        return;  // 无效操作，直接返回
    }
    
    // 弹出对话框让玩家选择要比牌的对手
    bool ok;  // 用于存储对话框结果
    QString selectedPlayer = QInputDialog::getItem(this, "请求开牌",  // 创建列表选择对话框
                                               "请选择要比牌的玩家:",  // 设置提示文本
                                               playerOptions, 0, false, &ok);  // 设置选项列表和默认选项
    if (!ok) return;  // 如果玩家取消选择，直接返回
    
    // 找到选择的玩家索引
    int targetPlayerIndex = -1;  // 初始化目标玩家索引为-1（无效值）
    for (size_t i = 0; i < players.size(); ++i) {  // 遍历所有玩家
        if (QString::fromStdString(players[i].name) == selectedPlayer) {  // 如果找到选择的玩家
            targetPlayerIndex = i;  // 记录目标玩家索引
            break;  // 找到后跳出循环
        }
    }
    
    if (targetPlayerIndex == -1) return;  // 如果没有找到目标玩家，直接返回

    // 下注最小下注金额的两倍
    int betAmount = minBet * 2;                // 计算下注金额为最小下注额的两倍
    currentPlayer.placeBet(betAmount);        // 当前玩家下注
    pot += betAmount;                         // 将下注金额添加到奖池
    
    // 创建一个对话框来显示比牌结果
    QDialog resultDialog(this);               // 创建对话框
    resultDialog.setWindowTitle("比牌结果");   // 设置对话框标题
    resultDialog.setMinimumSize(500, 400);    // 设置对话框最小尺寸
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&resultDialog);  // 创建垂直布局作为主布局
    
    // 添加比牌结果标题
    QLabel* titleLabel = new QLabel("比牌结果");  // 创建标题标签
    titleLabel->setAlignment(Qt::AlignCenter);   // 设置标题居中对齐
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");  // 设置标题样式
    mainLayout->addWidget(titleLabel);           // 将标题添加到主布局
    
    // 显示当前玩家的牌
    QGroupBox* currentPlayerBox = new QGroupBox(QString::fromStdString(currentPlayer.name));  // 创建当前玩家分组框
    QVBoxLayout* currentPlayerLayout = new QVBoxLayout(currentPlayerBox);  // 创建垂直布局用于当前玩家区域
    
    QHBoxLayout* currentCardLayout = new QHBoxLayout();  // 创建水平布局用于显示当前玩家的卡牌
    currentCardLayout->setAlignment(Qt::AlignCenter);    // 设置卡牌居中对齐
    currentCardLayout->setSpacing(10);                   // 设置卡牌间距
    
    for (const auto& cardStr : currentPlayer.cards) {    // 遍历当前玩家的每张牌
        Card card(cardStr);                             // 从字符串创建Card对象
        QLabel* cardLabel = new QLabel();               // 创建标签用于显示卡牌
        cardLabel->setFixedSize(80, 120);               // 设置卡牌大小
        
        // 获取卡牌图片路径
        QString imagePath = QString("d:/PokerServer/高清全套扑克牌/PNG/%1")
                          .arg(QString::fromStdString(card.getImageFileName()));  // 构建图片路径
        
        // 加载卡牌图片
        QPixmap cardPixmap(imagePath);                  // 从路径加载图片
        if (!cardPixmap.isNull()) {                     // 如果图片加载成功
            // 缩放图片并保持比例，使用平滑变换提高质量
            cardPixmap = cardPixmap.scaled(80, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            cardLabel->setPixmap(cardPixmap);           // 设置图片到标签
        } else {                                        // 如果图片加载失败
            // 如果图片加载失败，显示文本
            cardLabel->setText(QString::fromStdString(card.toString()));  // 显示卡牌文本表示
            cardLabel->setStyleSheet("background-color: white; color: black;");  // 设置白底黑字
        }
        
        currentCardLayout->addWidget(cardLabel);         // 将卡牌标签添加到布局
    }
    
    currentPlayerLayout->addLayout(currentCardLayout);   // 将卡牌布局添加到当前玩家布局
    mainLayout->addWidget(currentPlayerBox);             // 将当前玩家分组框添加到主布局
    
    // 显示目标玩家的牌
    QGroupBox* targetPlayerBox = new QGroupBox(QString::fromStdString(players[targetPlayerIndex].name));  // 创建目标玩家分组框
    QVBoxLayout* targetPlayerLayout = new QVBoxLayout(targetPlayerBox);  // 创建垂直布局用于目标玩家区域
    
    QHBoxLayout* targetCardLayout = new QHBoxLayout();  // 创建水平布局用于显示目标玩家的卡牌
    targetCardLayout->setAlignment(Qt::AlignCenter);    // 设置卡牌居中对齐
    targetCardLayout->setSpacing(10);                   // 设置卡牌间距
    
    for (const auto& cardStr : players[targetPlayerIndex].cards) {  // 遍历目标玩家的每张牌
        Card card(cardStr);                             // 从字符串创建Card对象
        QLabel* cardLabel = new QLabel();               // 创建标签用于显示卡牌
        cardLabel->setFixedSize(80, 120);               // 设置卡牌大小
        
        // 获取卡牌图片路径
        QString imagePath = QString("d:/PokerServer/高清全套扑克牌/PNG/%1")
                          .arg(QString::fromStdString(card.getImageFileName()));  // 构建图片路径
        
        // 加载卡牌图片
        QPixmap cardPixmap(imagePath);                  // 从路径加载图片
        if (!cardPixmap.isNull()) {                     // 如果图片加载成功
            // 缩放图片并保持比例，使用平滑变换提高质量
            cardPixmap = cardPixmap.scaled(80, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            cardLabel->setPixmap(cardPixmap);           // 设置图片到标签
        } else {                                        // 如果图片加载失败
            // 如果图片加载失败，显示文本
            cardLabel->setText(QString::fromStdString(card.toString()));  // 显示卡牌文本表示
            cardLabel->setStyleSheet("background-color: white; color: black;");  // 设置白底黑字
        }
        
        targetCardLayout->addWidget(cardLabel);          // 将卡牌标签添加到布局
    }
    
    targetPlayerLayout->addLayout(targetCardLayout);     // 将卡牌布局添加到目标玩家布局
    mainLayout->addWidget(targetPlayerBox);              // 将目标玩家分组框添加到主布局
    
    // 比较牌型并确定胜者
    bool currentPlayerWins = compareHands(currentPlayer.cards, players[targetPlayerIndex].cards);  // 比较两手牌的大小
    
    // 添加获胜者信息
    QLabel* winnerLabel = new QLabel();                // 创建获胜者信息标签
    winnerLabel->setAlignment(Qt::AlignCenter);       // 设置标签居中对齐
    winnerLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: gold; margin: 10px 0;");  // 设置标签样式
    
    if (currentPlayerWins) {  // 如果当前玩家获胜
        winnerLabel->setText(QString::fromStdString("获胜者: " + currentPlayer.name));  // 设置获胜者文本
        // 在当前玩家的卡牌区域添加获胜标记
        QLabel* winMark = new QLabel("获胜");          // 创建获胜标记标签
        winMark->setAlignment(Qt::AlignCenter);        // 设置标签居中对齐
        winMark->setStyleSheet("color: gold; font-weight: bold;");  // 设置金色加粗文本
        currentPlayerLayout->addWidget(winMark);       // 将获胜标记添加到当前玩家布局
    } else {  // 如果目标玩家获胜
        winnerLabel->setText(QString::fromStdString("获胜者: " + players[targetPlayerIndex].name));  // 设置获胜者文本
        // 在目标玩家的卡牌区域添加获胜标记
        QLabel* winMark = new QLabel("获胜");          // 创建获胜标记标签
        winMark->setAlignment(Qt::AlignCenter);        // 设置标签居中对齐
        winMark->setStyleSheet("color: gold; font-weight: bold;");  // 设置金色加粗文本
        targetPlayerLayout->addWidget(winMark);        // 将获胜标记添加到目标玩家布局
    }
    
    mainLayout->addWidget(winnerLabel);               // 将获胜者信息标签添加到主布局
    
    // 添加确认按钮
    QPushButton* okButton = new QPushButton("确定");  // 创建确定按钮
    okButton->setStyleSheet("padding: 8px 16px;");    // 设置按钮样式
    // 连接按钮点击信号到对话框的接受槽，点击按钮时关闭对话框
    connect(okButton, &QPushButton::clicked, &resultDialog, &QDialog::accept);
    mainLayout->addWidget(okButton, 0, Qt::AlignCenter);  // 将按钮添加到主布局并居中
    
    // 计算当前活跃玩家数量（未弃牌的玩家）
    int activePlayers = 0;                                // 初始化活跃玩家计数器为0
    for (const auto& player : players) {                  // 遍历所有玩家
        if (player.status != PlayerStatus::FOLDED) {      // 如果玩家未弃牌
            activePlayers++;                              // 活跃玩家数量加1
        }
    }
    
    // 处理比牌结果
    QString result = "比牌结果";
    if (currentPlayerWins) {
        // 当前玩家获胜
        result += QString::fromStdString("\n获胜者: " + currentPlayer.name);
        
        // 将目标玩家的下注金额加到当前玩家的下注金额上，而不是余额上
        currentPlayer.currentBet += players[targetPlayerIndex].currentBet;
        players[targetPlayerIndex].status = PlayerStatus::FOLDED;
        
        // 如果比牌前只剩两名玩家，则当前玩家获得所有奖池并结束游戏
        if (activePlayers == 2) {
            // 将赢家的下注金额加到余额上
            currentPlayer.money += currentPlayer.currentBet;
            // 将奖池中的入场费加到赢家余额上
            currentPlayer.money += pot;
            pot = 0;
            
            // 游戏结束
            gameInProgress = false;
            startButton->setEnabled(true);
            lookButton->setEnabled(false);
            betButton->setEnabled(false);
            foldButton->setEnabled(false);
            requestShowdownButton->setEnabled(false);
            
            // 显示对话框
            resultDialog.exec();
            updateUI();
            return; // 游戏已结束，直接返回
        }
        
        QMessageBox::information(this, "比牌结果", result);
    } else {
        // 目标玩家获胜
        result += QString::fromStdString("\n获胜者: " + players[targetPlayerIndex].name);
        
        // 将当前玩家的下注金额加到目标玩家的下注金额上，而不是余额上
        players[targetPlayerIndex].currentBet += currentPlayer.currentBet;
        currentPlayer.status = PlayerStatus::FOLDED;
        
        // 如果比牌前只剩两名玩家，则目标玩家获得所有奖池并结束游戏
        if (activePlayers == 2) {
            // 将赢家的下注金额加到余额上
            players[targetPlayerIndex].money += players[targetPlayerIndex].currentBet;
            // 将奖池中的入场费加到赢家余额上
            players[targetPlayerIndex].money += pot;
            pot = 0;
            
            // 游戏结束
            gameInProgress = false;
            startButton->setEnabled(true);
            lookButton->setEnabled(false);
            betButton->setEnabled(false);
            foldButton->setEnabled(false);
            requestShowdownButton->setEnabled(false);
            
            // 显示对话框
            resultDialog.exec();
            updateUI();
            return; // 游戏已结束，直接返回
        }
        
        QMessageBox::information(this, "比牌结果", result);
    }

    // 比牌后重新检查是否只剩一个玩家
    activePlayers = 0;                                // 重置活跃玩家计数器为0
    int lastActivePlayer = -1;                        // 初始化最后一个活跃玩家的索引为-1（无效值）
    for (size_t i = 0; i < players.size(); ++i) {     // 遍历所有玩家
        if (players[i].status != PlayerStatus::FOLDED) { // 如果玩家未弃牌
            activePlayers++;                          // 活跃玩家数量加1
            lastActivePlayer = i;                     // 记录最后一个活跃玩家的索引
        }
    }
    
    if (activePlayers == 1) {
        // 游戏结束，最后一个玩家获胜
        // 将最后一个玩家的下注金额加到余额上
        players[lastActivePlayer].money += players[lastActivePlayer].currentBet;
        // 将奖池中的入场费加到最后一个玩家余额上
        players[lastActivePlayer].money += pot;
        pot = 0;
        QMessageBox::information(this, "游戏结束",
                               QString::fromStdString(players[lastActivePlayer].name + " 获胜!"));
        gameInProgress = false;
        startButton->setEnabled(true);
        lookButton->setEnabled(false);
        betButton->setEnabled(false);
        foldButton->setEnabled(false);
        requestShowdownButton->setEnabled(false);
        
        updateUI();
        return; // 游戏已结束，直接返回
    } else {
        // 如果当前玩家已弃牌，切换到下一个玩家
        if (currentPlayer.status == PlayerStatus::FOLDED) {
            nextPlayer();
        }
    }
    
    updateUI();
}