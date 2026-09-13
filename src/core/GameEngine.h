#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include "Deck.h"
#include "Player.h"
#include "GameConstants.h"

class GameEngine : public QObject {
    Q_OBJECT
public:
    GameEngine();
    ~GameEngine() override;

    void addPlayer(const QString& name, bool isAI = false);
    void resetPlayers();
    bool resetTable(int startingChips);
    bool startGame();
    void nextTurn();
    bool seeCards(int playerId);
    bool fold(int playerId);
    bool bet(int playerId, int amount);
    bool compare(int playerId1, int playerId2);
    bool canAct(int playerId) const;
    int calculateRequiredBet(int playerId) const;
    void applyNetworkSnapshot(int pot, int bet, int turnIndex, GameConstants::GamePhase phase);

    // Getters
    const QList<Player*>& getPlayers() const { return m_players; }
    int getStartingChips() const { return m_startingChips; }
    int getCurrentPot() const { return m_currentPot; }
    int getCurrentBet() const { return m_currentBet; }
    int getCurrentTurnIndex() const { return m_currentTurnIndex; }
    GameConstants::GamePhase getCurrentPhase() const { return m_currentPhase; }

signals:
    void actionRejected(int playerId, const QString& reason);
    void gameStateChanged();
    void turnStarted(int playerId);
    void playerActed(int playerId, const QString& action, int amount);
    void potChanged(int newPot);
    void betChanged(int newBet);
    void phaseChanged(GameConstants::GamePhase newPhase);
    void gameOver(int winnerId);
    void compareResult(int winnerId, int loserId, const QString& winnerType, const QString& loserType);
    void roundCompleted(int winnerId, int pot); // 一局结束（含奖池金额）

private:
    Deck m_deck;
    int m_startingChips = GameConstants::INITIAL_CHIPS;
    QList<Player*> m_players;
    int m_currentPot;
    int m_currentBet;
    int m_currentTurnIndex;
    bool m_isGameRunning;
    GameConstants::GamePhase m_currentPhase;

    void setPhase(GameConstants::GamePhase phase);
    int nextActivePlayer(int currentIndex);
    bool checkGameOver();
};

#endif // GAMEENGINE_H
