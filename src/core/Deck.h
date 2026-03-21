#ifndef DECK_H
#define DECK_H

#include "Card.h"
#include <QtCore/QList>
#include <random>
#include <algorithm>
#include <ctime>

/**
 * @brief 牌堆类
 * 管理 52 张标准扑克牌，提供洗牌和发牌功能
 */
class Deck {
public:
    Deck();

    /**
     * @brief 重置并初始化 52 张牌
     */
    void reset();

    /**
     * @brief 洗牌（使用 Fisher-Yates 算法及高质量随机数引擎）
     */
    void shuffle();

    /**
     * @brief 发出一张牌
     * @return 弹出的牌对象
     */
    Card draw();

    /**
     * @brief 剩余牌数
     */
    int remainingCount() const { return m_cards.size(); }

private:
    QList<Card> m_cards;
};

#endif // DECK_H
