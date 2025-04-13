//
// GoldenFlower.cpp - 炸金花游戏实现文件
// 本文件实现了炸金花游戏的核心逻辑和界面交互功能
// 包括玩家管理、牌型评估、下注逻辑、比牌功能、响应式界面布局和网络多人对战
//

#define _USE_MATH_DEFINES  // 定义数学常量，用于计算玩家在牌桌周围的极坐标位置
#include "GoldenFlower.h"  // 包含游戏头文件，定义了游戏的类和枚举
#include "Card.h"          // 包含扑克牌类定义，用于牌面显示和牌型判断
#include <QEvent>          // 包含Qt事件类，用于处理窗口大小变化等事件
#include <algorithm>       // 包含算法库，用于洗牌(random_shuffle)和排序(sort)
#include <cmath>           // 包含数学库，用于计算玩家位置的三角函数和平方根
#include <QPixmap>         // 包含Qt图像处理类，用于加载和显示扑克牌图片
#include <QDir>            // 包含Qt目录操作类，用于查找和访问扑克牌图片文件
#include <QGroupBox>       // 包含Qt分组框类，用于创建玩家信息和卡牌的分组显示

// Player类实现 - 玩家类负责管理玩家的状态、资金和手牌

/**
 * 玩家类构造函数 - 初始化玩家的基本属性
 * @param name 玩家名称，用于界面显示和标识
 * @param initialMoney 玩家初始金额，决定玩家可下注的资金上限
 */
Player::Player(const string& name, int initialMoney) 
    : name(name),                    // 初始化玩家名称
      money(initialMoney),           // 初始化玩家初始金额
      currentBet(0),                 // 初始化当前总下注金额为0
      currentRoundBet(0),            // 初始化当前轮次下注金额为0（用于计算跟注金额）
      status(PlayerStatus::BLIND),   // 初始化玩家状态为蒙牌（未看牌）
      isDealer(false) {}             // 初始化玩家不是庄家（每局随机选择庄家）

/**
 * 重置玩家状态 - 在开始新一局游戏时调用，清空上一局的状态
 * 包括清空手牌、重置下注金额和玩家状态
 */
void Player::reset() {
    cards.clear();                   // 清空玩家手牌，准备接收新牌
    currentBet = 0;                  // 重置当前总下注额为0
    currentRoundBet = 0;             // 重置当前轮次下注额为0
    status = PlayerStatus::BLIND;    // 重置玩家状态为蒙牌（未看牌）
}

/**
 * 玩家下注方法 - 处理玩家下注逻辑
 * @param amount 下注金额，如果超过玩家剩余资金则按最大资金下注
 */
void Player::placeBet(int amount) {
    if (amount > money) amount = money; // 防止超额下注，最多下注玩家所有剩余金额
    money -= amount;                   // 从玩家余额中扣除下注金额
    currentBet += amount;              // 增加玩家当前总下注额（累计所有轮次）
    currentRoundBet = amount;          // 记录当前轮次下注金额（仅当前轮次）
}

/**
 * 玩家接收一张牌 - 在发牌阶段调用
 * @param card 牌的字符串表示，例如 "A of Hearts"
 */
void Player::receiveCard(const string& card) {
    cards.push_back(card);             // 将牌添加到玩家手牌中
}

// GoldenFlowerWindow类实现 - 游戏主窗口类，负责界面显示和游戏逻辑控制

/**
 * 游戏主窗口构造函数 - 初始化游戏界面和基本参数
 * @param parent 父窗口指针，用于Qt窗口层次结构管理
 */
GoldenFlowerWindow::GoldenFlowerWindow(QWidget *parent)
    : QMainWindow(parent),            // 调用父类QMainWindow构造函数
      currentPlayerIndex(0),          // 初始化当前操作玩家的索引为0
      pot(0),                         // 初始化奖池金额为0（不包括入场费）
      minBet(10),                     // 初始化最小下注额为10（可在游戏开始时修改）
      entranceFee(10),                // 初始化入场费为10（每局游戏开始时收取）
      gameInProgress(false),          // 初始化游戏状态为未进行中
      tableWidth(550),                // 初始化牌桌宽度为550像素
      tableHeight(350),               // 初始化牌桌高度为350像素
      playerInfoDistance(50),         // 初始化玩家信息距离牌桌边缘的距离为50像素
      cardDistance(50),               // 初始化卡牌距离牌桌中心的距离为50像素
      scaleFactor(1.0),               // 初始化界面缩放因子为1.0（原始大小）
      maxPlayers(4) {                 // 初始化最大玩家数为4
    initializeUI();                   // 调用初始化用户界面方法，创建并设置UI组件
    
    // 安装事件过滤器，用于捕获窗口大小变化事件和卡牌悬停事件
    this->installEventFilter(this);
}

/**
 * 游戏主窗口析构函数 - 清理资源
 * 负责释放所有动态分配的资源
 */
GoldenFlowerWindow::~GoldenFlowerWindow() {
    // 清理玩家标签和卡牌标签
    for (auto label : playerLabels) {
        if (label) {
            delete label;
        }
    }
    playerLabels.clear();
    
    for (auto label : cardLabels) {
        if (label) {
            delete label;
        }
    }
    cardLabels.clear();
}

/**
 * 事件过滤器 - 处理窗口大小变化事件，实现响应式布局
 * 当窗口大小变化时，计算新的缩放因子并调整所有UI元素的大小和位置
 * @param watched 被监视的对象指针
 * @param event 事件对象指针
 * @return 是否处理了事件，true表示事件已处理，false表示继续传递事件
 */
bool GoldenFlowerWindow::eventFilter(QObject *watched, QEvent *event) {
    // 处理窗口大小变化事件
    if (watched == this) {
        // 处理窗口大小变化事件（包括最大化、还原和手动调整大小）
        if (event->type() == QEvent::Resize || event->type() == QEvent::WindowStateChange) {
            // 获取调整后的窗口大小
            QSize newSize = this->size();
            
            // 计算宽度和高度的缩放因子 - 基于初始窗口大小900x700
            float widthScale = newSize.width() / 900.0f;  // 宽度缩放比例
            float heightScale = newSize.height() / 700.0f; // 高度缩放比例
            
            // 使用宽度和高度的缩放因子中较小的一个，确保元素比例一致
            // 这样在窗口最大化或拉伸时也能保持正确的比例，避免变形
            float newScaleFactor = qMin(widthScale, heightScale);
            
            // 计算新的牌桌尺寸，基于初始尺寸和新的缩放因子
            // 使用widthScale和heightScale分别计算宽度和高度，保持牌桌的椭圆形状
            int newTableWidth = int(550 * widthScale);   // 新牌桌宽度
            int newTableHeight = int(350 * heightScale);  // 新牌桌高度
            
            // 计算新的玩家信息和扑克牌与交点距离
            // 应用缩放因子确保距离随窗口大小等比例变化
            // 这样当窗口缩放时，玩家信息和扑克牌与牌桌边缘的相对位置关系保持不变
            int newPlayerInfoDistance = int(50 * newScaleFactor); // 新玩家信息距离
            int newCardDistance = int(50 * newScaleFactor);       // 新卡牌距离
            
            // 调用布局参数调整方法，更新UI元素大小和位置
            adjustLayoutParameters(newTableWidth, newTableHeight, newPlayerInfoDistance, newCardDistance, newScaleFactor);
        }
    }
    
    // 处理卡牌悬停事件
    QLabel* cardLabel = qobject_cast<QLabel*>(watched);
    if (cardLabel && cardLabel->objectName().startsWith("playerCard_")) {
        // 只处理已看牌的玩家的卡牌
        int playerIndex = cardLabel->objectName().split("_")[1].toInt();
        int cardIndex = cardLabel->objectName().split("_")[2].toInt();
        
        if (playerIndex < players.size() && 
            (players[playerIndex].status == PlayerStatus::LOOKED || !gameInProgress)) {
            
            if (event->type() == QEvent::Enter) {
                // 鼠标进入卡牌区域 - 放大卡牌
                QEnterEvent* enterEvent = static_cast<QEnterEvent*>(event);
                
                // 获取原始图片
                QPixmap originalPixmap = cardLabel->pixmap(Qt::ReturnByValue);
                if (!originalPixmap.isNull()) {
                    // 计算放大后的尺寸（放大1.5倍）
                    int enlargedWidth = originalPixmap.width() * 1.5;
                    int enlargedHeight = originalPixmap.height() * 1.5;
                    
                    // 缩放图片
                    QPixmap enlargedPixmap = originalPixmap.scaled(
                        enlargedWidth, enlargedHeight, 
                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    
                    // 保存原始图片和尺寸（用于恢复）
                    cardLabel->setProperty("originalPixmap", originalPixmap);
                    cardLabel->setProperty("originalSize", cardLabel->size());
                    
                    // 设置放大的图片
                    cardLabel->setPixmap(enlargedPixmap);
                    cardLabel->setFixedSize(enlargedWidth, enlargedHeight);
                    
                    // 调整位置，使放大后的卡牌居中于原位置
                    QPoint originalPos = cardLabel->pos();
                    QSize originalSize = cardLabel->property("originalSize").toSize();
                    int xOffset = (enlargedWidth - originalSize.width()) / 2;
                    int yOffset = (enlargedHeight - originalSize.height()) / 2;
                    cardLabel->move(originalPos.x() - xOffset, originalPos.y() - yOffset);
                    
                    // 将卡牌置于顶层
                    cardLabel->raise();
                }
                return true;
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开卡牌区域 - 恢复原始大小
                QPixmap originalPixmap = cardLabel->property("originalPixmap").value<QPixmap>();
                QSize originalSize = cardLabel->property("originalSize").toSize();
                
                if (!originalPixmap.isNull() && !originalSize.isEmpty()) {
                    // 恢复原始图片和尺寸
                    cardLabel->setPixmap(originalPixmap);
                    cardLabel->setFixedSize(originalSize);
                    
                    // 恢复原始位置
                    QPoint currentPos = cardLabel->pos();
                    QSize currentSize = cardLabel->size();
                    int xOffset = (currentSize.width() - originalSize.width()) / 2;
                    int yOffset = (currentSize.height() - originalSize.height()) / 2;
                    cardLabel->move(currentPos.x() + xOffset, currentPos.y() + yOffset);
                }
                return true;
            }
        }
    }
    
    // 对于未处理的事件，调用父类方法继续处理
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

/**
 * 设置距离参数 - 公共接口，允许外部调用修改玩家信息和扑克牌与交点之间的距离
 * @param newPlayerInfoDistance 新的玩家信息距离牌桌边缘的距离（像素）
 * @param newCardDistance 新的卡牌距离牌桌中心的距离（像素）
 */
void GoldenFlowerWindow::setDistanceParameters(int newPlayerInfoDistance, int newCardDistance) {
    playerInfoDistance = newPlayerInfoDistance;  // 更新玩家信息距离
    cardDistance = newCardDistance;             // 更新卡牌距离
    
    // 如果游戏正在进行中，立即更新UI以反映新的布局参数
    if (gameInProgress) {
        updateUI();  // 调用UI更新方法
    }
}

/**
 * 调整布局参数 - 根据窗口大小变化调整所有UI元素的大小和位置
 * @param newTableWidth 新的牌桌宽度（像素）
 * @param newTableHeight 新的牌桌高度（像素）
 * @param newPlayerInfoDistance 新的玩家信息距离（像素）
 * @param newCardDistance 新的卡牌距离（像素）
 * @param newScaleFactor 新的缩放因子，用于等比例缩放所有UI元素
 */
void GoldenFlowerWindow::adjustLayoutParameters(int newTableWidth, int newTableHeight, int newPlayerInfoDistance, int newCardDistance, float newScaleFactor) {
    // 更新所有布局参数
    tableWidth = newTableWidth;                // 更新牌桌宽度
    tableHeight = newTableHeight;              // 更新牌桌高度
    playerInfoDistance = newPlayerInfoDistance; // 更新玩家信息距离
    cardDistance = newCardDistance;            // 更新卡牌距离
    scaleFactor = newScaleFactor;              // 更新缩放因子
    
    // 更新牌桌背景大小和样式
    QLabel* tableBackground = centralWidget->findChild<QLabel*>("tableBackground");
    if (tableBackground) {
        // 设置牌桌新的固定大小
        tableBackground->setFixedSize(tableWidth, tableHeight);
        
        // 计算边框宽度，随缩放因子等比例变化
        int borderWidth = int(8 * scaleFactor);
        
        // 计算边框半径，确保在横向放大时保持正确的圆角效果
        // 取高度和宽度一半中的较小值，避免椭圆变形
        int borderRadius = qMin(tableHeight / 2, tableWidth / 2);
        
        // 构建牌桌样式字符串，包括背景色、边框和圆角
        QString tableStyle = QString("background-color: #0a6e31; border: %1px solid #8B4513; border-radius: %2px;")
                            .arg(borderWidth)
                            .arg(borderRadius);
        
        // 应用新样式到牌桌背景
        tableBackground->setStyleSheet(tableStyle);
    }
    
    // 调整所有按钮的字体大小和样式
    QFont buttonFont;
    buttonFont.setPointSizeF(10 * scaleFactor); // 按钮字体大小随缩放因子变化
    
    // 查找并更新所有按钮
    QList<QPushButton*> buttons = centralWidget->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        // 设置按钮字体
        button->setFont(buttonFont);
        
        // 计算按钮内边距，随缩放因子变化
        int paddingH = int(16 * scaleFactor); // 水平内边距
        int paddingV = int(8 * scaleFactor);  // 垂直内边距
        int borderRadius = int(5 * scaleFactor); // 边框圆角半径
        
        // 构建按钮样式字符串
        QString buttonStyle = QString("QPushButton { background-color: #FFA500; color: white; "
                                   "font-weight: bold; padding: %1px %2px; border-radius: %3px; } "
                                   "QPushButton:hover { background-color: #FF8C00; } "
                                   "QPushButton:disabled { background-color: #A9A9A9; }")
                                .arg(paddingV)
                                .arg(paddingH)
                                .arg(borderRadius);
        
        // 应用样式到按钮
        button->setStyleSheet(buttonStyle);
    }
    
    // 调整主布局的边距和间距
    int margin = int(20 * scaleFactor); // 边距随缩放因子变化
    int spacing = int(10 * scaleFactor); // 间距随缩放因子变化
    
    mainLayout->setContentsMargins(margin, margin, margin, margin);
    mainLayout->setSpacing(spacing);
    
    // 调整按钮布局的间距（如果存在）
    if (buttonLayout) {
        buttonLayout->setSpacing(spacing);
    }
    
    // 调整奖池信息标签的字体大小
    QLabel* potInfoLabel = centralWidget->findChild<QLabel*>("potInfoLabel");
    if (potInfoLabel) {
        QFont font = potInfoLabel->font();
        font.setPointSizeF(16 * scaleFactor); // 字体大小随缩放因子变化
        potInfoLabel->setFont(font);
    }
    
    // 如果游戏正在进行中，更新UI以反映新的布局参数
    if (gameInProgress) {
        updateUI();  // 调用UI更新方法，重新布局所有游戏元素
    }
}

/**
 * 显示比牌结果对话框 - 用于处理两名玩家之间的比牌结果显示
 * @param player1Index 第一个玩家的索引
 * @param player2Index 第二个玩家的索引
 * 对话框显示两名玩家的牌面，并标记获胜者
 */
void GoldenFlowerWindow::showComparisonDialog(int player1Index, int player2Index) {
    Player& player1 = players[player1Index];  // 获取第一个玩家的引用
    Player& player2 = players[player2Index];  // 获取第二个玩家的引用
    
    // 创建模态对话框来显示比牌结果
    QDialog resultDialog(this);               // 创建对话框，设置父窗口为当前窗口
    resultDialog.setWindowTitle("比牌结果");   // 设置对话框标题
    resultDialog.setMinimumSize(500, 400);    // 设置对话框最小尺寸，确保能完整显示牌面
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&resultDialog);  // 创建垂直布局作为主布局
    
    // 添加比牌结果标题标签
    QLabel* titleLabel = new QLabel("比牌结果");  // 创建标题标签
    titleLabel->setAlignment(Qt::AlignCenter);   // 设置标题文本居中对齐
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");  // 设置标题字体大小、粗细和下边距
    mainLayout->addWidget(titleLabel);           // 将标题标签添加到主布局
    
    // 显示第一个玩家的牌面区域
    QGroupBox* player1Box = new QGroupBox(QString::fromStdString(player1.name));  // 创建分组框，标题为玩家名称
    QVBoxLayout* player1Layout = new QVBoxLayout(player1Box);  // 创建垂直布局用于组织玩家区域内的元素
    
    // 创建水平布局用于显示第一个玩家的卡牌
    QHBoxLayout* player1CardLayout = new QHBoxLayout();  // 水平排列卡牌
    player1CardLayout->setAlignment(Qt::AlignCenter);    // 设置卡牌在布局中居中对齐
    player1CardLayout->setSpacing(10);                   // 设置卡牌之间的间距为10像素
    
    for (const auto& cardStr : player1.cards) {    // 遍历第一个玩家的每张牌
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
        
        player1CardLayout->addWidget(cardLabel);         // 将卡牌标签添加到布局
    }
    
    player1Layout->addLayout(player1CardLayout);   // 将卡牌布局添加到第一个玩家布局
    mainLayout->addWidget(player1Box);             // 将第一个玩家分组框添加到主布局
    
    // 显示第二个玩家的牌
    QGroupBox* player2Box = new QGroupBox(QString::fromStdString(player2.name));  // 创建第二个玩家分组框
    QVBoxLayout* player2Layout = new QVBoxLayout(player2Box);  // 创建垂直布局用于第二个玩家区域
    
    QHBoxLayout* player2CardLayout = new QHBoxLayout();  // 创建水平布局用于显示第二个玩家的卡牌
    player2CardLayout->setAlignment(Qt::AlignCenter);    // 设置卡牌居中对齐
    player2CardLayout->setSpacing(10);                   // 设置卡牌间距
    
    for (const auto& cardStr : player2.cards) {  // 遍历第二个玩家的每张牌
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
        
        player2CardLayout->addWidget(cardLabel);          // 将卡牌标签添加到布局
    }
    
    player2Layout->addLayout(player2CardLayout);     // 将卡牌布局添加到第二个玩家布局
    mainLayout->addWidget(player2Box);              // 将第二个玩家分组框添加到主布局
    
    // 比较牌型并确定胜者
    bool player1Wins = compareHands(player1.cards, player2.cards);  // 比较两手牌的大小
    
    // 添加获胜者信息
    QLabel* winnerLabel = new QLabel();                // 创建获胜者信息标签
    winnerLabel->setAlignment(Qt::AlignCenter);       // 设置标签居中对齐
    winnerLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: gold; margin: 10px 0;");  // 设置标签样式
    
    if (player1Wins) {  // 如果第一个玩家获胜
        winnerLabel->setText(QString::fromStdString("获胜者: " + player1.name));  // 设置获胜者文本
        // 在第一个玩家的卡牌区域添加获胜标记
        QLabel* winMark = new QLabel("获胜");          // 创建获胜标记标签
        winMark->setAlignment(Qt::AlignCenter);        // 设置标签居中对齐
        winMark->setStyleSheet("color: gold; font-weight: bold;");  // 设置金色加粗文本
        player1Layout->addWidget(winMark);       // 将获胜标记添加到第一个玩家布局
    } else {  // 如果第二个玩家获胜
        winnerLabel->setText(QString::fromStdString("获胜者: " + player2.name));  // 设置获胜者文本
        // 在第二个玩家的卡牌区域添加获胜标记
        QLabel* winMark = new QLabel("获胜");          // 创建获胜标记标签
        winMark->setAlignment(Qt::AlignCenter);        // 设置标签居中对齐
        winMark->setStyleSheet("color: gold; font-weight: bold;");  // 设置金色加粗文本
        player2Layout->addWidget(winMark);        // 将获胜标记添加到第二个玩家布局
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
    if (player1Wins) {
        // 第一个玩家获胜
        // 将第二个玩家的下注金额加到第一个玩家的下注金额上，而不是余额上
        player1.currentBet += player2.currentBet;
        player2.status = PlayerStatus::FOLDED;
        
        // 如果比牌前只剩两名玩家，则第一个玩家获得所有奖池并结束游戏
        if (activePlayers == 2) {
            // 将赢家的下注金额加到余额上
            player1.money += player1.currentBet;
            // 将奖池中的入场费加到赢家余额上
            player1.money += pot;
            pot = 0;
            
            // 游戏结束
            gameInProgress = false;
            startButton->setText(players.empty() ? "开始游戏" : "继续游戏");  // 根据是否有玩家设置按钮文本
            startButton->setEnabled(true);                   // 启用开始游戏按钮
            lookButton->setEnabled(false);                   // 禁用看牌按钮
            betButton->setEnabled(false);                    // 禁用下注按钮
            foldButton->setEnabled(false);                   // 禁用弃牌按钮
            requestShowdownButton->setEnabled(false);        // 禁用请求开牌按钮
            
            // 显示游戏结束消息
            QMessageBox::information(this, "游戏结束",
                                   QString::fromStdString(player1.name + " 获胜!"));
        }
    } else {
        // 第二个玩家获胜
        // 将第一个玩家的下注金额加到第二个玩家的下注金额上，而不是余额上
        player2.currentBet += player1.currentBet;
        player1.status = PlayerStatus::FOLDED;
        
        // 如果比牌前只剩两名玩家，则第二个玩家获得所有奖池并结束游戏
        if (activePlayers == 2) {
            // 将赢家的下注金额加到余额上
            player2.money += player2.currentBet;
            // 将奖池中的入场费加到赢家余额上
            player2.money += pot;
            pot = 0;
            
            // 游戏结束
            gameInProgress = false;
            startButton->setText(players.empty() ? "开始游戏" : "继续游戏");  // 根据是否有玩家设置按钮文本
            startButton->setEnabled(true);                   // 启用开始游戏按钮
            lookButton->setEnabled(false);                   // 禁用看牌按钮
            betButton->setEnabled(false);                    // 禁用下注按钮
            foldButton->setEnabled(false);                   // 禁用弃牌按钮
            requestShowdownButton->setEnabled(false);        // 禁用请求开牌按钮
            
            // 显示游戏结束消息
            QMessageBox::information(this, "游戏结束",
                                   QString::fromStdString(player2.name + " 获胜!"));
        }
    }
    
    // 显示对话框
    resultDialog.exec();                             // 显示对话框并等待用户关闭
    
    // 更新UI
    updateUI();                                      // 更新游戏界面
}

// 开始新游戏
void GoldenFlowerWindow::startNewGame() {
    // 检查是否是第一次开始游戏
    bool isFirstGame = players.empty();
    
    if (isFirstGame) {
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
    } else {
        // 如果不是第一次开始游戏，保留玩家余额，只重置其他状态
        // 弹出对话框获取入场费，默认为当前入场费，范围1-最小玩家余额的十分之一
        int minPlayerMoney = INT_MAX;
        for (const auto& player : players) {
            minPlayerMoney = min(minPlayerMoney, player.money);
        }
        
        bool ok;
        entranceFee = QInputDialog::getInt(this, "新游戏", "请输入入场费:",
                                         entranceFee, 1, minPlayerMoney / 10, 1, &ok);
        if (!ok) return;  // 用户取消，直接返回
    }
    
    // 随机选择庄家
    int dealerIndex = rand() % players.size();  // 随机生成庄家索引
    
    // 重置所有玩家的庄家状态
    for (auto& player : players) {
        player.isDealer = false;
    }
    
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
/**
 * 设置游戏 - 初始化游戏环境，包括清理上一局数据、创建牌组、洗牌和发牌
 * 在每局游戏开始时调用，为所有玩家准备新的牌局
 */
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
    // 定义卡牌相关变量，确保在所有代码路径中都可用
    int cardWidth = 30 * scaleFactor;  // 单张卡牌宽度
    int cardHeight = 45 * scaleFactor; // 单张卡牌高度
    int cardSpacing = 2 * scaleFactor; // 卡牌间距
    
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
    
    // 清理现有卡牌容器
    QList<QWidget*> containers = centralWidget->findChildren<QWidget*>();
    for (QWidget* container : containers) {
        if (container->objectName().startsWith("cardContainer")) {
            delete container;
        }
    }
    
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
    
    // 获取牌桌中心点坐标 - 确保在窗口大小变化后重新计算
    // 使用mapTo确保在窗口最大化或其他尺寸变化时获取正确的中心点坐标
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
        
        // 使用极坐标算法计算玩家在椭圆形牌桌边缘的位置
        // 牌桌是一个椭圆，使用参数方程 x = a*cos(t), y = b*sin(t) 计算边缘点
        double halfWidth = tableWidth / 2.0;  // 椭圆半长轴
        double halfHeight = tableHeight / 2.0; // 椭圆半短轴
        double t = angle; // 参数t等于玩家的角度位置
        
        // 计算椭圆边缘上的点坐标
        // 这里考虑了窗口缩放时宽高比可能变化的情况，确保玩家位置始终在椭圆边缘
        double x = halfWidth * cos(t);
        double y = halfHeight * sin(t);
        
        // 将计算的点坐标转换为屏幕坐标作为交点位置
        markerX = tableCenter.x() + x;
        markerY = tableCenter.y() + y;
        
        // 计算玩家信息和卡牌的位置，确保它们位于从桌子中心到边缘的延长线上
        
        // 计算玩家信息和卡牌的位置
        // 根据玩家在牌桌周围的位置计算信息标签和卡牌的位置
        // 确保玩家信息和扑克牌始终位于从桌子中心到边缘的延长线上
        // 实时计算从桌子中心到边缘交点的方向向量
        double dx = markerX - tableCenter.x();
        double dy = markerY - tableCenter.y();
        double distance = sqrt(dx * dx + dy * dy);
        
        if (distance > 0.0) {
            // 归一化方向向量 - 这是从桌子中心到边缘的延长线方向
            // 这个方向向量在窗口大小变化时保持不变，确保玩家信息和扑克牌始终位于同一直线上
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
            
            // 计算玩家信息标签位置 - 位于交点外侧（远离中心）
            // playerInfoDistance已通过缩放因子调整，确保窗口大小变化时保持相对距离
            infoX = markerX + nx * playerInfoDistance;
            infoY = markerY + ny * playerInfoDistance;
            
            // 计算卡牌容器的尺寸 - 3张牌加间距
            // 所有尺寸都与缩放因子成正比，确保窗口大小变化时保持正确比例
            int cardContainerWidth = (cardWidth * 3 + cardSpacing * 2); // 3张牌的总宽度加间距
            int cardContainerHeight = cardHeight; // 容器高度等于卡牌高度
            
            // 计算卡牌容器位置 - 位于交点内侧（靠近中心）
            // 使用负方向确保卡牌位于交点与中心点之间
            // cardDistance已通过缩放因子调整，确保窗口大小变化时保持相对距离
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
        
        // 移动玩家信息标签到计算的位置，确保居中显示
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
        cardContainer->setObjectName(QString("cardContainer_%1").arg(i));  // 设置唯一的对象名
        cardContainer->setStyleSheet("background-color: transparent;");  // 设置透明背景
        
        // 创建卡牌布局
        QHBoxLayout* cardLayout = new QHBoxLayout(cardContainer);  // 创建水平布局用于卡牌
        cardLayout->setSpacing(cardSpacing);  // 使用前面计算好的卡牌间距
        cardLayout->setContentsMargins(0, 0, 0, 0);  // 移除内边距
        cardLayout->setAlignment(Qt::AlignCenter);  // 设置卡牌居中对齐
        
        // 为每个玩家创建3张牌
        for (int j = 0; j < 3; ++j) {
            QLabel* cardLabel = new QLabel();  // 创建标签用于显示卡牌
            cardLabel->setFixedSize(cardWidth, cardHeight);  // 使用前面计算好的卡牌尺寸
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
                        cardPixmap = cardPixmap.scaled(cardWidth, cardHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);  // 使用前面计算好的卡牌尺寸缩放图片
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
                    backPixmap = backPixmap.scaled(cardWidth, cardHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);  // 使用前面计算好的卡牌尺寸缩放图片
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
        
        // 移动卡牌容器到计算的位置，确保居中显示
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
        int dialogWidth = int(350 * scaleFactor);
        int dialogHeight = int(200 * scaleFactor);
        cardDialog.setFixedSize(dialogWidth, dialogHeight);  // 设置对话框大小，应用缩放因子
        
        QHBoxLayout* layout = new QHBoxLayout(&cardDialog);  // 创建水平布局用于显示卡牌
        layout->setAlignment(Qt::AlignCenter);              // 设置卡牌居中对齐
        layout->setSpacing(int(8 * scaleFactor));          // 设置卡牌间距，应用缩放因子
        
        // 显示玩家的牌
        for (const auto& cardStr : currentPlayer.cards) {    // 遍历玩家手中的每张牌
            Card card(cardStr);                             // 从字符串创建Card对象
            QLabel* cardLabel = new QLabel();               // 创建标签用于显示卡牌
            
            // 根据缩放因子调整卡牌大小
            int cardWidth = int(100 * scaleFactor);
            int cardHeight = int(140 * scaleFactor);
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
        
        // 重新实现最小下注金额计算逻辑
        // 根据玩家状态(蒙牌/看牌)和上家下注金额确定最小下注额
        if (currentPlayer.status == PlayerStatus::BLIND) {
            // 如果玩家是蒙牌，则最小下注金额是上家下注金额的一半
            minBetAmount = (prevPlayer.currentRoundBet + 1) / 2;  // 向上取整
        } else if (currentPlayer.status == PlayerStatus::LOOKED) {
            // 如果玩家是看牌，则最小下注金额是上家下注金额
            minBetAmount = prevPlayer.currentRoundBet;
        } else {
            // 其他情况，保持原有逻辑
            minBetAmount = prevPlayer.currentRoundBet;
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
        
        // 检查是否只剩两个玩家，且当前玩家请求了开牌
        int activePlayers = 0;
        int otherPlayerIndex = -1;
        for (size_t i = 0; i < players.size(); ++i) {
            if (players[i].status != PlayerStatus::FOLDED) {
                activePlayers++;
                if (i != currentPlayerIndex) {
                    otherPlayerIndex = i;
                }
            }
        }
        
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

/**
 * 评估手牌类型 - 判断玩家手中的牌属于哪种牌型
 * 炸金花游戏中的牌型从高到低依次为：豹子、同花顺、同花、顺子、对子、高牌
 * 另有特殊牌型235（不同花色的2、3、5组合）
 * @param cardStrs 玩家手牌的字符串表示数组
 * @return 判断出的牌型枚举值
 */
CardType GoldenFlowerWindow::evaluateHand(const vector<string>& cardStrs) {
    vector<Card> cards;  // 创建Card对象数组
    for (const auto& cardStr : cardStrs) {  // 遍历所有牌字符串
        cards.push_back(Card(cardStr));     // 将字符串转换为Card对象并添加到数组
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
    
    // 检查特殊235牌型（特殊规则：2、3、5不同花色组成的牌）
    if (Card::isSpecial235(cards)) {
        return CardType::SPECIAL_235;  // 返回特殊235牌型
    }
    
    // 如果以上都不是，则为高牌
    return CardType::HIGH_CARD;  // 返回高牌牌型
}

/**
 * 比较两手牌的大小 - 实现炸金花游戏的牌型大小比较逻辑
 * 按照牌型大小顺序：豹子 > 同花顺 > 同花 > 顺子 > 对子 > 单张
 * 特殊规则：235组合可以反杀豹子，但输给其他所有牌型
 * @param hand1Strs 第一手牌的字符串表示
 * @param hand2Strs 第二手牌的字符串表示
 * @return true表示hand1大于hand2，false表示hand1小于等于hand2
 */
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
    
    // 处理特殊235反杀豹子的规则
    if (type1 == CardType::SPECIAL_235 && type2 == CardType::THREE_OF_KIND) {
        return true;  // 235反杀豹子
    }
    if (type2 == CardType::SPECIAL_235 && type1 == CardType::THREE_OF_KIND) {
        return false;  // 豹子被235反杀
    }
    
    // 特殊235输给除豹子外的任何牌型
    if (type1 == CardType::SPECIAL_235 && type2 != CardType::THREE_OF_KIND) {
        return false;
    }
    if (type2 == CardType::SPECIAL_235 && type1 != CardType::THREE_OF_KIND) {
        return true;
    }
    
    // 如果牌型不同，直接比较牌型大小
    if (type1 != type2) {
        return static_cast<int>(type1) > static_cast<int>(type2);  // 将枚举转换为整数进行比较
    }
    
    // 牌型相同时的比较逻辑
    sort(hand1.begin(), hand1.end());  // 对第一手牌进行排序
    sort(hand2.begin(), hand2.end());  // 对第二手牌进行排序
    
    switch (type1) {  // 根据牌型选择不同的比较策略
        case CardType::THREE_OF_KIND: {
            // 豹子比较点数
            if (hand1[0].getRank() != hand2[0].getRank()) {
                return static_cast<int>(hand1[0].getRank()) > static_cast<int>(hand2[0].getRank());
            }
            // 点数相同比较花色（红桃 > 黑桃 > 方块 > 梅花）
            return static_cast<int>(hand1[0].getSuit()) < static_cast<int>(hand2[0].getSuit());
        }
            
        case CardType::STRAIGHT_FLUSH:
        case CardType::STRAIGHT: {
            // 顺子或同花顺比较最大牌
            if (hand1[2].getRank() != hand2[2].getRank()) {  // 如果最大牌点数不同
                return static_cast<int>(hand1[2].getRank()) > static_cast<int>(hand2[2].getRank());
            }
            // 点数相同比较花色（红桃 > 黑桃 > 方块 > 梅花）
            return static_cast<int>(hand1[2].getSuit()) < static_cast<int>(hand2[2].getSuit());
        }
            
        case CardType::FLUSH: {
            // 同花从大到小比较每张牌
            for (int i = 2; i >= 0; --i) {  // 从最大牌开始比较
                if (hand1[i].getRank() != hand2[i].getRank()) {  // 如果点数不同
                    return static_cast<int>(hand1[i].getRank()) > static_cast<int>(hand2[i].getRank());
                }
            }
            // 所有点数都相同，比较最大牌的花色（红桃 > 黑桃 > 方块 > 梅花）
            return static_cast<int>(hand1[2].getSuit()) < static_cast<int>(hand2[2].getSuit());
        }
            
        case CardType::PAIR: {
            // 找出对子和单牌
            Card pair1, pair2, single1, single2;  // 声明变量用于存储对子和单牌
            for (int i = 0; i < 2; ++i) {  // 遍历前两张牌
                if (hand1[i].getRank() == hand1[i+1].getRank()) {  // 如果找到对子
                    pair1 = hand1[i];  // 记录对子
                    single1 = (i == 0) ? hand1[2] : hand1[0];  // 记录单牌
                    break;
                }
            }
            for (int i = 0; i < 2; ++i) {  // 遍历前两张牌
                if (hand2[i].getRank() == hand2[i+1].getRank()) {  // 如果找到对子
                    pair2 = hand2[i];  // 记录对子
                    single2 = (i == 0) ? hand2[2] : hand2[0];  // 记录单牌
                    break;
                }
            }
            if (pair1.getRank() != pair2.getRank()) {  // 如果对子点数不同
                return static_cast<int>(pair1.getRank()) > static_cast<int>(pair2.getRank());
            }
            if (single1.getRank() != single2.getRank()) {  // 如果单牌点数不同
                return static_cast<int>(single1.getRank()) > static_cast<int>(single2.getRank());
            }
            // 对子和单牌点数都相同，比较对子的花色（红桃 > 黑桃 > 方块 > 梅花）
            return static_cast<int>(pair1.getSuit()) < static_cast<int>(pair2.getSuit());
        }
            
        case CardType::HIGH_CARD: {
            // 从大到小比较每张牌
            for (int i = 2; i >= 0; --i) {  // 从最大牌开始比较
                if (hand1[i].getRank() != hand2[i].getRank()) {  // 如果点数不同
                    return static_cast<int>(hand1[i].getRank()) > static_cast<int>(hand2[i].getRank());
                }
            }
            // 所有点数都相同，比较最大牌的花色（红桃 > 黑桃 > 方块 > 梅花）
            return static_cast<int>(hand1[2].getSuit()) < static_cast<int>(hand2[2].getSuit());
        }
            
        default: {
            return false;  // 默认情况下返回false
        }
    }
}

/**
 * 开牌功能实现 - 所有玩家同时亮牌并比较牌型大小
 * 在游戏结束阶段调用，收集所有未弃牌玩家的牌面，比较牌型大小
 * 确定获胜者并分配奖池，然后重置游戏状态准备下一局
 */
// 删除了showdown函数 - 全部一起开牌功能已移除

/**
 * 请求开牌功能实现 - 当前玩家与指定玩家进行比牌
 * 当玩家点击请求开牌按钮时调用，允许玩家选择一名对手进行比牌
 * 比牌需要额外下注，输者自动弃牌，如果只剩最后一名玩家则游戏结束
 */
void GoldenFlowerWindow::requestShowdown() {
    Player& currentPlayer = players[currentPlayerIndex];  // 获取当前玩家引用
    
    // 计算当前最小下注金额
    int minBetAmount = 0;
    
    // 获取上家索引（跳过已弃牌的玩家）
    int prevPlayerIndex = (currentPlayerIndex - 1 + players.size()) % players.size();
    while (players[prevPlayerIndex].status == PlayerStatus::FOLDED) {
        prevPlayerIndex = (prevPlayerIndex - 1 + players.size()) % players.size();
    }
    
    // 计算最小下注额
    // 如果是庄家且是第一次下注
    if (currentPlayer.isDealer && currentPlayer.currentBet == 0) {
        // 如果玩家一开始就选择开牌
        if (currentPlayer.status == PlayerStatus::LOOKED) {
            // 看牌状态，开牌金额是入场费的两倍
            minBetAmount = entranceFee * 2;
        } else {
            // 蒙牌状态，开牌金额是入场费
            minBetAmount = entranceFee;
        }
    } else {
        Player& prevPlayer = players[prevPlayerIndex];
        // 根据上家和当前玩家的状态决定最小下注额
        if (prevPlayer.status == PlayerStatus::BLIND && currentPlayer.status == PlayerStatus::LOOKED) {
            // 上家蒙牌，当前玩家看牌，需要下注大于等于上家当轮下注的两倍
            minBetAmount = prevPlayer.currentRoundBet * 2;
        } else if (prevPlayer.status == PlayerStatus::LOOKED && currentPlayer.status == PlayerStatus::BLIND) {
            // 上家看牌，当前玩家蒙牌，需要下注大于等于上家当轮下注的一半
            minBetAmount = (prevPlayer.currentRoundBet + 1) / 2;  // 向上取整
        } else {
            // 其他情况（都看牌或都蒙牌），需要下注大于等于上家当轮下注的金额
            minBetAmount = prevPlayer.currentRoundBet;
        }
        
        // 请求开牌需要下注最小下注金额的两倍
        minBetAmount = minBetAmount * 2;
    }
    
    // 设置开牌所需的下注金额
    int betAmount = minBetAmount;
    
    // 检查玩家资金是否足够请求开牌
    if (currentPlayer.money < betAmount) {
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
    
    // 检查是否只剩两个玩家
    int activePlayers = 0;
    for (const auto& player : players) {
        if (player.status != PlayerStatus::FOLDED) {
            activePlayers++;
        }
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

    // 如果只剩两个玩家，添加确认界面
    if (activePlayers == 2) {
        // 创建确认对话框
        QMessageBox confirmBox(this);
        confirmBox.setWindowTitle("确认开牌");
        confirmBox.setText(QString::fromStdString(currentPlayer.name) + " 请求与 " + 
                          QString::fromStdString(players[targetPlayerIndex].name) + " 开牌。");
        confirmBox.setInformativeText(QString("确定要开牌吗？需要下注 %1 金额").arg(betAmount));
        confirmBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        confirmBox.setDefaultButton(QMessageBox::Ok);
        
        // 显示确认对话框并获取结果
        int ret = confirmBox.exec();
        
        // 如果用户点击确定，进行比牌
        if (ret == QMessageBox::Ok) {
            // 下注请求开牌所需金额
            currentPlayer.placeBet(betAmount);        // 当前玩家下注
            pot += betAmount;                         // 将下注金额添加到奖池
            
            // 比较两手牌的大小
            bool currentPlayerWins = compareHands(currentPlayer.cards, players[targetPlayerIndex].cards);
            
            // 显示比牌结果对话框
            showComparisonDialog(currentPlayerIndex, targetPlayerIndex);
            
            // 输者弃牌
            if (currentPlayerWins) {
                // 目标玩家输，设置为弃牌状态
                players[targetPlayerIndex].status = PlayerStatus::FOLDED;
            } else {
                // 当前玩家输，设置为弃牌状态
                currentPlayer.status = PlayerStatus::FOLDED;
            }
            
            // 检查是否只剩一个活跃玩家，如果是则游戏结束
            int remainingPlayers = 0;
            int lastActivePlayer = -1;
            for (size_t i = 0; i < players.size(); ++i) {
                if (players[i].status != PlayerStatus::FOLDED) {
                    remainingPlayers++;
                    lastActivePlayer = i;
                }
            }
            
            if (remainingPlayers == 1) {
                // 游戏结束，最后一个玩家获胜
                // 将奖池金额给予获胜者，还要加上其余人的入场费
                players[lastActivePlayer].money += pot;  // 将奖池金额给予获胜者
                
                // 计算入场费总额（除了获胜者以外的所有玩家的入场费）
                int totalEntranceFee = 0;
                for (size_t i = 0; i < players.size(); ++i) {
                    if (i != lastActivePlayer) {
                        totalEntranceFee += entranceFee;
                    }
                }
                
                // 将入场费总额加给获胜者
                players[lastActivePlayer].money += totalEntranceFee;
                
                pot = 0;  // 清空奖池
                
                QMessageBox::information(this, "游戏结束",
                                      QString::fromStdString(players[lastActivePlayer].name + " 获胜!"));
                gameInProgress = false;
                startButton->setEnabled(true);
                lookButton->setEnabled(false);
                betButton->setEnabled(false);
                foldButton->setEnabled(false);
                requestShowdownButton->setEnabled(false);
                
                updateUI();
                return;  // 游戏已结束，直接返回
            }
            
            // 轮到下家说话下注
            nextPlayer();
            updateUI();
        }
    } else {
        // 下注请求开牌所需金额
        currentPlayer.placeBet(betAmount);        // 当前玩家下注
        pot += betAmount;                         // 将下注金额添加到奖池
        
        // 比较两手牌的大小
        bool currentPlayerWins = compareHands(currentPlayer.cards, players[targetPlayerIndex].cards);
        
        // 显示比牌结果对话框
        showComparisonDialog(currentPlayerIndex, targetPlayerIndex);
        
        // 输者弃牌
        if (currentPlayerWins) {
            // 目标玩家输，设置为弃牌状态
            players[targetPlayerIndex].status = PlayerStatus::FOLDED;
        } else {
            // 当前玩家输，设置为弃牌状态
            currentPlayer.status = PlayerStatus::FOLDED;
        }
        
        // 检查是否只剩一个活跃玩家，如果是则游戏结束
        int remainingPlayers = 0;
        int lastActivePlayer = -1;
        for (size_t i = 0; i < players.size(); ++i) {
            if (players[i].status != PlayerStatus::FOLDED) {
                remainingPlayers++;
                lastActivePlayer = i;
            }
        }
        
        if (remainingPlayers == 1) {
            // 游戏结束，最后一个玩家获胜
            // 将奖池金额给予获胜者，还要加上其余人的入场费
            players[lastActivePlayer].money += pot;  // 将奖池金额给予获胜者
            
            // 计算入场费总额（除了获胜者以外的所有玩家的入场费）
            int totalEntranceFee = 0;
            for (size_t i = 0; i < players.size(); ++i) {
                if (i != lastActivePlayer) {
                    totalEntranceFee += entranceFee;
                }
            }
            
            // 将入场费总额加给获胜者
            players[lastActivePlayer].money += totalEntranceFee;
            
            pot = 0;  // 清空奖池
            
            QMessageBox::information(this, "游戏结束",
                                  QString::fromStdString(players[lastActivePlayer].name + " 获胜!"));
            gameInProgress = false;
            startButton->setEnabled(true);
            lookButton->setEnabled(false);
            betButton->setEnabled(false);
            foldButton->setEnabled(false);
            requestShowdownButton->setEnabled(false);
            
            updateUI();
            return;  // 游戏已结束，直接返回
        }
        
        // 轮到下家说话下注
        nextPlayer();
        updateUI();
    }
}

/**
 * 初始化网络UI组件 - 创建和设置网络游戏相关的UI元素
 * 包括创建房间、加入房间、离开房间和邀请玩家等按钮
 */



/**
 * 结束游戏 - 处理游戏结束逻辑
 * @param winnerIndex 获胜玩家的索引
 */
void GoldenFlowerWindow::endGame(int winnerIndex) {
    // 检查索引是否有效
    if (winnerIndex < 0 || winnerIndex >= players.size()) {
        return;
    }
    
    // 将奖池金额给予获胜者
    players[winnerIndex].money += pot;
    
    // 计算入场费总额（除了获胜者以外的所有玩家的入场费）
    int totalEntranceFee = 0;
    for (size_t i = 0; i < players.size(); ++i) {
        if (i != winnerIndex) {
            totalEntranceFee += entranceFee;
        }
    }
    
    // 将入场费总额加给获胜者
    players[winnerIndex].money += totalEntranceFee;
    
    pot = 0;  // 清空奖池
    
    // 显示游戏结束消息
    QMessageBox::information(this, "游戏结束",
                          QString::fromStdString(players[winnerIndex].name + " 获胜!"));
    
    // 更新游戏状态
    gameInProgress = false;
    startButton->setEnabled(true);
    lookButton->setEnabled(false);
    betButton->setEnabled(false);
    foldButton->setEnabled(false);
    requestShowdownButton->setEnabled(false);
    
    
    // 更新UI
    updateUI();
}
