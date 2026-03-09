#pragma once
#include <string>

namespace bow {

enum class GenreType {
    Science,
    Sports,
    Movies,
    History,
    Technology,
    GeneralKnowledge,
    Literature,
    LogicPuzzles,
    Mythology,
    Geography,
    Music,
    ArtCulture,
    VideoGames,
    SpaceAstronomy,
    CurrentAffairs,
    BusinessEconomy
};

int genreToOpenTdbCategory(GenreType g);
std::string genreToString(GenreType g);

} // namespace bow
