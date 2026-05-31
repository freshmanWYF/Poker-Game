#ifndef CARD_H
#define CARD_H

#include "GameConstants.h"
#include <QtCore/QString>

/**
 * @brief 扑克牌类
 * 包含花色和点数，提供格式化输出功能
 */
class Card {
public:
    Card() = default;
    Card(GameConstants::Suit suit, GameConstants::Rank rank);

    GameConstants::Suit getSuit() const { return m_suit; }
    GameConstants::Rank getRank() const { return m_rank; }

    QString suitName() const;
    QString rankName() const;
    QString toString() const;
    QString imagePath() const;

    // 用于排序对比
    bool operator<(const Card& other) const {
        if (m_rank != other.m_rank) return m_rank < other.m_rank;
        return m_suit < other.m_suit;
    }

private:
    GameConstants::Suit m_suit;
    GameConstants::Rank m_rank;
};

#endif // CARD_H
