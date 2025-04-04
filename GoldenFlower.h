//
// Created for GoldenFlower Game
//

#ifndef POKERSERVER_GOLDENFLOWER_H
#define POKERSERVER_GOLDENFLOWER_H

#include <vector>
#include <string>
#include <map>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

using namespace std;

// 牌型枚举
enum class CardType {
    HIGH_CARD,      // 单张
    PAIR,           // 对子
    STRAIGHT,       // 顺子
    FLUSH,          // 同花
    STRAIGHT_FLUSH, // 同花顺
    THREE_OF_KIND,  // 豹子
    SPECIAL_235     // 特殊235
};

// 玩家状态枚举
enum class PlayerStatus {
    WAITING,    // 等待操作
    FOLDED,     // 已弃牌
    BLIND,      // 蒙牌
    LOOKED      // 已看牌
};

// 玩家类
class Player {
public:
    Player(const string& name, int initialMoney);
    
    string name;              // 玩家名称
    vector<string> cards;     // 手牌
    int money;               // 当前金额
    int currentBet;          // 当前总下注
    int currentRoundBet;     // 当前轮次下注
    PlayerStatus status;     // 玩家状态
    bool isDealer;           // 是否为庄家
    
    void reset();            // 重置玩家状态
    void placeBet(int amount); // 下注
    void receiveCard(const string& card); // 接收一张牌
};

// 游戏主窗口类
class GoldenFlowerWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit GoldenFlowerWindow(QWidget *parent = nullptr);
    
    // 事件过滤器，用于处理窗口大小变化事件
    bool eventFilter(QObject *watched, QEvent *event) override;
    
    // 设置距离参数的公共接口，用于调整玩家信息和扑克牌与交点之间的距离
    void setDistanceParameters(int newPlayerInfoDistance, int newCardDistance);

private slots:
    void startNewGame();        // 开始新游戏
    void lookCards();           // 看牌
    void placeBet();            // 下注
    void fold();                // 弃牌
    void showdown();            // 开牌
    void requestShowdown();     // 请求指定玩家开牌

private:
    // UI组件
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QGridLayout *playerLayout;
    QHBoxLayout *buttonLayout;
    vector<QLabel*> playerLabels;
    vector<QLabel*> cardLabels;    // 存储卡牌图像的标签
    QPushButton *startButton;
    QPushButton *lookButton;
    QPushButton *betButton;
    QPushButton *foldButton;
    QPushButton *showdownButton;
    QPushButton *requestShowdownButton;
    
    // 游戏逻辑
    vector<Player> players;
    int currentPlayerIndex;
    int pot;                    // 奖池
    int minBet;                // 最小下注
    int entranceFee;           // 入场费
    bool gameInProgress;
    
    // 布局调整参数
    int tableWidth;           // 牌桌宽度
    int tableHeight;          // 牌桌高度
    int playerInfoDistance;    // 玩家信息距离牌桌边缘的距离
    int cardDistance;          // 卡牌距离牌桌中心的距离
    float scaleFactor;         // 界面缩放因子
    
    void initializeUI();        // 初始化UI
    void updateUI();            // 更新UI
    void setupGame();           // 设置游戏
    void nextPlayer();          // 切换到下一个玩家
    CardType evaluateHand(const vector<string>& cards); // 评估牌型
    bool compareHands(const vector<string>& hand1, const vector<string>& hand2); // 比较牌型
    void adjustLayoutParameters(int newTableWidth, int newTableHeight, int newPlayerInfoDistance, int newCardDistance, float newScaleFactor); // 调整布局参数
};

#endif //POKERSERVER_GOLDENFLOWER_H