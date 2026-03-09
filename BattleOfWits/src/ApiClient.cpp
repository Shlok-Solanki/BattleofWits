#include "ApiClient.h"
#include "Utils.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <array>
#include <iostream>

namespace bow {

using nlohmann::json;

static Question makeQuestionFromJson(const json& j) {
    Question q;
    q.category = j.value("category", "");
    q.text = htmlDecode(j.value("question", ""));
    std::array<std::string,4> opts{};
    // correct + incorrect options
    auto correct = htmlDecode(j.value("correct_answer", ""));
    auto incorrect = j.value("incorrect_answers", std::vector<std::string>{});
    for (auto& s : incorrect) s = htmlDecode(s);
    // Ensure 4 options; pad if needed
    std::vector<std::string> all = incorrect;
    all.push_back(correct);
    while (all.size() < 4) all.push_back("");
    for (int i = 0; i < 4; ++i) opts[i] = all[i];
    int correctIdx = static_cast<int>(all.size()) - 1; // last before shuffle
    shuffleOptions(opts, correctIdx);
    q.options = opts;
    q.correctIndex = correctIdx;
    return q;
}

std::vector<Question> ApiClient::fetchQuestions(GenreType genre, int amount) {
    std::vector<Question> out;
    int category = genreToOpenTdbCategory(genre);
    // Build path: try multiple difficulty levels for variety
    std::string path = "/api.php?amount=" + std::to_string(amount) + "&type=multiple&category=" + std::to_string(category);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli("opentdb.com", 443);
#else
    httplib::Client cli("http://opentdb.com", 80);
#endif
    cli.set_read_timeout(10, 0);

    if (auto res = cli.Get(path.c_str())) {
        if (res->status == 200) {
            auto j = json::parse(res->body, nullptr, false);
            if (!j.is_discarded() && j.contains("results")) {
                for (auto& item : j["results"]) {
                    out.push_back(makeQuestionFromJson(item));
                }
            }
        }
    }
    return out;
}

std::vector<Question> ApiClient::fetchQuestionsMulti(const std::vector<GenreType>& genres, int totalAmount) {
    std::vector<Question> all;
    if (genres.empty() || totalAmount <= 0) return all;
    int n = static_cast<int>(genres.size());
    int base = totalAmount / n;
    int rem = totalAmount % n;
    for (int i = 0; i < n; ++i) {
        int amt = base + (i < rem ? 1 : 0);
        if (amt <= 0) continue;
        auto part = fetchQuestions(genres[i], amt);
        all.insert(all.end(), part.begin(), part.end());
    }
    // Top-up if we received fewer than requested (OpenTDB can under-deliver)
    if ((int)all.size() < totalAmount) {
        int need = totalAmount - static_cast<int>(all.size());
        auto extra = fetchQuestions(GenreType::GeneralKnowledge, need);
        all.insert(all.end(), extra.begin(), extra.end());
    }
    return all;
}

} // namespace bow
