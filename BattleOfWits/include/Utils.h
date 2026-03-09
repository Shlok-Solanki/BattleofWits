#pragma once
#include <string>
#include <vector>

namespace bow {

// Utility helpers: HTML entity decode and simple shuffling helper
std::string htmlDecode(const std::string& s);

// Fisher-Yates shuffle that tracks the index of the correct answer
void shuffleOptions(std::array<std::string,4>& options, int& correctIndex);

} // namespace bow
