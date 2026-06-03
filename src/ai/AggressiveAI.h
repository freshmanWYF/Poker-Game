#ifndef AGGRESSIVEAI_H
#define AGGRESSIVEAI_H

#include "AIStrategy.h"

/**
 * @brief 激进型 AI
 * 特点：频繁加注施压，虚张声势多，适合喜欢高风险高回报
 */
class AggressiveAI : public AIStrategy {
public:
    AggressiveAI() = default;
    virtual ~AggressiveAI() = default;

    Action decide(Player* self, const GameEngine* engine) override;
    float evaluateHandStrength(const Hand& hand) override;

    QString label() const override { return "🔥"; }
    QString name() const override { return "激进型"; }
};

#endif // AGGRESSIVEAI_H
