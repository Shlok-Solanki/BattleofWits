#include "Leaderboard.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

namespace bow {

void Leaderboard::recordScore(const std::string& playerName, int score, const std::string& path) {
    std::ofstream ofs(path, std::ios::app);
    if (!ofs) return;
    ofs << playerName << "," << score << "\n";
}

std::vector<std::pair<std::string, int>> Leaderboard::readTop(int n, const std::string& path) {
    std::vector<std::pair<std::string, int>> entries;
    std::ifstream ifs(path);
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string name, scoreStr;
        if (std::getline(iss, name, ',') && std::getline(iss, scoreStr)) {
            entries.emplace_back(name, std::stoi(scoreStr));
        }
    }
    std::sort(entries.begin(), entries.end(), [](auto& a, auto& b) { return b.second < a.second; });
    if ((int)entries.size() > n) entries.resize(n);
    return entries;
}

} // namespace bow
