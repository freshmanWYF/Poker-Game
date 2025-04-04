//
// Created for Card representation and comparison
//

#ifndef POKERSERVER_CARD_H
#define POKERSERVER_CARD_H

#include <string>
#include <vector>
#include <map>

using namespace std;

// 花色枚举，按照大小顺序排列：红桃 > 黑桃 > 方块 > 梅花
enum class Suit {
    HEARTS,    // 红桃
    SPADES,    // 黑桃
    DIAMONDS,  // 方块
    CLUBS      // 梅花
};

// 点数枚举
enum class Rank {
    TWO = 2, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN,
    JACK, QUEEN, KING, ACE
};

class Card {
public:
    Card() : rank(Rank::TWO), suit(Suit::CLUBS) {} // 默认构造函数
    Card(Rank rank, Suit suit);
    Card(const string& cardStr); // 从字符串构造卡牌
    
    Rank getRank() const { return rank; }
    Suit getSuit() const { return suit; }
    
    // 获取牌面的字符串表示
    string toString() const;
    
    // 获取图片文件名
    string getImageFileName() const;
    
    // 比较运算符重载
    bool operator<(const Card& other) const;
    bool operator==(const Card& other) const;
    
    // 静态方法：判断是否为顺子
    static bool isStraight(const vector<Card>& cards);
    // 静态方法：判断是否为同花
    static bool isFlush(const vector<Card>& cards);
    // 静态方法：判断是否为豹子
    static bool isThreeOfAKind(const vector<Card>& cards);
    // 静态方法：判断是否为对子
    static bool isPair(const vector<Card>& cards);
    // 静态方法：判断是否为特殊235
    static bool isSpecial235(const vector<Card>& cards);
    
private:
    Rank rank;
    Suit suit;
    
    // 静态映射表：用于字符串转换
    static const map<string, Rank> rankMap;
    static const map<string, Suit> suitMap;
    
    // 初始化静态映射表
    static map<string, Rank> initRankMap();
    static map<string, Suit> initSuitMap();
};

#endif //POKERSERVER_CARD_H