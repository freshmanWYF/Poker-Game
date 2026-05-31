#include "AdaptiveAI.h"
#include "../utils/Logger.h"
#include <random>
#include <ctime>

float AdaptiveAI::evaluateHandStrength(const Hand& hand) {
    float score = 0.0f;
    auto type = hand.getType();
    auto cards = hand.getCards();

    switch (type) {
        case GameConstants::SPECIAL_235: score = 85.0f; break;
        case GameConstants::Triple: score = 80.0f; break;
        case GameConstants::FlushStraight: score = 70.0f; break;
        case GameConstants::Flush: score = 55.0f; break;
        case GameConstants::Straight: score = 40.0f; break;
        case GameConstants::Pair: score = 20.0f; break;
        case GameConstants::HighCard: score = 0.0f; break;
    }

    if (!cards.isEmpty()) {
        float maxRank = static_cast<float>(cards[0].getRank());
        score += (maxRank / 14.0f) * 20.0f;
    }

    return score;
}

void AdaptiveAI::observeOpponents(const GameEngine* engine, int selfId) {
    // 简单观察：通过当前底注和奖池推断对手行为
    // 底注高说明有人频繁加注，奖池小说明弃牌多
    int currentBet = engine->getCurrentBet();
    int pot = engine->getCurrentPot();

    if (currentBet > GameConstants::MIN_BET * 3) {
        m_opponentRaiseCount++;
        m_totalOpponentActions++;
    }
    if (pot < GameConstants::MIN_BET * engine->getPlayers().size() * 2) {
        m_opponentFoldCount++;
        m_totalOpponentActions++;
    }
    if (m_totalOpponentActions > 0 && m_totalOpponentActions % 5 == 0) {
        // 每 5 轮衰减一次，避免旧数据过度影响
        m_opponentRaiseCount = m_opponentRaiseCount * 3 / 4;
        m_opponentFoldCount = m_opponentFoldCount * 3 / 4;
    }
}

float AdaptiveAI::opponentAggression(const GameEngine* engine, int selfId) const {
    if (m_totalOpponentActions == 0) return 0.5f; // 无数据时中性
    return static_cast<float>(m_opponentRaiseCount) / m_totalOpponentActions;
}

AIStrategy::Action AdaptiveAI::decide(Player* self, const GameEngine* engine) {
    static std::mt19937 g(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(0, 100);
    int randomVal = dist(g);

    // 观察对手
    observeOpponents(engine, self->getId());
    float aggression = opponentAggression(engine, self->getId());

    // 看牌逻辑：中等概率，筹码少时更早看牌
    if (!self->isSeen()) {
        int seeThreshold = (self->getChips() < 300) ? 60 : 40;
        if (randomVal < seeThreshold || engine->getCurrentBet() > GameConstants::MIN_BET * 2) {
            const_cast<GameEngine*>(engine)->seeCards(self->getId());
            Logger::instance().log(QString("AI[%1][适应型] 决定看牌").arg(self->getName()));
        }
    }

    float strength = evaluateHandStrength(self->getHand());
    int currentBet = engine->getCurrentBet();
    int chips = self->getChips();

    // 根据筹码量和对手激进度动态调整策略
    bool lowChips = (chips < 300);
    bool rich = (chips > 1500);
    bool opponentsAggressive = (aggression > 0.6f);
    bool opponentsPassive = (aggression < 0.3f);

    Action action = Fold;
    float multiplier = self->isSeen() ? 1.2f : 1.0f;

    // 筹码少 → 保守模式
    if (lowChips) {
        if (strength >= 70.0f * multiplier) {
            action = (randomVal < 70) ? Raise : Call;
        } else if (strength >= 40.0f * multiplier) {
            action = Call;
        } else {
            action = Fold; // 筹码少时不诈唬
        }
    }
    // 筹码多 + 对手被动 → 激进模式
    else if (rich || opponentsPassive) {
        if (strength >= 45.0f * multiplier) {
            action = (randomVal < 70) ? Raise : Call;
        } else if (strength >= 20.0f * multiplier) {
            action = (randomVal < 50) ? Raise : Call;
        } else {
            int bluff = self->isSeen() ? 5 : 12;
            action = (randomVal < bluff) ? Raise : Fold;
        }
    }
    // 对手激进 → 保守应对
    else if (opponentsAggressive) {
        if (strength >= 60.0f * multiplier) {
            action = (randomVal < 60) ? Raise : Call;
        } else if (strength >= 35.0f * multiplier) {
            action = Call;
        } else {
            action = Fold;
        }
    }
    // 默认策略（类似 SimpleAI）
    else {
        if (strength >= 75.0f * multiplier) {
            action = (randomVal < 80) ? Raise : Call;
        } else if (strength >= 45.0f * multiplier) {
            if (currentBet > chips / 3) {
                action = Fold;
            } else {
                action = (randomVal < 60) ? Call : Raise;
            }
        } else if (strength >= 20.0f * multiplier) {
            action = (currentBet < 100 && randomVal < 40) ? Call : Fold;
        } else {
            int bluff = self->isSeen() ? 2 : 8;
            action = (randomVal < bluff) ? Raise : Fold;
        }
    }

    Logger::instance().log(QString("AI[%1][适应型] %2选择了%3 (对手激进度:%4%)")
        .arg(self->getName())
        .arg(self->isSeen() ? "【已看牌】" : "")
        .arg(action == Raise ? "加注" : (action == Call ? "跟注" : "弃牌"))
        .arg(static_cast<int>(aggression * 100)));

    return action;
}
