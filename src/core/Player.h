#ifndef PLAYER_H
#define PLAYER_H

#include <QtCore/QString>
#include "Hand.h"
#include "GameConstants.h"

class Player {
public:
    Player(int id, QString name, bool isAI = false);

    int getId() const { return m_id; }
    QString getName() const { return m_name; }
    int getChips() const { return m_chips; }
    void addChips(int amount) { m_chips += amount; }
    void removeChips(int amount) { m_chips -= amount; }

    Hand getHand() const { return m_hand; }
    void setHand(const Hand& hand) { m_hand = hand; }

    GameConstants::PlayerStatus getStatus() const { return m_status; }
    void setStatus(GameConstants::PlayerStatus status) { m_status = status; }

    bool isAI() const { return m_isAI; }
    bool isActive() const { return m_status == GameConstants::Active; }
    bool isSeen() const { return m_isSeen; }
    void setSeen(bool seen) { m_isSeen = seen; }

private:
    int m_id;
    QString m_name;
    int m_chips;
    Hand m_hand;
    GameConstants::PlayerStatus m_status;
    bool m_isAI;
    bool m_isSeen = false;
};

#endif // PLAYER_H
