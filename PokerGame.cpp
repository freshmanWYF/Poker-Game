//
// Created by 32098 on 25-2-5.
//
#include "PokerGame.h"
#include <iostream>
using namespace std; // 引入std命名空间

// 构造函数：初始化牌堆并洗牌
GameLogic::GameLogic() {
    initializeDeck();
    shuffleDeck();
}

// 初始化牌堆：生成标准的52张扑克牌
void GameLogic::initializeDeck() {
    vector<string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
    vector<string> ranks = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};

    for (const auto& suit : suits) {
        for (const auto& rank : ranks) {
            deck.push_back(rank + " of " + suit);
        }
    }
}

// 洗牌：随机打乱牌堆
void GameLogic::shuffleDeck() {
    random_device rd; // 随机种子
    mt19937 g(rd());  // 随机数生成器
    shuffle(deck.begin(), deck.end(), g); // 打乱牌堆
}

// 发牌：依次给每个玩家发牌，直到每个玩家手中有三张牌
vector<vector<string>> GameLogic::dealCards(int numPlayers) {
    const int cardsPerPlayer = 3; // 每个玩家发三张牌
    const int maxPlayers = deck.size() / CARDS_PER_PLAYER; // 最大玩家数

    // 异常检测：玩家人数是否合法
    if (numPlayers <= 0) {
        throw invalid_argument("Number of players must be a positive integer.");
    }
    if (numPlayers > maxPlayers) {
        throw invalid_argument("Too many players! Maximum players allowed is " + to_string(maxPlayers) + ".");
    }

    vector<vector<string>> hands(numPlayers); // 每个玩家的手牌

    for (int card = 0; card < cardsPerPlayer; ++card) { // 每人发三张牌
        for (int player = 0; player < numPlayers; ++player) {
            hands[player].push_back(deck.back()); // 从牌堆末尾取牌
            deck.pop_back(); // 移除已发的牌
        }
    }

    return hands;
}

// 打印牌堆（用于调试）
void GameLogic::printDeck() const {
    for (const auto& card : deck) {
        cout << card << endl;
    }
}