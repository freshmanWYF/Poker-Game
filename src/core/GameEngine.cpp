#include "GameEngine.h"

GameEngine::GameEngine()
    : m_currentPot(0), m_currentBet(GameConstants::MIN_BET), 
      m_currentTurnIndex(0), m_isGameRunning(false),
      m_currentPhase(GameConstants::Settlement) {}

void GameEngine::addPlayer(const QString& name, bool isAI) {
    m_players.append(new Player(m_players.size(), name, isAI));
}

void GameEngine::resetPlayers() {
    qDeleteAll(m_players);
    m_players.clear();
    m_isGameRunning = false;
    m_currentPhase = GameConstants::Settlement;
}

void GameEngine::startGame() {
    if (m_players.size() < 2) return;

    setPhase(GameConstants::Dealing);

    m_deck.reset();
    m_deck.shuffle();

    m_currentPot = 0;
    m_currentBet = GameConstants::MIN_BET;
    m_isGameRunning = true;

    for (auto player : m_players) {
        player->setStatus(GameConstants::Active);
        player->setSeen(false); // 确保一开始都是暗牌
        QList<Card> cards;
        for (int i = 0; i < GameConstants::CARDS_PER_PLAYER; ++i) {
            cards.append(m_deck.draw());
        }
        player->setHand(Hand(cards));
        
        // 初始底注
        player->removeChips(GameConstants::MIN_BET);
        m_currentPot += GameConstants::MIN_BET;
        emit playerActed(player->getId(), "投入底注", GameConstants::MIN_BET);
    }

    m_currentTurnIndex = 0;
    setPhase(GameConstants::Betting);
    emit gameStateChanged();
    emit turnStarted(m_players[m_currentTurnIndex]->getId());
}

void GameEngine::seeCards(int playerId) {
    m_players[playerId]->setSeen(true);
    emit playerActed(playerId, "看牌", 0);
    emit gameStateChanged();
}

void GameEngine::nextTurn() {
    if (!m_isGameRunning) return;

    m_currentTurnIndex = nextActivePlayer(m_currentTurnIndex);
    
    if (checkGameOver()) return;

    emit turnStarted(m_players[m_currentTurnIndex]->getId());
    emit gameStateChanged();
}

void GameEngine::fold(int playerId) {
    auto player = m_players[playerId];
    player->setStatus(GameConstants::Folded);
    emit playerActed(playerId, "弃牌", 0);
    
    if (checkGameOver()) return;
    nextTurn();
}

void GameEngine::bet(int playerId, int amount) {
    auto player = m_players[playerId];
    
    // 如果看牌了，下注是暗牌的两倍
    int darkBet = player->isSeen() ? (amount / 2) : amount;
    
    // 确保暗牌底注不小于当前暗牌底注
    if (darkBet < m_currentBet) {
        darkBet = m_currentBet;
        amount = player->isSeen() ? (2 * darkBet) : darkBet;
    }
    
    player->removeChips(amount);
    m_currentPot += amount;
    m_currentBet = darkBet;
    
    emit playerActed(playerId, amount > darkBet ? "加注" : "跟注", amount);
    emit betChanged(m_currentBet);
    emit potChanged(m_currentPot);
    nextTurn();
}

int GameEngine::calculateRequiredBet(int playerId) const {
    auto player = m_players[playerId];
    return player->isSeen() ? (2 * m_currentBet) : m_currentBet;
}

void GameEngine::applyNetworkSnapshot(int pot, int bet, int turnIndex, GameConstants::GamePhase phase) {
    m_currentPot = pot;
    m_currentBet = bet;
    m_currentTurnIndex = turnIndex;
    m_currentPhase = phase;
    m_isGameRunning = (phase != GameConstants::Settlement);
    emit gameStateChanged();
}

void GameEngine::compare(int playerId1, int playerId2) {
    if (m_currentPhase != GameConstants::Betting) return;

    // 比牌需要支付当前应下注额的 2 倍
    int cost = calculateRequiredBet(playerId1) * 2;
    Player* p1 = m_players[playerId1];
    Player* p2 = m_players[playerId2];

    if (!p1->isActive() || !p2->isActive()) return;
    if (p1->getChips() < cost) return;

    p1->removeChips(cost);
    m_currentPot += cost;
    emit potChanged(m_currentPot);

    int result = Hand::compare(p1->getHand(), p2->getHand());
    QString p1Type = p1->getHand().typeName();
    QString p2Type = p2->getHand().typeName();

    if (result >= 0) {
        // p1 赢（result==1 或 0 时发起者胜）
        p2->setStatus(GameConstants::Lost);
        emit playerActed(playerId1, QString("比牌胜出 (%1 vs %2)").arg(p1Type, p2Type), cost);
        emit playerActed(playerId2, QString("比牌落败 (%1 vs %2)").arg(p2Type, p1Type), 0);
        emit compareResult(playerId1, playerId2, p1Type, p2Type);
    } else {
        // p2 赢
        p1->setStatus(GameConstants::Lost);
        emit playerActed(playerId2, QString("比牌胜出 (%1 vs %2)").arg(p2Type, p1Type), 0);
        emit playerActed(playerId1, QString("比牌落败 (%1 vs %2)").arg(p1Type, p2Type), cost);
        emit compareResult(playerId2, playerId1, p2Type, p1Type);
    }

    if (checkGameOver()) return;

    setPhase(GameConstants::Betting);
    nextTurn();
}

void GameEngine::setPhase(GameConstants::GamePhase phase) {
    if (m_currentPhase != phase) {
        m_currentPhase = phase;
        emit phaseChanged(m_currentPhase);
    }
}

int GameEngine::nextActivePlayer(int currentIndex) {
    int next = (currentIndex + 1) % m_players.size();
    while (!m_players[next]->isActive() && next != currentIndex) {
        next = (next + 1) % m_players.size();
    }
    return next;
}

bool GameEngine::checkGameOver() {
    int activeCount = 0;
    Player* lastActive = nullptr;

    for (auto player : m_players) {
        if (player->isActive()) {
            activeCount++;
            lastActive = player;
        }
    }

    if (activeCount <= 1) {
        m_isGameRunning = false;
        setPhase(GameConstants::Settlement);
        if (lastActive) {
            int potWon = m_currentPot;
            lastActive->addChips(potWon);
            lastActive->setStatus(GameConstants::Winner);
            emit playerActed(lastActive->getId(), "赢得奖池", potWon);
            emit roundCompleted(lastActive->getId(), potWon);
            emit gameOver(lastActive->getId());
        }
        return true;
    }
    return false;
}
