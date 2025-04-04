//
// Created by 32098 on 25-2-5.
//

#ifndef POKERSERVER_POKERGAME_H
#define POKERSERVER_POKERGAME_H
#include <vector>
#include <string>
#include <algorithm>
#include <random>
using namespace std;
// 全局变量：每个玩家的手牌数
const int CARDS_PER_PLAYER = 3;

class GameLogic {
public:
    GameLogic();
    void shuffleDeck(); // 洗牌
    vector<vector<string>> dealCards(int numPlayers); // 发牌
    void printDeck() const; // 打印牌堆（用于调试）

private:
    vector<string> deck; // 牌堆
    void initializeDeck(); // 初始化牌堆
};

#endif //POKERSERVER_POKERGAME_H
