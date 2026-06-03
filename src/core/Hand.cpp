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
        case GameConstants::SPECIAL_235:    return "特殊235";
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
    // 特殊235：不同花色的2、3、5（可反杀豹子）
    else {
        QList<int> ranks = { m_cards[0].getRank(), m_cards[1].getRank(), m_cards[2].getRank() };
        std::sort(ranks.begin(), ranks.end());
        bool is235 = (ranks[0] == GameConstants::Two && ranks[1] == GameConstants::Three && ranks[2] == GameConstants::Five);
        bool allDifferentSuits = (m_cards[0].getSuit() != m_cards[1].getSuit()
                               && m_cards[1].getSuit() != m_cards[2].getSuit()
                               && m_cards[0].getSuit() != m_cards[2].getSuit());
        if (is235 && allDifferentSuits) {
            m_type = GameConstants::SPECIAL_235;
        } else {
            m_type = GameConstants::HighCard;
        }
    }
}

int Hand::compare(const Hand& h1, const Hand& h2) {
    // 1. 特殊235 vs 豹子的互杀规则
    if (h1.m_type == GameConstants::SPECIAL_235 && h2.m_type == GameConstants::Triple) return 1;  // 235反杀豹子
    if (h2.m_type == GameConstants::SPECIAL_235 && h1.m_type == GameConstants::Triple) return -1; // 豹子被235反杀

    // 特殊235 输给除豹子外的所有牌型
    if (h1.m_type == GameConstants::SPECIAL_235 && h2.m_type != GameConstants::Triple) return -1;
    if (h2.m_type == GameConstants::SPECIAL_235 && h1.m_type != GameConstants::Triple) return 1;

    // 2. 比较牌型（此时双方都不是 SPECIAL_235 vs Triple 的组合）
    if (h1.m_type > h2.m_type) return 1;
    if (h1.m_type < h2.m_type) return -1;

    // 辅助函数：逐张比较花色
    auto compareSuits = [](const Hand& a, const Hand& b) -> int {
        for (int i = 0; i < 3; ++i) {
            if (a.m_cards[i].getSuit() < b.m_cards[i].getSuit()) return 1;  // Suit值小=花色大
            if (a.m_cards[i].getSuit() > b.m_cards[i].getSuit()) return -1;
        }
        return 0;
    };

    // 3. 牌型相同时比较点数
    switch (h1.m_type) {
        case GameConstants::SPECIAL_235: {
            // 两个235比花色（取最大牌5的花色）
            int s1 = -1, s2 = -1;
            for (int i = 0; i < 3; ++i) {
                if (h1.m_cards[i].getRank() == GameConstants::Five) s1 = h1.m_cards[i].getSuit();
                if (h2.m_cards[i].getRank() == GameConstants::Five) s2 = h2.m_cards[i].getSuit();
            }
            if (s1 < s2) return 1;   // Suit值小=花色大
            if (s1 > s2) return -1;
            return 0;
        }
        case GameConstants::Triple: {
            // 豹子：同点数比较花色
            if (h1.m_cards[0].getRank() > h2.m_cards[0].getRank()) return 1;
            if (h1.m_cards[0].getRank() < h2.m_cards[0].getRank()) return -1;
            return compareSuits(h1, h2);
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

            if (h1.m_cards[0].getRank() > h2.m_cards[0].getRank()) return 1;
            if (h1.m_cards[0].getRank() < h2.m_cards[0].getRank()) return -1;
            // 同点数比较花色（顺金三张同花，顺子比最高牌花色）
            if (h1.m_cards[0].getSuit() < h2.m_cards[0].getSuit()) return 1;
            if (h1.m_cards[0].getSuit() > h2.m_cards[0].getSuit()) return -1;
            break;
        }
        case GameConstants::Flush:
        case GameConstants::HighCard: {
            // 金花、单张：从大到小逐张对比点数，再比花色
            for (int i = 0; i < 3; ++i) {
                if (h1.m_cards[i].getRank() > h2.m_cards[i].getRank()) return 1;
                if (h1.m_cards[i].getRank() < h2.m_cards[i].getRank()) return -1;
            }
            return compareSuits(h1, h2);
        }
        case GameConstants::Pair: {
            // 对子：先比对子的点数，再比单张的点数，最后比花色
            auto getPairInfo = [](const Hand& h, int& pairRank, int& kickerRank, int& kickerSuit) {
                if (h.m_cards[0].getRank() == h.m_cards[1].getRank()) {
                    pairRank = h.m_cards[0].getRank();
                    kickerRank = h.m_cards[2].getRank();
                    kickerSuit = h.m_cards[2].getSuit();
                } else {
                    pairRank = h.m_cards[1].getRank();
                    kickerRank = h.m_cards[0].getRank();
                    kickerSuit = h.m_cards[0].getSuit();
                }
            };
            int p1, k1, ks1, p2, k2, ks2;
            getPairInfo(h1, p1, k1, ks1);
            getPairInfo(h2, p2, k2, ks2);

            if (p1 > p2) return 1;
            if (p1 < p2) return -1;
            if (k1 > k2) return 1;
            if (k1 < k2) return -1;
            // 同对子+同单张，比单张花色
            if (ks1 < ks2) return 1;  // Suit值小=花色大
            if (ks1 > ks2) return -1;
            break;
        }
        default:
            break;
    }
    return 0; // 完全相同
}
