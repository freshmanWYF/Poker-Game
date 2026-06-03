#include "HttpServer.h"
#include "../utils/Logger.h"
#include <QtCore/QDir>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QColor>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>

#ifdef HAS_QRENCODE
#include <qrencode.h>
#endif

HttpServer::HttpServer(QObject* parent) : QObject(parent), m_server(new QTcpServer(this)) {}
HttpServer::~HttpServer() { stop(); }

bool HttpServer::start(int port) {
    if (m_server->isListening()) m_server->close();
    if (!m_server->listen(QHostAddress::AnyIPv4, port)) return false;
    m_port = port;
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
    Logger::instance().log(QString("HTTP 服务器已启动: http://%1:%2").arg(getLocalIP()).arg(port));
    emit serverStarted(serverUrl());
    return true;
}

void HttpServer::stop() {
    if (m_server->isListening()) m_server->close();
}

QString HttpServer::getLocalIP() const {
    QStringList candidates;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto& iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (const auto& addr : iface.addressEntries()) {
                QString ip = addr.ip().toString();
                if (ip.contains('.') && !ip.startsWith("169.254")) {
                    candidates.append(ip);
                }
            }
        }
    }
    if (!candidates.isEmpty()) return candidates.first();
    return QString("127.0.0.1");
}

QString HttpServer::serverUrl() const {
    return QString("http://%1:%2").arg(getLocalIP()).arg(m_port);
}

void HttpServer::onNewConnection() {
    while (auto* sock = m_server->nextPendingConnection()) {
        connect(sock, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
    }
}

void HttpServer::onReadyRead() {
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray request = client->readAll();
    QStringList lines = QString::fromLatin1(request).split("\r\n");
    if (lines.isEmpty()) return;

    // 解析请求行: GET /path HTTP/1.1
    QStringList parts = lines.first().split(' ');
    if (parts.size() < 2) { send404(client); return; }
    QString method = parts[0];
    QString path = parts[1];

    if (method != "GET") {
        sendResponse(client, "Method Not Allowed", "text/plain", 405);
        return;
    }

    if (path == "/") path = "/index.html";

    // 去掉前缀斜杠
    QString relPath = path.mid(1);
    serveFile(client, relPath);
}

void HttpServer::serveFile(QTcpSocket* client, const QString& relPath) {
    // 嵌入的 HTML 内容（炸金花 Web 界面 - 优化版）
    static QMap<QString, QByteArray> embedded = {
        {"index.html", R"rawl(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>炸金花 - 手机端</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
:root {
  --green-felt: #1a5a1a;
  --green-dark: #0d3a0d;
  --gold: #FFD700;
  --gold-dim: #d4af37;
  --cyan: #00FFCC;
  --red: #e74c3c;
  --blue: #3498db;
  --orange: #e67e22;
  --purple: #9b59b6;
  --green: #27ae60;
  --dark-bg: rgba(0,0,0,0.5);
  --card-radius: 8px;
}

body {
  font-family: -apple-system, 'PingFang SC', 'Microsoft YaHei', sans-serif;
  background: radial-gradient(ellipse at center, var(--green-felt) 0%, var(--green-dark) 100%);
  color: #fff;
  height: 100vh;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  user-select: none;
}

/* ====== 顶部状态栏 ====== */
#header {
  background: var(--dark-bg);
  padding: 10px 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-shrink: 0;
  border-bottom: 1px solid rgba(255,255,255,0.1);
}
.header-title {
  display: flex;
  align-items: center;
  gap: 8px;
}
.header-title .logo {
  width: 28px;
  height: 28px;
  background: var(--gold);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 14px;
}
.header-title .title-text {
  font-size: 16px;
  font-weight: bold;
  color: var(--gold);
}
.header-info {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 13px;
}
#phase-label {
  color: var(--cyan);
  font-weight: bold;
  background: rgba(0,255,204,0.1);
  padding: 3px 10px;
  border-radius: 12px;
  border: 1px solid rgba(0,255,204,0.3);
}
#pot-label {
  color: var(--gold);
  font-weight: bold;
}
#conn-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #888;
  transition: background 0.3s;
}
#conn-dot.connected { background: #0f0; box-shadow: 0 0 6px #0f0; }

/* ====== 玩家桌面区域 ====== */
#table-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 8px;
  gap: 6px;
  overflow-y: auto;
}

/* 玩家行布局：偶数行横向，奇数行反向横向 */
.player-row {
  display: flex;
  gap: 6px;
  justify-content: center;
  animation: fadeSlideIn 0.3s ease-out;
}
@keyframes fadeSlideIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}

/* 单个玩家卡片 */
.player-card {
  flex: 1;
  min-width: 0;
  background: var(--dark-bg);
  border-radius: 12px;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 6px;
  border: 2px solid transparent;
  transition: border-color 0.3s, background 0.3s, transform 0.2s;
  position: relative;
}
.player-card.active-turn {
  border-color: var(--gold);
  background: rgba(255,215,0,0.08);
  transform: scale(1.02);
}
.player-card.winner {
  border-color: #0f0;
  background: rgba(0,255,0,0.1);
}
.player-card.loser { opacity: 0.45; }
.player-card.my-card {
  border-color: rgba(0,255,204,0.5);
  background: rgba(0,255,204,0.05);
}

/* 玩家头部：头像+名字 */
.player-head {
  display: flex;
  align-items: center;
  gap: 6px;
}
.player-avatar {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  background: #333;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 16px;
  flex-shrink: 0;
  border: 2px solid #555;
}
.player-card.active-turn .player-avatar { border-color: var(--gold); }
.player-card.my-card .player-avatar { border-color: var(--cyan); }

.player-meta { flex: 1; min-width: 0; }
.player-name {
  font-size: 12px;
  font-weight: bold;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.player-chips {
  font-size: 11px;
  color: #aaa;
}
.player-bet {
  font-size: 10px;
  color: var(--gold-dim);
  margin-top: 1px;
}

/* 玩家状态标签 */
.player-status {
  font-size: 10px;
  padding: 2px 6px;
  border-radius: 8px;
  text-align: center;
  background: rgba(255,255,255,0.1);
  color: #ccc;
}
.player-status.seen { background: rgba(155,89,182,0.3); color: #c39bd3; }
.player-status.folded { background: rgba(231,76,60,0.3); color: #e74c3c; }
.player-status.lost { background: rgba(149,165,166,0.3); color: #95a5a6; }
.player-status.winner { background: rgba(39,174,96,0.3); color: #2ecc71; }
.player-status.my-turn { background: rgba(255,215,0,0.3); color: var(--gold); }

/* 卡牌区域 */
.player-cards {
  display: flex;
  gap: 3px;
  justify-content: center;
}
.card {
  width: 36px;
  height: 52px;
  background: #fff;
  border-radius: var(--card-radius);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  font-size: 13px;
  font-weight: bold;
  color: #222;
  border: 1px solid #ccc;
  box-shadow: 1px 2px 4px rgba(0,0,0,0.3);
  transition: transform 0.2s;
}
.card.red { color: #c00; }
.card.back {
  background: linear-gradient(135deg, #2c3e50, #1a252f);
  color: #fff;
  border-color: #1a252f;
  font-size: 18px;
  position: relative;
  overflow: hidden;
}
.card.back::before {
  content: '?';
  position: absolute;
  font-size: 18px;
  color: rgba(255,255,255,0.15);
}
.card.face-up {
  animation: cardFlip 0.4s ease-out;
}
@keyframes cardFlip {
  0% { transform: rotateY(0deg); }
  50% { transform: rotateY(90deg); }
  100% { transform: rotateY(0deg); }
}
.player-card.my-card .card.face-up { animation: none; }

/* ====== 底部操作区 ====== */
#action-area {
  flex-shrink: 0;
  background: var(--dark-bg);
  padding: 10px 12px 12px;
  display: none;
  flex-direction: column;
  gap: 8px;
  border-top: 1px solid rgba(255,255,255,0.1);
}
#action-area.visible { display: flex; }

/* 操作区标题 */
.action-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.action-header .label {
  font-size: 13px;
  color: var(--gold);
  font-weight: bold;
}
#countdown {
  font-size: 20px;
  font-weight: bold;
  color: var(--cyan);
  min-width: 28px;
  text-align: center;
}
#countdown.urgent { color: var(--red); animation: pulse 0.5s infinite; }
@keyframes pulse {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.15); }
}

/* 操作按钮行 */
.action-btns {
  display: flex;
  gap: 6px;
  justify-content: center;
}
.action-btn {
  flex: 1;
  min-width: 0;
  padding: 12px 6px;
  border: none;
  border-radius: 10px;
  font-size: 13px;
  font-weight: bold;
  cursor: pointer;
  transition: transform 0.1s, opacity 0.15s, box-shadow 0.15s;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}
.action-btn:active { transform: scale(0.94); }
.action-btn:disabled { opacity: 0.35; cursor: not-allowed; transform: none; }

.btn-fold { background: linear-gradient(135deg, #c0392b, #a93226); color: #fff; box-shadow: 0 3px 0 #7b241c; }
.btn-fold:active { box-shadow: 0 1px 0 #7b241c; }
.btn-see { background: linear-gradient(135deg, #8e44ad, #7d3c98); color: #fff; box-shadow: 0 3px 0 #5b2c6f; }
.btn-see:active { box-shadow: 0 1px 0 #5b2c6f; }
.btn-call { background: linear-gradient(135deg, #2980b9, #2471a3); color: #fff; box-shadow: 0 3px 0 #1a5276; }
.btn-call:active { box-shadow: 0 1px 0 #1a5276; }
.btn-raise { background: linear-gradient(135deg, #e67e22, #ca6f1e); color: #fff; box-shadow: 0 3px 0 #a04000; }
.btn-raise:active { box-shadow: 0 1px 0 #a04000; }
.btn-compare { background: linear-gradient(135deg, #27ae60, #1e8449); color: #fff; box-shadow: 0 3px 0 #145a32; }
.btn-compare:active { box-shadow: 0 1px 0 #145a32; }

.btn-label { font-size: 13px; }
.btn-sub { font-size: 10px; opacity: 0.8; }

/* ====== 比牌/加注对话框 ====== */
#dialog-overlay {
  position: fixed; top: 0; left: 0; right: 0; bottom: 0;
  background: rgba(0,0,0,0.75);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 100;
}
#dialog-overlay.visible { display: flex; }
#dialog-box {
  background: linear-gradient(135deg, #1e1e1e, #2d2d2d);
  border-radius: 16px;
  padding: 20px 16px;
  width: 92%;
  max-width: 340px;
  border: 1px solid rgba(255,255,255,0.1);
  box-shadow: 0 8px 32px rgba(0,0,0,0.5);
}
#dialog-box h3 {
  color: var(--gold);
  margin-bottom: 8px;
  text-align: center;
  font-size: 17px;
}
#dialog-box .sub {
  text-align: center;
  font-size: 12px;
  color: #888;
  margin-bottom: 14px;
}
.option-list { display: flex; flex-direction: column; gap: 8px; }
.option-btn {
  padding: 12px 14px;
  border: none;
  border-radius: 10px;
  background: linear-gradient(135deg, #3a3a3a, #2a2a2a);
  color: #fff;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  display: flex;
  justify-content: space-between;
  align-items: center;
  transition: background 0.15s, transform 0.1s;
}
.option-btn:active { transform: scale(0.97); }
.option-btn .opt-amount { color: var(--cyan); font-size: 13px; }
.option-btn.cancel-btn {
  background: linear-gradient(135deg, #444, #333);
  font-size: 13px;
  margin-top: 6px;
}

/* ====== Toast ====== */
#toast {
  position: fixed;
  top: 58px;
  left: 50%;
  transform: translateX(-50%);
  background: rgba(0,0,0,0.85);
  color: var(--gold);
  padding: 8px 20px;
  border-radius: 20px;
  font-size: 13px;
  z-index: 200;
  opacity: 0;
  transition: opacity 0.3s;
  pointer-events: none;
  white-space: nowrap;
}
#toast.visible { opacity: 1; }

/* ====== 结算弹窗 ====== */
#result-overlay {
  position: fixed; top: 0; left: 0; right: 0; bottom: 0;
  background: rgba(0,0,0,0.85);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 150;
  flex-direction: column;
  gap: 16px;
}
#result-overlay.visible { display: flex; }
#result-overlay .result-title {
  font-size: 28px;
  font-weight: bold;
  color: var(--gold);
  text-align: center;
  text-shadow: 0 2px 10px rgba(255,215,0,0.3);
}
#result-overlay .result-pot {
  font-size: 16px;
  color: var(--cyan);
  text-align: center;
}
#result-overlay .result-btn {
  padding: 12px 40px;
  background: var(--gold);
  color: #222;
  border: none;
  border-radius: 24px;
  font-size: 15px;
  font-weight: bold;
  cursor: pointer;
}

/* ====== 响应式：小屏 ====== */
@media (max-height: 600px) {
  .player-card { padding: 6px; gap: 4px; }
  .card { width: 30px; height: 44px; }
  #action-area { padding: 8px 10px 10px; }
  .action-btn { padding: 10px 6px; font-size: 12px; }
}
</style>
</head>
<body>

<div id="header">
  <div class="header-title">
    <div class="logo">🃏</div>
    <span class="title-text">炸金花</span>
  </div>
  <div class="header-info">
    <span id="phase-label">等待中</span>
    <span id="pot-label">奖池: <span id="pot-val">0</span></span>
    <div id="conn-dot"></div>
  </div>
</div>

<div id="table-area"></div>

<div id="action-area">
  <div class="action-header">
    <span class="label">⚡ 你的回合</span>
    <span id="countdown">30</span>
  </div>
  <div class="action-btns">
    <button class="action-btn btn-see" id="btn-see">
      <span class="btn-label">看牌</span>
    </button>
    <button class="action-btn btn-fold" id="btn-fold">
      <span class="btn-label">弃牌</span>
    </button>
    <button class="action-btn btn-call" id="btn-call">
      <span class="btn-label">跟注</span>
      <span class="btn-sub" id="call-amount">0</span>
    </button>
    <button class="action-btn btn-raise" id="btn-raise">
      <span class="btn-label">加注</span>
    </button>
    <button class="action-btn btn-compare" id="btn-compare">
      <span class="btn-label">比牌</span>
    </button>
  </div>
</div>

<div id="dialog-overlay">
  <div id="dialog-box">
    <h3 id="dialog-title">选择</h3>
    <div class="sub" id="dialog-sub"></div>
    <div class="option-list" id="dialog-options"></div>
  </div>
</div>

<div id="toast"></div>

<div id="result-overlay">
  <div class="result-title" id="result-title">结算</div>
  <div class="result-pot" id="result-pot">奖池: 0</div>
  <button class="result-btn" onclick="document.getElementById('result-overlay').className=''">确定</button>
</div>

<script>
// ============ 状态 ============
var ws = null;
var myId = -1;
var players = [];
var currentTurn = -1;
var phase = 0; // 0=Dealing 1=Betting 2=Comparing 3=Settlement
var gameRunning = false;
var currentPot = 0;
var currentBet = 0;
var myChips = 0;
var myCards = [];
var mySeen = false;
var myHandVisible = false; // 自己的牌是否已可见（看牌后才显示）
var countdownSec = 30;
var countdownTimer = null;
var requiredCall = 0;

// ============ 阶段映射 ============
var PHASE_NAMES = ['发牌中', '下注中', '比牌中', '结算'];
var PHASE_COLORS = ['#aaa', '#00FFCC', '#e74c3c', '#FFD700'];

// ============ 连接 ============
function connectWS() {
  var proto = location.protocol === 'https:' ? 'wss' : 'ws';
  ws = new WebSocket(proto + '://' + location.hostname + ':12347');

  ws.onopen = function() {
    document.getElementById('conn-dot').className = 'connected';
    showToast('已连接服务器');
    ws.send(JSON.stringify({type: 'join', name: '手机玩家'}));
  };

  ws.onmessage = function(e) {
    var data = JSON.parse(e.data);
    if (data.type === 'welcome') {
      myId = data.id !== undefined ? data.id : (data.playerId !== undefined ? data.playerId : -1);
      showToast('你是 ' + (myId + 1) + ' 号玩家');
    } else if (data.type === 'sync') {
      updateGameState(data);
    } else if (data.type === 'result') {
      // 结算后显示所有人的牌
      myHandVisible = true;
      showResult(data);
    }
  };

  ws.onclose = function() {
    document.getElementById('conn-dot').className = '';
    showToast('连接断开，3秒后重连...');
    setTimeout(connectWS, 3000);
  };

  ws.onerror = function() {
    showToast('连接失败');
  };
}

// ============ 游戏状态同步 ============
function updateGameState(data) {
  currentPot = data.pot || 0;
  currentBet = data.bet || 0;
  currentTurn = data.turn;
  var prevPhase = phase;
  phase = data.phase;
  gameRunning = (phase === 1);

  // 游戏重开时（从结算阶段回到下注/发牌阶段），重置状态
  if (prevPhase === 3 && phase < 3) {
    myHandVisible = false;
  }

  // 更新顶部信息
  document.getElementById('pot-val').textContent = currentPot;
  document.getElementById('phase-label').textContent = PHASE_NAMES[phase] || '等待中';
  document.getElementById('phase-label').style.color = PHASE_COLORS[phase] || '#aaa';

  players = data.players || [];
  renderTable();

  // 刷新操作按钮
  refreshActionArea();
}

function refreshActionArea() {
  var isMyTurn = (currentTurn === myId) && gameRunning;
  var myPlayer = players[myId];
  var isActive = myPlayer && (myPlayer.status === 1); // 1=Active

  // 计算跟注金额
  if (myPlayer) {
    myChips = myPlayer.chips || 0;
    myCards = myPlayer.hand || [];
    mySeen = myPlayer.isSeen || false;
    requiredCall = Math.max(0, currentBet); // 简化：跟当前下注
  }

  var actionArea = document.getElementById('action-area');
  if (isMyTurn && isActive) {
    actionArea.className = 'visible';
    startCountdown();
  } else {
    actionArea.className = '';
    stopCountdown();
  }

  var canAct = isMyTurn && isActive;
  var btnSee = document.getElementById('btn-see');
  var btnCall = document.getElementById('btn-call');
  var btnRaise = document.getElementById('btn-raise');
  var btnCompare = document.getElementById('btn-compare');
  var btnFold = document.getElementById('btn-fold');

  btnSee.disabled = !canAct || mySeen;
  btnFold.disabled = !canAct;
  btnCall.disabled = !canAct;
  btnRaise.disabled = !canAct;
  btnCompare.disabled = !canAct;

  btnSee.style.display = mySeen ? 'none' : '';

  // 跟注金额显示
  document.getElementById('call-amount').textContent = requiredCall + '';
}

// ============ 计时器 ============
function startCountdown() {
  stopCountdown();
  countdownSec = 30;
  var el = document.getElementById('countdown');
  el.className = '';
  el.textContent = countdownSec;
  countdownTimer = setInterval(function() {
    countdownSec--;
    el.textContent = countdownSec;
    if (countdownSec <= 10) el.className = 'urgent';
    if (countdownSec <= 0) {
      stopCountdown();
      // 超时自动弃牌
      sendAction('fold');
    }
  }, 1000);
}

function stopCountdown() {
  if (countdownTimer) {
    clearInterval(countdownTimer);
    countdownTimer = null;
  }
}

// ============ 渲染玩家桌面 ============
function renderTable() {
  var area = document.getElementById('table-area');
  area.innerHTML = '';

  if (players.length === 0) return;

  // 分成两行
  var half = Math.ceil(players.length / 2);
  var row1 = players.slice(0, half);
  var row2 = players.slice(half);

  [row2, row1].forEach(function(row, rowIdx) {
    if (row.length === 0) return;
    var rowDiv = document.createElement('div');
    rowDiv.className = 'player-row';
    rowDiv.style.animationDelay = (rowIdx * 0.1) + 's';

    row.forEach(function(p, i) {
      var idx = players.indexOf(p);
      var card = createPlayerCard(p, idx);
      rowDiv.appendChild(card);
    });

    area.appendChild(rowDiv);
  });
}

function createPlayerCard(p, idx) {
  var isMe = (idx === myId);
  var isTurn = (idx === currentTurn) && gameRunning;
  var isWinner = (p.status === 4);
  var isLoser = (p.status === 3);

  var div = document.createElement('div');
  var cls = 'player-card';
  if (isTurn) cls += ' active-turn';
  if (isWinner) cls += ' winner';
  if (isLoser) cls += ' loser';
  if (isMe) cls += ' my-card';
  div.className = cls;

  // 头像
  var avatar = document.createElement('div');
  avatar.className = 'player-avatar';
  avatar.textContent = isMe ? '🙋' : '👤';

  // 名字+筹码
  var meta = document.createElement('div');
  meta.className = 'player-meta';
  meta.innerHTML = '<div class="player-name">' + (isMe ? '我' : (p.name || '玩家' + (idx+1))) + '</div>' +
    '<div class="player-chips">💰 ' + (p.chips || 0) + '</div>' +
    '<div class="player-bet"></div>';

  var head = document.createElement('div');
  head.className = 'player-head';
  head.appendChild(avatar);
  head.appendChild(meta);

  // 状态标签
  var statusEl = document.createElement('div');
  statusEl.className = 'player-status';
  var statusText = getStatusText(p.status, p.isSeen);
  statusEl.textContent = statusText;
  if (p.isSeen && p.status === 1) statusEl.className += ' seen';
  else if (p.status === 2) statusEl.className += ' folded';
  else if (p.status === 3) statusEl.className += ' lost';
  else if (p.status === 4) statusEl.className += ' winner';
  else if (isTurn) statusEl.className += ' my-turn';

  // 卡牌：自己的牌需看牌后才显示，其他人在比牌/结算后显示
  var cardsEl = document.createElement('div');
  cardsEl.className = 'player-cards';
  var hasHand = p.hand && p.hand.length > 0;
  var showMyCards = isMe && myHandVisible; // 看牌后自己的牌才可见
  var showAllCards = isWinner || isLoser; // 结算后所有人都可见

  if (hasHand && (showMyCards || showAllCards)) {
    // 显示牌面
    p.hand.forEach(function(c) {
      var cardEl = document.createElement('div');
      cardEl.className = 'card face-up' + (isRedCard(c.s) ? ' red' : '');
      cardEl.innerHTML = getRankSymbol(c.r) + '<br>' + getSuitSymbol(c.s);
      cardsEl.appendChild(cardEl);
    });
  } else if (hasHand && isMe) {
    // 自己的牌但还没看牌：显示背面 + 问号标记
    for (var i = 0; i < 3; i++) {
      var cardEl = document.createElement('div');
      cardEl.className = 'card back';
      cardsEl.appendChild(cardEl);
    }
  } else {
    // 其他人的牌：背面
    for (var i = 0; i < 3; i++) {
      var cardEl = document.createElement('div');
      cardEl.className = 'card back';
      cardsEl.appendChild(cardEl);
    }
  }

  div.appendChild(head);
  div.appendChild(statusEl);
  div.appendChild(cardsEl);
  return div;
}

// ============ 状态文本 ============
function getStatusText(status, isSeen) {
  if (status === 1) return isSeen ? '已看牌' : '蒙牌中';
  if (status === 2) return '已弃牌';
  if (status === 3) return '比牌输';
  if (status === 4) return '🏆 赢家';
  return '等待中';
}

function isRedCard(suit) { return suit === 1 || suit === 3; }

function getSuitSymbol(s) {
  return ['♠', '♥', '♣', '♦'][s] || '?';
}

function getRankSymbol(r) {
  var names = ['', '', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A'];
  return names[r] || '?';
}

// ============ 操作 ============
function sendAction(action, data) {
  if (!ws || ws.readyState !== 1) return;
  stopCountdown();
  var msg = {type: 'action', action: action};
  if (data) Object.assign(msg, data);
  ws.send(JSON.stringify(msg));
  showToast('已发送: ' + action);
}

document.getElementById('btn-fold').onclick = function() { sendAction('fold'); };
document.getElementById('btn-see').onclick = function() {
  myHandVisible = true;
  sendAction('see');
};
document.getElementById('btn-call').onclick = function() { sendAction('call'); };

document.getElementById('btn-raise').onclick = function() {
  // 加注选项：跟注、2x、3x、全下
  var callAmt = requiredCall;
  var doubleAmt = Math.max(callAmt * 2, callAmt + 10);
  var tripleAmt = Math.max(callAmt * 3, callAmt + 20);
  var options = [
    {label: '跟注', amount: callAmt},
    {label: '加注 2x', amount: doubleAmt},
    {label: '加注 3x', amount: tripleAmt},
    {label: '全下 (' + myChips + ')', amount: myChips}
  ].filter(function(o) { return o.amount > 0 && o.amount <= myChips; });

  showDialog('加注', options, function(amount) {
    if (amount > 0) sendAction('raise', {amount: amount});
  }, '需跟注: ' + callAmt + ' | 筹码: ' + myChips);
};

document.getElementById('btn-compare').onclick = function() {
  var options = players.filter(function(p, i) {
    return i !== myId && p.status === 1;
  }).map(function(p) {
    return {label: p.name || '玩家' + (players.indexOf(p)+1), idx: players.indexOf(p)};
  });
  if (options.length === 0) {
    showToast('没有可比的玩家');
    return;
  }
  showDialog('选择比牌对象', options.map(function(o) {
    return {label: o.label, idx: o.idx};
  }), function(targetIdx) {
    sendAction('compare', {targetId: targetIdx});
  });
};

// ============ 对话框 ============
function showDialog(title, options, callback, subText) {
  document.getElementById('dialog-title').textContent = title;
  document.getElementById('dialog-sub').textContent = subText || '';
  var list = document.getElementById('dialog-options');
  list.innerHTML = '';

  options.forEach(function(opt) {
    var btn = document.createElement('button');
    btn.className = 'option-btn';
    btn.innerHTML = '<span>' + opt.label + '</span>' +
      (opt.amount !== undefined ? '<span class="opt-amount">' + opt.amount + '</span>' : '');
    btn.onclick = function() {
      closeDialog();
      callback(opt.amount !== undefined ? opt.amount : opt.idx);
    };
    list.appendChild(btn);
  });

  var cancelBtn = document.createElement('button');
  cancelBtn.className = 'option-btn cancel-btn';
  cancelBtn.textContent = '取消';
  cancelBtn.onclick = closeDialog;
  list.appendChild(cancelBtn);

  document.getElementById('dialog-overlay').className = 'visible';
}

function closeDialog() {
  document.getElementById('dialog-overlay').className = '';
}

// ============ 结算 ============
function showResult(data) {
  var overlay = document.getElementById('result-overlay');
  var title = document.getElementById('result-title');
  var pot = document.getElementById('result-pot');
  title.textContent = data.winnerName ? data.winnerName + ' 获胜!' : '结算';
  pot.textContent = '奖池: ' + (data.pot || currentPot);
  overlay.className = 'visible';
}

// ============ Toast ============
function showToast(msg) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'visible';
  setTimeout(function() { t.className = ''; }, 2000);
}

// ============ 启动 ============
connectWS();
</script>
</body>
</html>
)rawl"}
    };

    if (embedded.contains(relPath)) {
        QByteArray content = embedded[relPath];
        QString ct = "text/html";
        if (relPath.endsWith(".css")) ct = "text/css";
        else if (relPath.endsWith(".js")) ct = "application/javascript";
        else if (relPath.endsWith(".png")) ct = "image/png";
        else if (relPath.endsWith(".ico")) ct = "image/x-icon";
        sendResponse(client, content, ct, 200);
        return;
    }

    // 其他文件返回 404
    Q_UNUSED(relPath);
    send404(client);
}

void HttpServer::sendResponse(QTcpSocket* client, const QByteArray& body,
                              const QString& contentType, int statusCode) {
    const char* statusLine = (statusCode == 200) ? "HTTP/1.1 200 OK" :
                             (statusCode == 405) ? "HTTP/1.1 405 Method Not Allowed" :
                             "HTTP/1.1 404 Not Found";

    QByteArray response;
    response.append(QByteArray(statusLine) + "\r\n");
    response.append("Content-Type: " + contentType.toLatin1() + "; charset=utf-8\r\n");
    response.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    response.append("Connection: close\r\n");
    response.append("\r\n");
    response.append(body);

    client->write(response);
    client->disconnectFromHost();
}

void HttpServer::send404(QTcpSocket* client) {
    sendResponse(client, "404 Not Found", "text/plain", 404);
}

QByteArray HttpServer::generateQRCodePNG(const QString& text, int size) {
#ifdef HAS_QRENCODE
    QRcode* qr = QRcode_encodeString(text.toUtf8().constData(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr) return QByteArray();

    int modules = qr->width;
    int moduleSize = qMax(1, size / (modules + 2));
    int imgSize = moduleSize * (modules + 2);

    QImage image(imgSize, imgSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);

    for (int y = 0; y < modules; ++y) {
        for (int x = 0; x < modules; ++x) {
            if (qr->data[y * modules + x] & 0x01) {
                painter.drawRect((x + 1) * moduleSize, (y + 1) * moduleSize, moduleSize, moduleSize);
            }
        }
    }
    painter.end();
    QRcode_free(qr);

    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    return ba;
#else
    Q_UNUSED(text) Q_UNUSED(size)
    return QByteArray();
#endif
}