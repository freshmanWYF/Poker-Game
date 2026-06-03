#ifndef CAUTIOUSAI_H
#define CAUTIOUSAI_H

#include "AIStrategy.h"

/**
 * @brief 保守型 AI
 * 特点：很少加注，多跟注/弃牌，避免大亏，适合稳扎稳打
 */
class CautiousAI : public AIStrategy {
public:
    CautiousAI() = default;
    virtual ~CautiousAI() = default;

    Action decide(Player* self, const GameEngine* engine) override;
    float evaluateHandStrength(const Hand& hand) override;

    QString label() const override { return "🐢"; }
    QString name() const override { return "保守型"; }
};

#endif // CAUTIOUSAI_H
