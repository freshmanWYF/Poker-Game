#include "AggressiveAI.h"
#include "../utils/Logger.h"
#include <random>
#include <ctime>

float AggressiveAI::evaluateHandStrength(const Hand& hand) {
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

AIStrategy::Action AggressiveAI::decide(Player* self, const GameEngine* engine) {
    static std::mt19937 g(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(0, 100);
    int randomVal = dist(g);

    // 看牌概率低（30%），蒙牌时更敢打
    if (!self->isSeen()) {
        if (randomVal < 30 || engine->getCurrentBet() > GameConstants::MIN_BET * 4) {
            const_cast<GameEngine*>(engine)->seeCards(self->getId());
            Logger::instance().log(QString("AI[%1][激进型] 决定看牌").arg(self->getName()));
        }
    }

    float strength = evaluateHandStrength(self->getHand());
    int currentBet = engine->getCurrentBet();

    Action action = Fold;
    float multiplier = self->isSeen() ? 1.2f : 1.0f;

    if (strength >= 45.0f * multiplier) {
        // 强牌：大部分时候加注
        if (randomVal < 75) {
            action = Raise;
        } else {
            action = Call;
        }
    } else if (strength >= 20.0f * multiplier) {
        // 中等牌：筹码不够时也敢跟
        if (currentBet > self->getChips() / 2) {
            action = Fold;
        } else if (randomVal < 50) {
            action = Raise; // 中等牌也经常加注施压
        } else {
            action = Call;
        }
    } else {
        // 弱牌：高概率诈唬（15% 蒙牌 / 5% 看牌）
        int bluffChance = self->isSeen() ? 5 : 15;
        if (randomVal < bluffChance) {
            action = Raise;
        } else {
            action = Fold;
        }
    }

    Logger::instance().log(QString("AI[%1][激进型] %2选择了%3")
        .arg(self->getName())
        .arg(self->isSeen() ? "【已看牌】" : "")
        .arg(action == Raise ? "加注" : (action == Call ? "跟注" : "弃牌")));

    return action;
}
