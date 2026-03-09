#pragma once
#include <vector>
#include "Question.h"
#include "Genre.h"

namespace bow {

class ApiClient {
public:
    // Fetch questions from OpenTDB for a given genre. Returns up to `amount` questions.
    static std::vector<Question> fetchQuestions(GenreType genre, int amount = 10);

    // Fetch and concatenate questions from multiple genres, distributing roughly evenly.
    static std::vector<Question> fetchQuestionsMulti(const std::vector<GenreType>& genres, int totalAmount);
};

} // namespace bow
