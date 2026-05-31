#include "Player.h"

Player::Player(int id, QString name, bool isAI)
    : m_id(id), m_name(name), m_chips(GameConstants::INITIAL_CHIPS), 
      m_status(GameConstants::Waiting), m_isAI(isAI) {}
