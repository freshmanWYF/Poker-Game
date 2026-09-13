const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function client() {
  class Element {
    constructor(tag = 'div') {
      this.tagName = tag;
      this.children = [];
      this.style = {};
      this.className = '';
      this.disabled = false;
      this._text = '';
      this._html = '';
    }
    appendChild(child) { this.children.push(child); return child; }
    set textContent(value) { this._text = String(value); this.children = []; }
    get textContent() { return this._text + this.children.map(child => child.textContent).join(''); }
    set innerHTML(value) { this._html = value; this.children = []; this._text = ''; }
    get innerHTML() { return this._html; }
  }
  const elements = new Map();
  const html = fs.readFileSync(path.join(__dirname, '../web/index.html'), 'utf8');
  for (const match of html.matchAll(/id="([^"]+)"/g)) elements.set(match[1], new Element());
  const timers = new Map();
  let timerId = 0;
  class Socket {
    constructor() { this.readyState = 0; this.messages = []; }
    send(message) { this.messages.push(JSON.parse(message)); }
  }
  const context = vm.createContext({
    document: {
      getElementById: id => { assert.ok(elements.has(id), `Missing DOM element: ${id}`); return elements.get(id); },
      createElement: tag => new Element(tag),
    },
    location: { protocol: 'http:', hostname: '127.0.0.1' },
    WebSocket: Socket,
    setTimeout: callback => { timers.set(++timerId, callback); return timerId; },
    clearTimeout: id => timers.delete(id),
  });
  vm.runInContext(fs.readFileSync(path.join(__dirname, '../web/game.js'), 'utf8'), context);
  context.ws.readyState = 1;
  context.ws.onopen();
  const receive = data => context.ws.onmessage({ data: JSON.stringify(data) });
  receive({ type: 'welcome', id: 1 });
  const state = {
    type: 'sync', pot: 30, bet: 10, turn: 1, turnId: 7, phase: 1, countdown: 12,
    players: [
      { name: 'host', chips: 990, status: 1, isSeen: false },
      { name: 'guest', chips: 990, status: 1, isSeen: false },
      { name: 'other', chips: 990, status: 1, isSeen: false },
    ],
  };
  receive(state);
  return { context, elements, state, receive, timers };
}

test('seeing cards uses confirmed state, doubles cost and preserves server countdown', () => {
  const { context, elements, state, receive } = client();
  assert.equal(elements.get('call-amount').textContent, '10');
  elements.get('btn-see').onclick();
  assert.equal(context.mySeen, false);
  assert.equal(elements.get('btn-call').disabled, true);
  const action = context.ws.messages.at(-1);
  assert.equal(action.turnId, 7);
  assert.equal(action.action, 'see');
  state.players[1].isSeen = true;
  state.players[1].hand = [{ s: 0, r: 14 }, { s: 1, r: 14 }, { s: 2, r: 14 }];
  state.countdown = 8;
  receive(state);
  assert.equal(elements.get('call-amount').textContent, '20');
  assert.equal(elements.get('countdown').textContent, '8');
  assert.equal(elements.get('btn-call').disabled, true);
  receive({ type: 'actionResult', requestId: action.requestId, accepted: true });
  assert.equal(elements.get('btn-call').disabled, false);
  assert.equal(elements.get('countdown').textContent, '8');
  state.players[1].chips = 30;
  receive(state);
  assert.equal(elements.get('btn-compare').disabled, true);
  assert.equal(elements.get('btn-call').disabled, false);
});

test('duplicate actions are suppressed until the matching acknowledgement', () => {
  const { context, elements, receive } = client();
  const before = context.ws.messages.length;
  elements.get('btn-call').onclick();
  elements.get('btn-fold').onclick();
  assert.equal(context.ws.messages.length, before + 1);
  receive({ type: 'actionResult', requestId: 999, accepted: true });
  assert.equal(elements.get('btn-call').disabled, true);
  receive({ type: 'actionResult', requestId: context.ws.messages.at(-1).requestId, accepted: false });
  assert.equal(elements.get('btn-call').disabled, false);
});

test('turn changes close stale dialogs and disconnect disables actions', () => {
  const { context, elements, state, receive } = client();
  elements.get('btn-raise').onclick();
  assert.equal(elements.get('dialog-overlay').className, 'visible');
  state.turn = 2;
  state.turnId = 8;
  receive(state);
  assert.equal(elements.get('dialog-overlay').className, '');
  assert.equal(elements.get('action-area').className, '');
  const before = context.ws.messages.length;
  elements.get('btn-call').onclick();
  assert.equal(context.ws.messages.length, before);
  context.ws.readyState = 3;
  context.ws.onclose();
  assert.equal(elements.get('btn-fold').disabled, true);
  assert.equal(context.myId, -1);
  assert.match(elements.get('connection-status').textContent, /连接断开/);
});

test('player and comparison names are rendered as text', () => {
  const { elements, state, receive } = client();
  const name = '<img src=x onerror="alert(1)">';
  state.players[0].name = name;
  receive(state);
  const nameElement = elements.get('table-area').children[0].children[0].children[1].children[0];
  assert.equal(nameElement.textContent, name);
  assert.equal(nameElement.innerHTML, '');
  elements.get('btn-compare').onclick();
  const optionLabel = elements.get('dialog-options').children[0].children[0];
  assert.equal(optionLabel.textContent, name);
  assert.equal(optionLabel.innerHTML, '');
});

test('maximum seen bet is even and cannot exceed remaining chips', () => {
  const { context, elements, state, receive } = client();
  state.players[1].isSeen = true;
  state.players[1].chips = 101;
  receive(state);
  elements.get('btn-raise').onclick();
  const options = elements.get('dialog-options').children;
  options.at(-2).onclick();
  assert.equal(context.ws.messages.at(-1).amount, 100);
  assert.equal(context.ws.messages.at(-1).action, 'raise');
});

test('next round clears the result overlay and restores hidden cards', () => {
  const { elements, state, receive } = client();
  state.phase = 3;
  state.players[1].isSeen = true;
  state.players[1].hand = [{ s: 0, r: 14 }, { s: 1, r: 14 }, { s: 2, r: 14 }];
  receive(state);
  receive({ type: 'result', winnerName: 'guest', pot: 30 });
  assert.equal(elements.get('result-overlay').className, 'visible');
  state.phase = 1;
  state.turnId = 10;
  state.players[1].isSeen = false;
  delete state.players[1].hand;
  receive(state);
  assert.equal(elements.get('result-overlay').className, '');
  const myCard = elements.get('table-area').children.at(-1);
  assert.ok(myCard.children[2].children.every(card => card.className === 'card back'));
});

test('table reset clears pending actions and results while preserving the seat', () => {
  const { context, elements, state, receive } = client();
  elements.get('btn-raise').onclick();
  elements.get('btn-see').onclick();
  assert.notEqual(context.pendingAction, null);
  receive({ type: 'result', winnerName: 'guest', pot: 30 });
  state.tableId = 1;
  state.phase = 3;
  state.turn = -1;
  state.pot = 0;
  state.countdown = 0;
  for (const player of state.players) {
    player.chips = 5000;
    player.status = 0;
    player.isSeen = false;
    delete player.hand;
  }
  receive(state);
  assert.equal(context.myId, 1);
  assert.equal(context.pendingAction, null);
  assert.equal(context.myChips, 5000);
  assert.equal(elements.get('result-overlay').className, '');
  assert.equal(elements.get('dialog-overlay').className, '');
  assert.equal(elements.get('action-area').className, '');
  state.phase = 1;
  state.turn = 1;
  state.turnId = 20;
  state.players[1].status = 1;
  receive(state);
  assert.equal(elements.get('btn-call').disabled, false);
});
