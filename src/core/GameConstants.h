#ifndef GAMECONSTANTS_H
#define GAMECONSTANTS_H

#include <QtCore/QString>
#include <QtCore/QList>

namespace GameConstants {
    // 扑克牌花色 (0-3)
    enum Suit {
        Spades = 0,   // 黑桃
        Hearts,       // 红桃
        Clubs,        // 草花
        Diamonds      // 方块
    };

    // 扑克牌点数 (2-14, 其中 14 代表 Ace)
    enum Rank {
        Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace
    };

    // 炸金花牌型（按大小排序，豹子最大）
    enum HandType {
        HighCard = 1,    // 单张
        Pair,            // 对子
        Straight,        // 顺子
        Flush,           // 金花 (同花)
        FlushStraight,   // 顺金 (同花顺)
        Triple           // 豹子 (三张相同)
    };

    // 玩家状态
    enum PlayerStatus {
        Waiting,    // 等待开始
        Active,     // 活跃（未弃牌）
        Folded,     // 已弃牌
        Lost,       // 比牌输了
        Winner      // 赢家
    };

    // 游戏阶段 (状态机)
    enum GamePhase {
        Dealing,    // 发牌阶段
        Betting,    // 下注阶段
        Comparing,  // 比牌阶段
        Settlement  // 结算阶段
    };

    const int CARDS_PER_HAND = 3;
    const int CARDS_PER_PLAYER = 3;
    const int INITIAL_CHIPS = 1000;
    const int MIN_BET = 10;
    const int MAX_PLAYERS = 17;
    const int TOTAL_CARDS = 52;
}

#endif // GAMECONSTANTS_H
