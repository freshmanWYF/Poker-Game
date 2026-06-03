#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtWidgets/QListWidget>
#include "PlayerWidget.h"
#include "../core/GameEngine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow() = default;

    void updateUI(const GameEngine* engine);
    void setActionButtonsEnabled(bool enabled);
    void setLocalPlayerId(int playerId);
    void addConsumptionLog(const QString& msg, bool highlight = false);
    void clearConsumptionLog();
    void reinitAIWidgets(int count);
    void setGameRunning(bool running);
    void revealPlayerTemporarily(int playerId, int durationMs = 2000);
    void showQRCode(const QString& url);
    void setPlayerCountdown(int playerId, int seconds);
    void resetAllCountdowns();

    // 动画接口
    void playDealingAnimation();
    void playChipAnimation(int fromPlayerId, bool toPot);

private:
    const GameEngine* m_lastEngine = nullptr; // 保存引擎引用用于获取玩家列表
    bool m_gameRunning = false;
    bool m_hasStartedGame = false; // 标记游戏是否已经开始过
    int m_localPlayerId = 0;

signals:
    void seeCardsClicked();
    void foldClicked();
    void checkClicked();
    void callClicked();
    void raiseClicked(int amount);
    void compareClicked(int targetPlayerId);
    void startGameClicked();
    void playerCountChanged(int count);

    // 联机信号
    void createRoomClicked();
    void joinRoomClicked(const QString& address);

private:
    QList<PlayerWidget*> m_aiWidgets;
    PlayerWidget* m_humanWidget;

    QLabel* m_potLabel;
    QLabel* m_betLabel;
    QListWidget* m_consumptionList;

    QPushButton* m_btnSee;
    QPushButton* m_btnFold;
    QPushButton* m_btnCall;
    QPushButton* m_btnRaise;
    QPushButton* m_btnCompare;
    QPushButton* m_btnStart;
    QPushButton* m_btnPlayerCount;
    QPushButton* m_btnHost;
    QPushButton* m_btnJoin;
    QPushButton* m_btnPlayAgain;
    QPushButton* m_btnRules;
    QPushButton* m_btnStats;

    void setupUI();
    QHBoxLayout* m_aiAreaLayout;
};

#endif // MAINWINDOW_H
