#include "Hand.h"
#include <algorithm>

Hand::Hand() : m_type(GameConstants::HighCard) {}

Hand::Hand(const QList<Card>& cards) {
    setCards(cards);
}

void Hand::setCards(const QList<Card>& cards) {
    m_cards = cards;
    evaluate();
}

QString Hand::typeName() const {
    switch (m_type) {
        case GameConstants::Triple:         return "豹子";
        case GameConstants::FlushStraight:  return "顺金";
        case GameConstants::Flush:          return "金花";
        case GameConstants::Straight:       return "顺子";
        case GameConstants::Pair:           return "对子";
        case GameConstants::HighCard:       return "单张";
        default:                            return "未知";
    }
}

void Hand::evaluate() {
    if (m_cards.size() != GameConstants::CARDS_PER_HAND) return;

    // 1. 降序排序，方便后续判定
    std::sort(m_cards.begin(), m_cards.end(), [](const Card& a, const Card& b) {
        return b.getRank() < a.getRank();
    });

    bool isFlush = (m_cards[0].getSuit() == m_cards[1].getSuit() && m_cards[1].getSuit() == m_cards[2].getSuit());
    
    // 顺子特殊处理：Ace 也可以是 A23 中的小顺，但在标准炸金花中，A23 通常作为第二大的顺子或最小顺
    // 这里按标准逻辑处理：QKA(14,13,12), ... A23(14,3,2)
    bool isStraight = false;
    if (m_cards[0].getRank() == m_cards[1].getRank() + 1 && m_cards[1].getRank() == m_cards[2].getRank() + 1) {
        isStraight = true;
    } else if (m_cards[0].getRank() == GameConstants::Ace && m_cards[1].getRank() == GameConstants::Three && m_cards[2].getRank() == GameConstants::Two) {
        // A23 判定为顺子
        isStraight = true;
    }

    // 2. 判定牌型
    // 豹子 (Triple)
    if (m_cards[0].getRank() == m_cards[1].getRank() && m_cards[1].getRank() == m_cards[2].getRank()) {
        m_type = GameConstants::Triple;
    }
    // 顺金 (FlushStraight)
    else if (isFlush && isStraight) {
        m_type = GameConstants::FlushStraight;
    }
    // 金花 (Flush)
    else if (isFlush) {
        m_type = GameConstants::Flush;
    }
    // 顺子 (Straight)
    else if (isStraight) {
        m_type = GameConstants::Straight;
    }
    // 对子 (Pair)
    else if (m_cards[0].getRank() == m_cards[1].getRank() || m_cards[1].getRank() == m_cards[2].getRank()) {
        m_type = GameConstants::Pair;
    }
    // 单张 (HighCard)
    else {
        m_type = GameConstants::HighCard;
    }
}

int Hand::compare(const Hand& h1, const Hand& h2) {
    // 1. 比较牌型
    if (h1.m_type > h2.m_type) return 1;
    if (h1.m_type < h2.m_type) return -1;

    // 2. 牌型相同时比较点数
    switch (h1.m_type) {
        case GameConstants::Triple:
        case GameConstants::Flush:
        case GameConstants::HighCard: {
            // 豹子、金花、单张：从大到小逐张对比
            for (int i = 0; i < 3; ++i) {
                if (h1.m_cards[i].getRank() > h2.m_cards[i].getRank()) return 1;
                if (h1.m_cards[i].getRank() < h2.m_cards[i].getRank()) return -1;
            }
            break;
        }
        case GameConstants::FlushStraight:
        case GameConstants::Straight: {
            // 顺子类：先处理 A23 特殊情况
            auto isA23 = [](const Hand& h) {
                return h.m_cards[0].getRank() == GameConstants::Ace && h.m_cards[1].getRank() == GameConstants::Three;
            };
            bool h1A23 = isA23(h1);
            bool h2A23 = isA23(h2);

            if (h1A23 && !h2A23) return -1; // A23 比 QKA 小
            if (!h1A23 && h2A23) return 1;
            
            // 普通顺子比最大的那张
            if (h1.m_cards[0].getRank() > h2.m_cards[0].getRank()) return 1;
            if (h1.m_cards[0].getRank() < h2.m_cards[0].getRank()) return -1;
            break;
        }
        case GameConstants::Pair: {
            // 对子：先比对子的点数，再比单张的点数
            auto getPairInfo = [](const Hand& h, int& pairRank, int& kickerRank) {
                if (h.m_cards[0].getRank() == h.m_cards[1].getRank()) {
                    pairRank = h.m_cards[0].getRank();
                    kickerRank = h.m_cards[2].getRank();
                } else {
                    pairRank = h.m_cards[1].getRank();
                    kickerRank = h.m_cards[0].getRank();
                }
            };
            int p1, k1, p2, k2;
            getPairInfo(h1, p1, k1);
            getPairInfo(h2, p2, k2);

            if (p1 > p2) return 1;
            if (p1 < p2) return -1;
            if (k1 > k2) return 1;
            if (k1 < k2) return -1;
            break;
        }
    }
    return 0; // 完全相同
}
