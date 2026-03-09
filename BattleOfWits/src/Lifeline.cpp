#include "Lifeline.h"
#include <algorithm>
#include <numeric>
#include <random>

namespace bow {

std::vector<int> Lifeline::applyFiftyFifty(const Question& q) {
    // Hide two wrong options randomly
    std::vector<int> wrong;
    for (int i = 0; i < 4; ++i) if (i != q.correctIndex) wrong.push_back(i);
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(wrong.begin(), wrong.end(), rng);
    wrong.resize(2);
    return wrong; // indices to hide
}

std::vector<int> Lifeline::audiencePoll(const Question& q) {
    (void)q; // unused
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(10, 40);
    int correct = dist(rng) + 30; // bias towards correct
    int rem = 100 - correct;
    int a = std::uniform_int_distribution<int>(0, rem)(rng);
    int b = std::uniform_int_distribution<int>(0, rem - a)(rng);
    int c = rem - a - b;
    std::vector<int> res = {a, b, c, correct};
    std::shuffle(res.begin(), res.end(), rng);
    return res;
}

} // namespace bow
