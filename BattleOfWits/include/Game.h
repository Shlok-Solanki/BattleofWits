#pragma once
#include <string>
#include <vector>
#include "Player.h"
#include "Question.h"
#include "Genre.h"

namespace bow {

class Game {
public:
    Game();

    void setPlayerName(const std::string& name);
    void selectGenre(GenreType genre);

    // Starts a sample round: fetches 1 question and displays it (console or SFML)
    void startSampleRound();

    // Starts a round with `amount` questions (GUI if SFML available). Keeps cumulative score.
    void startRound(int amount);

    // Starts a round pulling questions from multiple genres, distributed across them.
    void startRoundMulti(const std::vector<GenreType>& genres, int amount);

private:
    Player player_{};
    GenreType selectedGenre_ = GenreType::GeneralKnowledge;
};

} // namespace bow
