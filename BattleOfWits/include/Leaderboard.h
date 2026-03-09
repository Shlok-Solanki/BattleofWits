#pragma once
#include <string>
#include <vector>

namespace bow {

class Leaderboard {
public:
    static void recordScore(const std::string& playerName, int score, const std::string& path = "data/leaderboard.txt");
    static std::vector<std::pair<std::string, int>> readTop(int n = 10, const std::string& path = "data/leaderboard.txt");
};

} // namespace bow
