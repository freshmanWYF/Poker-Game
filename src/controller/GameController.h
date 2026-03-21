#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QList>
#include <QtCore/QString>
#include "../core/GameEngine.h"
#include "../ui/MainWindow.h"
#include "../ai/SimpleAI.h"
#include "../network/NetworkManager.h"

class GameController : public QObject {
    Q_OBJECT
public:
    GameController(GameEngine* engine, MainWindow* view);
    virtual ~GameController() = default;

public slots:
    void handleTurnStarted(int playerId);
    void handleTurnEnded(int playerId);
    void handleGameOver(int winnerId);
    void handlePhaseChanged(GameConstants::GamePhase phase);
    void handlePlayerActed(int playerId, const QString& action, int amount);
    void onPlayerCountChanged(int count);
    void processAI();

    void onStartGame();
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

private:
    GameEngine* m_engine;
    MainWindow* m_view;
    SimpleAI m_ai;
    QTimer* m_aiTimer;
    NetworkManager* m_network;
    int m_myPlayerId = -1; // 在联机模式下自己的 ID
    QList<int> m_joinClientIds;
    QList<QString> m_joinClientNames;
    QString m_hostName;

    void updateView();
    void sendStateToAll(); // 房主同步状态给所有客户端
};

#endif // GAMECONTROLLER_H
