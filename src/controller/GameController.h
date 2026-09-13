#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QElapsedTimer>
#include "../core/GameEngine.h"
#include "../ui/MainWindow.h"
#include "../ai/AIStrategy.h"
#include "../network/NetworkManager.h"
#include "../network/HttpServer.h"
#include "../network/WebSocketServer.h"

class GameController : public QObject {
    Q_OBJECT
public:
    GameController(GameEngine* engine, MainWindow* view);
    virtual ~GameController();

signals:
    // WebSocket 手机端事件（由 lambda 发出，连接到槽）
    void wsClientJoined(int clientId, const QString& name);
    void wsClientAction(int clientId, const QJsonObject& data);
    void wsClientDisconnected(int clientId);

public slots:
    void handleTurnStarted(int playerId);
    void handleGameOver(int winnerId);
    void handlePlayerActed(int playerId, const QString& action, int amount);
    void handleRoundCompleted(int winnerId, int pot);
    void onPlayerCountChanged(int count);
    void processAI();

    void onStartGame();
    void onRestartTable(int startingChips);
    void onFold();
    void onCall();
    void onRaise(int amount);
    void onCompare(int targetId);
    void onSeeCards();

    // 联机槽函数
    void onCreateRoom();
    void onJoinRoom(const QString& address);
    void onNetworkDataReceived(const QJsonObject& data);
    void onClientDataReceived(int clientId, const QJsonObject& data);

    // WebSocket 手机端专用槽
    void onWSClientJoined(int clientId, const QString& name);
    void onWSClientAction(int clientId, const QJsonObject& data);
    void onWSClientDisconnected(int clientId);
    void broadcastWebSocketState();

    // 倒计时
    void startCountdown(int playerId);
    void stopCountdown();
    void onCountdownTick();

private:
    static constexpr int COUNTDOWN_SECONDS = 15;

    int m_countdownPlayerId = -1; // 当前倒计时的玩家 ID
    GameEngine* m_engine;
    MainWindow* m_view;
    QList<AIStrategy*> m_aiStrategies; // 每个 AI 玩家的策略（索引 0 对应玩家 1）
    QTimer* m_aiTimer;
    QTimer* m_countdownTimer;
    int m_countdownSeconds = 0;
    NetworkManager* m_network;
    HttpServer* m_httpServer;
    WebSocketServer* m_webSocketServer;
    int m_myPlayerId = -1; // 在联机模式下自己的 ID
    QList<int> m_wsClientIds;      // WebSocket 客户端 ID 列表
    QList<QString> m_wsClientNames; // WebSocket 客户端名字
    QMap<int, int> m_wsClientPlayerMap; // WebSocket clientId → 玩家索引
    QMap<int, int> m_tcpClientPlayerMap;
    QList<int> m_roundStartingChips;
    QElapsedTimer m_turnClock;
    int m_turnSerial = 0;
    int m_tableSerial = 0;
    bool m_isRemoteClient = false;


    void updateView();
    int claimSeat(const QString& name);
    void replaceWithAI(int playerId);
    bool applyAction(int playerId, const QJsonObject& data);
    bool localCanAct() const;
    QJsonObject stateForPlayer(int playerId) const;
    void sendStateToAll(); // 房主同步状态给所有客户端（旧TCP协议）
    void clearAIStrategies();
    AIStrategy* createRandomStrategy(); // 随机创建一种 AI 策略
    void tryRestoreChips(); // 尝试从存档恢复筹码
};

#endif // GAMECONTROLLER_H
