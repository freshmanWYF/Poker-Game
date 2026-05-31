#ifndef HAND_H
#define HAND_H

#include <QtCore/QList>
#include <QtCore/QString>
#include "Card.h"
#include "GameConstants.h"

/**
 * @brief 手牌类 (炸金花核心逻辑)
 * 处理 3 张牌的牌型判定与比较
 */
class Hand {
public:
    Hand();
    Hand(const QList<Card>& cards);

    void setCards(const QList<Card>& cards);
    QList<Card> getCards() const { return m_cards; }

    GameConstants::HandType getType() const { return m_type; }
    QString typeName() const;

    /**
     * @brief 比较两手牌的大小
     * @param h1 第一手牌
     * @param h2 第二手牌
     * @return 1 表示 h1 赢, -1 表示 h2 赢, 0 表示完全相同
     */
    static int compare(const Hand& h1, const Hand& h2);

private:
    void evaluate(); // 判定牌型并排序

    QList<Card> m_cards;
    GameConstants::HandType m_type;
};

#endif // HAND_H
