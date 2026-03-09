#pragma once
#include <vector>
#include "Question.h"

namespace bow {

class Lifeline {
public:
    // Returns indices (0..3) of options to hide for 50-50
    static std::vector<int> applyFiftyFifty(const Question& q);

    // Returns percentage distribution for A,B,C,D that sums to 100
    static std::vector<int> audiencePoll(const Question& q);
};

} // namespace bow
