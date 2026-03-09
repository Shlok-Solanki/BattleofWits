#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace bow {

struct Question {
    std::string category;
    std::string text;
    std::array<std::string, 4> options{}; // A, B, C, D
    int correctIndex{0}; // 0..3
    std::optional<std::string> hint;
};

} // namespace bow
