#include "Deck.h"

Deck::Deck() {
    reset();
}

void Deck::reset() {
    m_cards.clear();
    // 循环所有花色和点数 (2-14)
    for (int s = GameConstants::Spades; s <= GameConstants::Diamonds; ++s) {
        for (int r = GameConstants::Two; r <= GameConstants::Ace; ++r) {
            m_cards.append(Card(static_cast<GameConstants::Suit>(s), static_cast<GameConstants::Rank>(r)));
        }
    }
}

void Deck::shuffle() {
    // 使用当前时间作为随机数种子
    static std::mt19937 g(static_cast<unsigned int>(std::time(nullptr)));
    std::shuffle(m_cards.begin(), m_cards.end(), g);
}

Card Deck::draw() {
    if (m_cards.isEmpty()) return Card(); // 应该在调用前检查 count
    return m_cards.takeFirst();
}
