#include "Card.h"

Card::Card(GameConstants::Suit suit, GameConstants::Rank rank)
    : m_suit(suit), m_rank(rank) {}

QString Card::suitName() const {
    switch (m_suit) {
        case GameConstants::Spades:   return "♠";
        case GameConstants::Hearts:   return "♥";
        case GameConstants::Clubs:    return "♣";
        case GameConstants::Diamonds: return "♦";
        default: return "?";
    }
}

QString Card::rankName() const {
    if (m_rank <= GameConstants::Ten) return QString::number(m_rank);
    switch (m_rank) {
        case GameConstants::Jack:  return "J";
        case GameConstants::Queen: return "Q";
        case GameConstants::King:  return "K";
        case GameConstants::Ace:   return "A";
        default: return "?";
    }
}

QString Card::toString() const {
    return suitName() + rankName();
}

QString Card::imagePath() const {
    QString suit;
    switch (m_suit) {
        case GameConstants::Spades:   suit = "Spade"; break;
        case GameConstants::Hearts:   suit = "Heart"; break;
        case GameConstants::Clubs:    suit = "Club"; break;
        case GameConstants::Diamonds: suit = "Diamond"; break;
        default: suit = "Unknown";
    }

    QString rank;
    if (m_rank == GameConstants::Ace) rank = "A";
    else if (m_rank == GameConstants::Jack) rank = "J";
    else if (m_rank == GameConstants::Queen) rank = "Q";
    else if (m_rank == GameConstants::King) rank = "K";
    else rank = QString::number(m_rank);

    // 根据 resources.qrc 中的 alias 拼接路径
    // 注意：resources.qrc 中定义了类似 SpadeA.png, Club10.png 等
    return QString(":/poker/%1%2.png").arg(suit).arg(rank);
}
