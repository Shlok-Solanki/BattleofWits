#pragma once
#include <string>

namespace bow {

struct LifelineStatus {
    bool fiftyFiftyUsed{false};
    bool audiencePollUsed{false};
    bool skipUsed{false};
    bool hintUsed{false};
};

class Player {
public:
    explicit Player(std::string name = "Player");

    const std::string& getName() const;
    int getScore() const;
    void addScore(int delta);

    LifelineStatus& lifelines();
    const LifelineStatus& lifelines() const;

private:
    std::string name_;
    int score_{0};
    LifelineStatus lifelines_{};
};

} // namespace bow
