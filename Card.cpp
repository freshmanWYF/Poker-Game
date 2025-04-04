//
// Created for Card Implementation
//

#include "Card.h"
#include <sstream>
#include <algorithm>

// 初始化静态成员变量
const map<string, Rank> Card::rankMap = Card::initRankMap();
const map<string, Suit> Card::suitMap = Card::initSuitMap();

// 初始化点数映射表
map<string, Rank> Card::initRankMap() {
    map<string, Rank> m;
    m["2"] = Rank::TWO;
    m["3"] = Rank::THREE;
    m["4"] = Rank::FOUR;
    m["5"] = Rank::FIVE;
    m["6"] = Rank::SIX;
    m["7"] = Rank::SEVEN;
    m["8"] = Rank::EIGHT;
    m["9"] = Rank::NINE;
    m["10"] = Rank::TEN;
    m["J"] = Rank::JACK;
    m["Q"] = Rank::QUEEN;
    m["K"] = Rank::KING;
    m["A"] = Rank::ACE;
    return m;
}

// 初始化花色映射表
map<string, Suit> Card::initSuitMap() {
    map<string, Suit> m;
    m["Hearts"] = Suit::HEARTS;
    m["Spades"] = Suit::SPADES;
    m["Diamonds"] = Suit::DIAMONDS;
    m["Clubs"] = Suit::CLUBS;
    return m;
}

// 构造函数
Card::Card(Rank rank, Suit suit) : rank(rank), suit(suit) {}

// 从字符串构造卡牌
Card::Card(const string& cardStr) {
    size_t pos = cardStr.find(" of ");
    if (pos != string::npos) {
        string rankStr = cardStr.substr(0, pos);
        string suitStr = cardStr.substr(pos + 4);
        
        auto rankIt = rankMap.find(rankStr);
        auto suitIt = suitMap.find(suitStr);
        
        if (rankIt != rankMap.end() && suitIt != suitMap.end()) {
            rank = rankIt->second;
            suit = suitIt->second;
        } else {
            throw invalid_argument("Invalid card string: " + cardStr);
        }
    } else {
        throw invalid_argument("Invalid card string format: " + cardStr);
    }
}

// 获取牌面的字符串表示
string Card::toString() const {
    string rankStr;
    switch (rank) {
        case Rank::ACE: rankStr = "A"; break;
        case Rank::KING: rankStr = "K"; break;
        case Rank::QUEEN: rankStr = "Q"; break;
        case Rank::JACK: rankStr = "J"; break;
        default: rankStr = to_string(static_cast<int>(rank));
    }
    
    string suitStr;
    switch (suit) {
        case Suit::HEARTS: suitStr = "Hearts"; break;
        case Suit::SPADES: suitStr = "Spades"; break;
        case Suit::DIAMONDS: suitStr = "Diamonds"; break;
        case Suit::CLUBS: suitStr = "Clubs"; break;
    }
    
    return rankStr + " of " + suitStr;
}

// 获取图片文件名
string Card::getImageFileName() const {
    string rankStr;
    switch (rank) {
        case Rank::ACE: rankStr = "A"; break;
        case Rank::KING: rankStr = "K"; break;
        case Rank::QUEEN: rankStr = "Q"; break;
        case Rank::JACK: rankStr = "J"; break;
        default: rankStr = to_string(static_cast<int>(rank));
    }
    
    string suitStr;
    switch (suit) {
        case Suit::HEARTS: suitStr = "Heart"; break;
        case Suit::SPADES: suitStr = "Spade"; break;
        case Suit::DIAMONDS: suitStr = "Diamond"; break;
        case Suit::CLUBS: suitStr = "Club"; break;
    }
    
    return suitStr + rankStr + ".png";
}

// 比较运算符重载
bool Card::operator<(const Card& other) const {
    if (rank != other.rank) {
        return static_cast<int>(rank) < static_cast<int>(other.rank);
    }
    return static_cast<int>(suit) > static_cast<int>(other.suit); // 注意这里是反向的，因为HEARTS是最大的
}

bool Card::operator==(const Card& other) const {
    return rank == other.rank && suit == other.suit;
}

// 判断是否为顺子
bool Card::isStraight(const vector<Card>& cards) {
    if (cards.size() != 3) return false;
    
    vector<int> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<int>(card.getRank()));
    }
    sort(ranks.begin(), ranks.end());
    
    // 处理特殊情况：A23
    if (ranks[0] == static_cast<int>(Rank::TWO) &&
        ranks[1] == static_cast<int>(Rank::THREE) &&
        ranks[2] == static_cast<int>(Rank::ACE)) {
        return true;
    }
    
    // 普通顺子
    return ranks[2] - ranks[1] == 1 && ranks[1] - ranks[0] == 1;
}

// 判断是否为同花
bool Card::isFlush(const vector<Card>& cards) {
    if (cards.size() != 3) return false;
    return cards[0].getSuit() == cards[1].getSuit() &&
           cards[1].getSuit() == cards[2].getSuit();
}

// 判断是否为豹子
bool Card::isThreeOfAKind(const vector<Card>& cards) {
    if (cards.size() != 3) return false;
    return cards[0].getRank() == cards[1].getRank() &&
           cards[1].getRank() == cards[2].getRank();
}

// 判断是否为对子
bool Card::isPair(const vector<Card>& cards) {
    if (cards.size() != 3) return false;
    return (cards[0].getRank() == cards[1].getRank() && cards[1].getRank() != cards[2].getRank()) ||
           (cards[1].getRank() == cards[2].getRank() && cards[0].getRank() != cards[1].getRank()) ||
           (cards[0].getRank() == cards[2].getRank() && cards[0].getRank() != cards[1].getRank());
}

// 判断是否为特殊235
bool Card::isSpecial235(const vector<Card>& cards) {
    if (cards.size() != 3) return false;
    
    vector<int> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<int>(card.getRank()));
    }
    sort(ranks.begin(), ranks.end());
    
    return ranks[0] == static_cast<int>(Rank::TWO) &&
           ranks[1] == static_cast<int>(Rank::THREE) &&
           ranks[2] == static_cast<int>(Rank::FIVE);
}