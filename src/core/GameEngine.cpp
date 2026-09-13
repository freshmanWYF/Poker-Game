#include "GameEngine.h"
#include <limits>

GameEngine::GameEngine()
    : m_currentPot(0), m_currentBet(GameConstants::MIN_BET),
      m_currentTurnIndex(-1), m_isGameRunning(false),
      m_currentPhase(GameConstants::Settlement) {}

GameEngine::~GameEngine() { qDeleteAll(m_players); }

void GameEngine::addPlayer(const QString& name, bool isAI) {
    if (m_isGameRunning || m_players.size() >= GameConstants::MAX_PLAYERS) return;
    m_players.append(new Player(m_players.size(), name, isAI));
    m_players.last()->setChips(m_startingChips);
}

void GameEngine::resetPlayers() {
    qDeleteAll(m_players);
    m_players.clear();
    m_isGameRunning = false;
    m_currentPhase = GameConstants::Settlement;
    m_currentPot = 0;
    m_currentBet = GameConstants::MIN_BET;
    m_currentTurnIndex = -1;
}

bool GameEngine::resetTable(int startingChips) {
    if (startingChips < GameConstants::MIN_STARTING_CHIPS ||
        startingChips > GameConstants::MAX_STARTING_CHIPS) return false;

    m_isGameRunning = false;
    m_startingChips = startingChips;
    m_currentPot = 0;
    m_currentBet = GameConstants::MIN_BET;
    m_currentTurnIndex = -1;
    for (auto* player : m_players) {
        player->setChips(startingChips);
        player->setHand(Hand());
        player->setSeen(false);
        player->setStatus(GameConstants::Waiting);
    }
    setPhase(GameConstants::Settlement);
    emit potChanged(0);
    emit betChanged(m_currentBet);
    // 重开不结算被中止的牌局，不产生赢家或战绩。
    emit gameStateChanged();
    return true;
}

bool GameEngine::startGame() {
    if (m_isGameRunning || m_players.size() < 2) return false;
    int fundedPlayers = 0;
    qint64 totalChips = 0;
    for (const auto* player : m_players) {
        fundedPlayers += player->getChips() >= GameConstants::MIN_BET;
        totalChips += player->getChips();
    }
    if (fundedPlayers < 2 || totalChips > std::numeric_limits<int>::max()) return false;

    m_isGameRunning = true;
    m_currentPot = 0;
    m_currentBet = GameConstants::MIN_BET;
    m_currentTurnIndex = -1;
    setPhase(GameConstants::Dealing);
    m_deck.reset();
    m_deck.shuffle();

    for (auto* player : m_players) {
        player->setSeen(false);
        player->setHand(Hand());
        if (player->getChips() < GameConstants::MIN_BET) {
            player->setStatus(GameConstants::Waiting);
            continue;
        }
        player->setStatus(GameConstants::Active);
        QList<Card> cards;
        for (int i = 0; i < GameConstants::CARDS_PER_PLAYER; ++i) cards.append(m_deck.draw());
        player->setHand(Hand(cards));
        player->removeChips(GameConstants::MIN_BET);
        m_currentPot += GameConstants::MIN_BET;
        emit playerActed(player->getId(), "投入底注", GameConstants::MIN_BET);
    }

    m_currentTurnIndex = nextActivePlayer(-1);
    setPhase(GameConstants::Betting);
    emit turnStarted(m_currentTurnIndex);
    emit gameStateChanged();
    return true;
}

bool GameEngine::canAct(int playerId) const {
    return m_isGameRunning && m_currentPhase == GameConstants::Betting &&
           playerId >= 0 && playerId < m_players.size() &&
           playerId == m_currentTurnIndex && m_players[playerId]->isActive();
}

bool GameEngine::seeCards(int playerId) {
    if (!canAct(playerId) || m_players[playerId]->isSeen()) return false;
    m_players[playerId]->setSeen(true);
    emit playerActed(playerId, "看牌", 0);
    emit gameStateChanged();
    return true;
}

void GameEngine::nextTurn() {
    if (!m_isGameRunning || checkGameOver()) return;
    m_currentTurnIndex = nextActivePlayer(m_currentTurnIndex);
    emit turnStarted(m_currentTurnIndex);
    emit gameStateChanged();
}

bool GameEngine::fold(int playerId) {
    if (!canAct(playerId)) return false;
    m_players[playerId]->setStatus(GameConstants::Folded);
    emit playerActed(playerId, "弃牌", 0);
    nextTurn();
    return true;
}

bool GameEngine::bet(int playerId, int amount) {
    if (!canAct(playerId)) return false;
    auto* player = m_players[playerId];
    const int required = calculateRequiredBet(playerId);
    if (amount <= 0 || amount < required || amount > player->getChips() ||
        (player->isSeen() && amount % 2 != 0) ||
        amount > std::numeric_limits<int>::max() - m_currentPot) {
        emit actionRejected(playerId, "下注无效：金额需达到跟注要求且不能超过筹码；看牌后金额需为偶数。");
        return false;
    }
    const bool raised = amount > required;
    player->removeChips(amount);
    m_currentPot += amount;
    m_currentBet = player->isSeen() ? amount / 2 : amount;
    emit playerActed(playerId, raised ? "加注" : "跟注", amount);
    emit betChanged(m_currentBet);
    emit potChanged(m_currentPot);
    nextTurn();
    return true;
}

int GameEngine::calculateRequiredBet(int playerId) const {
    if (playerId < 0 || playerId >= m_players.size()) return 0;
    const qint64 amount = qint64(m_currentBet) * (m_players[playerId]->isSeen() ? 2 : 1);
    return int(qMin(amount, qint64(std::numeric_limits<int>::max())));
}

void GameEngine::applyNetworkSnapshot(int pot, int bet, int turnIndex, GameConstants::GamePhase phase) {
    m_currentPot = pot;
    m_currentBet = bet;
    m_currentTurnIndex = turnIndex;
    m_currentPhase = phase;
    m_isGameRunning = (phase != GameConstants::Settlement);
    emit gameStateChanged();
}

bool GameEngine::compare(int playerId1, int playerId2) {
    if (!canAct(playerId1)) return false;
    if (playerId2 < 0 || playerId2 >= m_players.size() || playerId1 == playerId2 ||
        !m_players[playerId2]->isActive()) {
        emit actionRejected(playerId1, "比牌对象无效，请重新选择仍在本局的对手。");
        return false;
    }
    Player* p1 = m_players[playerId1];
    Player* p2 = m_players[playerId2];
    const qint64 cost = qint64(calculateRequiredBet(playerId1)) * 2;
    if (cost > p1->getChips() || cost > std::numeric_limits<int>::max() - m_currentPot) {
        emit actionRejected(playerId1, "筹码不足，无法支付双倍跟注额进行比牌。");
        return false;
    }
    if (p1->getHand().getCards().size() != GameConstants::CARDS_PER_HAND ||
        p2->getHand().getCards().size() != GameConstants::CARDS_PER_HAND) return false;

    p1->removeChips(int(cost));
    m_currentPot += int(cost);
    emit potChanged(m_currentPot);
    const bool initiatorWins = Hand::compare(p1->getHand(), p2->getHand()) >= 0;
    auto* winner = initiatorWins ? p1 : p2;
    auto* loser = initiatorWins ? p2 : p1;
    loser->setStatus(GameConstants::Lost);
    // 对局未结束时不在公共日志暴露胜者牌型。
    emit playerActed(playerId1, initiatorWins ? "比牌胜出" : "比牌落败", int(cost));
    emit playerActed(playerId2, initiatorWins ? "比牌落败" : "比牌胜出", 0);
    emit compareResult(winner->getId(), loser->getId(), winner->getHand().typeName(), loser->getHand().typeName());
    nextTurn();
    return true;
}

void GameEngine::setPhase(GameConstants::GamePhase phase) {
    if (m_currentPhase == phase) return;
    m_currentPhase = phase;
    emit phaseChanged(phase);
}

int GameEngine::nextActivePlayer(int currentIndex) {
    for (int offset = 1; offset <= m_players.size(); ++offset) {
        const int next = (currentIndex + offset) % m_players.size();
        if (m_players[next]->isActive()) return next;
    }
    return -1;
}

bool GameEngine::checkGameOver() {
    if (!m_isGameRunning) return true;
    Player* winner = nullptr;
    int activeCount = 0;
    for (auto* player : m_players) {
        if (!player->isActive()) continue;
        ++activeCount;
        winner = player;
    }
    if (activeCount > 1) return false;

    m_isGameRunning = false;
    m_currentTurnIndex = -1;
    if (winner) {
        winner->addChips(m_currentPot);
        winner->setStatus(GameConstants::Winner);
    }
    setPhase(GameConstants::Settlement);
    if (winner) {
        emit playerActed(winner->getId(), "赢得奖池", m_currentPot);
        emit roundCompleted(winner->getId(), m_currentPot);
        emit gameOver(winner->getId());
    }
    emit gameStateChanged();
    return true;
}
