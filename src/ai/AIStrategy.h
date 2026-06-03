#ifndef AISTRATEGY_H
#define AISTRATEGY_H

#include "../core/Player.h"
#include "../core/GameEngine.h"
#include <QtCore/QString>

/**
 * @brief AI 决策接口类
 * 采用策略模式，方便后续扩展不同的 AI 算法
 */
class AIStrategy {
public:
    virtual ~AIStrategy() = default;

    // AI 的可能动作
    enum Action {
        Fold,   // 弃牌
        Call,   // 跟注
        Raise   // 加注
    };

    /**
     * @brief 决策函数
     * @param self AI 玩家对象
     * @param engine 游戏引擎引用
     * @return 最终做出的动作
     */
    virtual Action decide(Player* self, const GameEngine* engine) = 0;

    /**
     * @brief 手牌强度评估 (0-100)
     */
    virtual float evaluateHandStrength(const Hand& hand) = 0;

    /**
     * @brief AI 标签（用于 UI 显示）
     */
    virtual QString label() const { return ""; }

    /**
     * @brief AI 类型名称
     */
    virtual QString name() const { return "标准型"; }
};

#endif // AISTRATEGY_H
