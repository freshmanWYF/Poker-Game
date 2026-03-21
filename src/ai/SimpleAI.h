#ifndef SIMPLEAI_H
#define SIMPLEAI_H

#include "AIStrategy.h"
#include "../core/Player.h"
#include "../core/GameEngine.h"

class SimpleAI : public AIStrategy {
public:
    SimpleAI() = default;
    virtual ~SimpleAI() = default;
    Action decide(Player* self, const GameEngine* engine) override;
    float evaluateHandStrength(const Hand& hand) override;
};

#endif // SIMPLEAI_H
