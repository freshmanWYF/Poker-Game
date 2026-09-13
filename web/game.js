// 服务器负责回合、筹码和倒计时；只有确认的状态才能改变牌桌。
var ws = null;
var myId = -1;
var players = [];
var currentTurn = -1;
var turnId = -1;
var tableId = 0;
var phase = 3;
var gameRunning = false;
var currentPot = 0;
var currentBet = 0;
var myChips = 0;
var mySeen = false;
var requiredCall = 0;
var pendingAction = null;
var requestId = 0;
var reconnectTimer = null;
var toastTimer = null;
var PHASE_NAMES = ['发牌中', '下注中', '比牌中', '结算'];
var PHASE_COLORS = ['#aaa', '#00FFCC', '#e74c3c', '#FFD700'];

function connectionStatus(message) {
  document.getElementById('connection-status').textContent = message;
}

function connectWS() {
  clearTimeout(reconnectTimer);
  var proto = location.protocol === 'https:' ? 'wss' : 'ws';
  var socket = new WebSocket(proto + '://' + location.hostname + ':12347');
  ws = socket;
  socket.onopen = function() {
    if (ws !== socket) return;
    connectionStatus('已连接，正在申请座位…');
    socket.send(JSON.stringify({type: 'join', name: '手机玩家'}));
  };
  socket.onmessage = function(event) {
    if (ws !== socket) return;
    var data;
    try { data = JSON.parse(event.data); } catch (_) { return; }
    if (data.type === 'welcome') {
      myId = data.id;
      pendingAction = null;
      document.getElementById('conn-dot').className = 'connected';
      connectionStatus('已入座 · ' + (myId + 1) + ' 号玩家');
    } else if (data.type === 'sync') {
      updateGameState(data);
    } else if (data.type === 'actionResult') {
      if (pendingAction === data.requestId) pendingAction = null;
      refreshActionArea();
      if (!data.accepted) showToast('操作未生效，请根据当前回合重新操作');
    } else if (data.type === 'error') {
      connectionStatus(data.message || '暂时无法加入');
      showToast(data.message || '操作失败');
    } else if (data.type === 'result') {
      closeDialog();
      showResult(data);
    }
  };
  socket.onclose = function() {
    if (ws !== socket) return;
    document.getElementById('conn-dot').className = '';
    myId = -1;
    pendingAction = null;
    gameRunning = false;
    closeDialog();
    document.getElementById('result-overlay').className = '';
    refreshActionArea();
    connectionStatus('连接断开或本局暂不可加入，3 秒后重试；原座位由 AI 接管。');
    reconnectTimer = setTimeout(connectWS, 3000);
  };
  socket.onerror = function() {
    connectionStatus('连接失败，请确认手机与房主在同一 WiFi，且房间已创建。');
  };
}

function updateGameState(data) {
  var resetTable = (data.tableId || 0) !== tableId;
  tableId = data.tableId || 0;
  if (resetTable) {
    pendingAction = null;
    closeDialog();
    document.getElementById('result-overlay').className = '';
    showToast('房主已重开牌桌，筹码已重置');
  }
  var oldTurnId = turnId;
  var oldPhase = phase;
  currentPot = data.pot || 0;
  currentBet = data.bet || 0;
  currentTurn = data.turn;
  turnId = data.turnId;
  phase = data.phase;
  gameRunning = phase === 1;
  players = data.players || [];
  if (turnId !== oldTurnId || !gameRunning) closeDialog();
  if (oldPhase === 3 && phase !== 3) document.getElementById('result-overlay').className = '';
  document.getElementById('pot-val').textContent = currentPot;
  document.getElementById('phase-label').textContent = PHASE_NAMES[phase] || '等待中';
  document.getElementById('phase-label').style.color = PHASE_COLORS[phase] || '#aaa';
  var countdown = document.getElementById('countdown');
  countdown.textContent = Math.max(0, data.countdown || 0);
  countdown.className = data.countdown <= 5 ? 'urgent' : '';
  if (myId >= 0) {
    var message = !gameRunning ? '等待房主开始下一局' : currentTurn === myId ?
      '轮到你了 · 看牌不重置倒计时' : '等待 ' + (players[currentTurn] ? players[currentTurn].name : '其他玩家') + ' 操作';
    connectionStatus(message);
  }
  renderTable();
  refreshActionArea();
}

function canAct() {
  return ws && ws.readyState === 1 && myId >= 0 && currentTurn === myId && gameRunning &&
    players[myId] && players[myId].status === 1;
}

function refreshActionArea() {
  var myPlayer = players[myId];
  myChips = myPlayer ? myPlayer.chips : 0;
  mySeen = !!(myPlayer && myPlayer.isSeen);
  requiredCall = Math.max(0, currentBet) * (mySeen ? 2 : 1);
  var enabled = !!canAct() && pendingAction === null;
  document.getElementById('action-area').className = canAct() ? 'visible' : '';
  document.getElementById('btn-see').disabled = !enabled || mySeen;
  document.getElementById('btn-fold').disabled = !enabled;
  document.getElementById('btn-call').disabled = !enabled || myChips < requiredCall;
  document.getElementById('btn-raise').disabled = !enabled || myChips < requiredCall;
  document.getElementById('btn-compare').disabled = !enabled || myChips < requiredCall * 2;
  document.getElementById('btn-see').style.display = mySeen ? 'none' : '';
  document.getElementById('call-amount').textContent = requiredCall;
}

// ============ 渲染玩家桌面 ============
function renderTable() {
  var area = document.getElementById('table-area');
  area.innerHTML = '';
  players.forEach(function(p, idx) {
    if (idx !== myId) area.appendChild(createPlayerCard(p, idx));
  });
  if (players[myId]) area.appendChild(createPlayerCard(players[myId], myId));
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
  var name = document.createElement('div');
  name.className = 'player-name';
  name.textContent = (isMe ? '我 · ' : '') + (p.name || '玩家' + (idx + 1));
  var chips = document.createElement('div');
  chips.className = 'player-chips';
  chips.textContent = '筹码 ' + (p.chips || 0);
  meta.appendChild(name);
  meta.appendChild(chips);

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
  var showMyCards = isMe && p.isSeen; // 看牌后自己的牌才可见
  var showAllCards = phase === 3; // 结算后所有人都可见

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
  if (!canAct() || pendingAction !== null) return;
  var msg = {type: 'action', action: action, turnId: turnId, requestId: ++requestId};
  if (data) Object.assign(msg, data);
  pendingAction = msg.requestId;
  ws.send(JSON.stringify(msg));
  refreshActionArea();
}

document.getElementById('btn-fold').onclick = function() { sendAction('fold'); };
document.getElementById('btn-see').onclick = function() { sendAction('see'); };
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
    {label: '最大可下注', amount: myChips - myChips % (mySeen ? 2 : 1)}
  ].filter(function(o) { return o.amount >= callAmt && o.amount <= myChips; });

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
    var label = document.createElement('span');
    label.textContent = opt.label;
    btn.appendChild(label);
    if (opt.amount !== undefined) {
      var amount = document.createElement('span');
      amount.className = 'opt-amount';
      amount.textContent = opt.amount;
      btn.appendChild(amount);
    }
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
  clearTimeout(toastTimer);
  toastTimer = setTimeout(function() { t.className = ''; }, 3000);
}

// ============ 启动 ============
connectWS();
