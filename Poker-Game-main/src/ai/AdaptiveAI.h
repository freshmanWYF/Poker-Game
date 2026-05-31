#ifndef ADAPTIVEAI_H
#define ADAPTIVEAI_H

#include "AIStrategy.h"

/**
 * @brief 适应型 AI
 * 特点：观察对手行为（看牌频率、加注频率），动态调整策略
 * 筹码低时保守，筹码高时激进
 */
class AdaptiveAI : public AIStrategy {
public:
    AdaptiveAI() = default;
    virtual ~AdaptiveAI() = default;

    Action decide(Player* self, const GameEngine* engine) override;
    float evaluateHandStrength(const Hand& hand) override;

    QString label() const override { return "🧠"; }
    QString name() const override { return "适应型"; }

private:
    // 对手行为统计（简单计数器，每局不重置，体现"记忆"）
    int m_opponentRaiseCount = 0;
    int m_opponentFoldCount = 0;
    int m_totalOpponentActions = 0;

    // 评估场上对手的激进度 (0.0 = 全弃牌, 1.0 = 全加注)
    float opponentAggression(const GameEngine* engine, int selfId) const;

    // 记录对手行为（在 decide 中调用）
    void observeOpponents(const GameEngine* engine, int selfId);
};

#endif // ADAPTIVEAI_H
