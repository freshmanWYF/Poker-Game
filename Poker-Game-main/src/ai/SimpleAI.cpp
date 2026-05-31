#include "SimpleAI.h"
#include "../utils/Logger.h"
#include <random>
#include <ctime>
#include <cmath>

float SimpleAI::evaluateHandStrength(const Hand& hand) {
    float score = 0.0f;
    auto type = hand.getType();
    auto cards = hand.getCards();

    // 1. 基础牌型分 (0-85)
    switch (type) {
        case GameConstants::SPECIAL_235: score = 85.0f; break;   // 特殊235（可反杀豹子）
        case GameConstants::Triple: score = 80.0f; break;        // 豹子
        case GameConstants::FlushStraight: score = 70.0f; break; // 顺金
        case GameConstants::Flush: score = 55.0f; break;         // 金花
        case GameConstants::Straight: score = 40.0f; break;      // 顺子
        case GameConstants::Pair: score = 20.0f; break;          // 对子
        case GameConstants::HighCard: score = 0.0f; break;       // 单张
    }

    // 2. 点数加成分 (0-20)
    // 最大的牌点数越大，分越高。Ace(14)贡献最大。
    if (!cards.isEmpty()) {
        float maxRank = static_cast<float>(cards[0].getRank());
        score += (maxRank / 14.0f) * 20.0f;
    }

    return score;
}

AIStrategy::Action SimpleAI::decide(Player* self, const GameEngine* engine) {
    // 随机引擎
    static std::mt19937 g(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(0, 100);
    int randomVal = dist(g);

    // 如果还没看牌，决定是否看牌
    if (!self->isSeen()) {
        // 策略：如果当前底注较高，或者已经跟了几轮，或者随机概率，则选择看牌
        // 这里简化为：50% 概率在行动前看牌
        if (randomVal < 50 || engine->getCurrentBet() > GameConstants::MIN_BET * 2) {
            const_cast<GameEngine*>(engine)->seeCards(self->getId());
            Logger::instance().log(QString("AI[%1] 决定看牌，后续下注翻倍").arg(self->getName()));
        }
    }

    float strength = evaluateHandStrength(self->getHand());
    int currentBet = engine->getCurrentBet();
    
    QString reason;
    Action action = Fold;

    // 根据是否看牌调整决策阈值
    // 如果看牌了，由于成本翻倍，策略会变得更谨慎
    float thresholdMultiplier = self->isSeen() ? 1.2f : 1.0f;

    // 决策逻辑
    if (strength >= 75.0f * thresholdMultiplier) {
        // 极强牌
        if (randomVal < 80) {
            action = Raise;
            reason = "手牌极强，主动进攻";
        } else {
            action = Call;
            reason = "手牌极强，选择埋伏";
        }
    } else if (strength >= 45.0f * thresholdMultiplier) {
        // 中强牌
        if (currentBet > self->getChips() / 3) { // 看牌后对筹码更敏感
            action = Fold;
            reason = "手牌尚可但下注过大，风险规避";
        } else if (randomVal < 60) {
            action = Call;
            reason = "手牌不错，选择跟注";
        } else {
            action = Raise;
            reason = "手牌不错，尝试加注施压";
        }
    } else if (strength >= 20.0f * thresholdMultiplier) {
        // 普通牌
        if (currentBet < 100 && randomVal < 40) {
            action = Call;
            reason = "普通牌，小注博一下";
        } else {
            action = Fold;
            reason = "牌型一般，不值得冒险";
        }
    } else {
        // 弱牌
        if (randomVal < (self->isSeen() ? 2 : 8)) { // 蒙牌时更容易诈唬
            action = Raise;
            reason = "手牌虽弱，尝试空手套白狼（诈唬）";
        } else {
            action = Fold;
            reason = "牌太烂了，溜了溜了";
        }
    }

    Logger::instance().log(QString("AI[%1] %2选择了%3")
        .arg(self->getName())
        .arg(self->isSeen() ? "【已看牌】" : "")
        .arg(action == Raise ? "加注" : (action == Call ? "跟注" : "弃牌")));

    return action;
}
