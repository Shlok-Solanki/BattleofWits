#include "Player.h"

namespace bow {

Player::Player(std::string name) : name_(std::move(name)) {}

const std::string& Player::getName() const { return name_; }
int Player::getScore() const { return score_; }
void Player::addScore(int delta) { score_ += delta; }

LifelineStatus& Player::lifelines() { return lifelines_; }
const LifelineStatus& Player::lifelines() const { return lifelines_; }

} // namespace bow
