# 炸金花（Golden Flower Poker）

基于 Qt6/C++ 的本地+联机炸金花扑克牌游戏，支持 PC 桌面端与手机 Web 端多人对局。

## 功能特性

### 游戏核心
- 炸金花完整规则：蒙牌、看牌、跟注、加注、比牌
- 4 种 AI 策略：谨慎型 / 激进型 / 简单型 / 自适应型
- 牌型判定：豹子 > 顺金 > 金花 > 顺子 > 对子 > 单张，特殊 235 反杀豹子
- 筹码系统：初始 1000 筹码，自动投入底注，支持多局累计

### WiFi 多人联机
- 手机扫码加入：PC 端创建房间后生成 WiFi QR 码，手机扫码连接
- WebSocket 实时通信（端口 12347），HTTP 托管 Web UI（端口 8080）
- 手机 Web 界面：两行玩家布局、倒计时操作、加注选项与 PC 端一致
- 手机玩家操作：蒙牌开始，点击"看牌"后才显示手牌

### 断线处理
- 手机玩家掉线自动由 AI 接管，策略数组重建保证 AI 操作正常
- 玩家数下限保护：设置的 AI 数量不会低于当前真人玩家数

### 其他
- 卡牌动画、筹码飞向赢家动画
- 胜率预估、战绩统计
- 对局记录与筹码存档

## 技术栈

- **语言**：C++17
- **框架**：Qt6（QtWidgets、QtWebSockets、QtNetwork）
- **构建**：CMake + Ninja
- **AI**：4 种策略，根据手牌/筹码/对手数量动态决策
- **联机**：WebSocket（手机端） + TCP（旧协议兼容）

## 项目结构

```
src/
├── ai/            # AI 策略（CautiousAI、AggressiveAI、SimpleAI、AdaptiveAI）
├── controller/    # GameController：游戏逻辑与网络协调
├── core/          # 游戏核心（GameEngine、Player、Card、Hand、Deck、GameConstants）
├── network/       # 网络模块（WebSocketServer、HttpServer、NetworkManager）
├── ui/            # 桌面 UI（MainWindow、PlayerWidget、CardWidget）
└── utils/         # 工具（Logger、GameStore）
```

## 构建与运行

```bash
mkdir build && cd build
cmake -G Ninja ..
ninja
./PokerGame
```

### 依赖
- Qt6（Core、Gui、Widgets、Network、WebSockets）
- libqrencode（可选，用于生成 QR 码）

### 手机连接
1. PC 端点击"创建房间"，左上角显示 QR 码
2. 手机连接同一 WiFi，扫码或手动输入地址
3. 浏览器打开后自动加入游戏，替代 AI 玩家槽位

## 操作说明

| 按钮 | 说明 |
|------|------|
| 看牌 | 花费当前下注额查看手牌，之后下注额翻倍 |
| 跟注 | 支付当前下注额继续游戏 |
| 加注 | 支付更多筹码，可选 2x / 3x / 全下 |
| 比牌 | 支付双倍下注额与指定玩家比牌 |
| 弃牌 | 放弃本轮，不消耗筹码 |
