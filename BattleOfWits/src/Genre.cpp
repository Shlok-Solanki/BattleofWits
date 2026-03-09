#include "Genre.h"

namespace bow {

int genreToOpenTdbCategory(GenreType g) {
    switch (g) {
        case GenreType::Science: return 17;            // Science & Nature
        case GenreType::Sports: return 21;             // Sports
        case GenreType::Movies: return 11;             // Film
        case GenreType::History: return 23;            // History
        case GenreType::Technology: return 18;         // Computers
        case GenreType::GeneralKnowledge: return 9;    // General Knowledge
        case GenreType::Literature: return 10;         // Books
        case GenreType::LogicPuzzles: return 19;       // Mathematics (closest fit)
        case GenreType::Mythology: return 20;          // Mythology
        case GenreType::Geography: return 22;          // Geography
        case GenreType::Music: return 12;              // Music
        case GenreType::ArtCulture: return 25;         // Art (and Culture approximation)
        case GenreType::VideoGames: return 15;         // Video Games
        case GenreType::SpaceAstronomy: return 17;     // Science & Nature (approx)
        case GenreType::CurrentAffairs: return 9;      // No direct category; fallback GK
        case GenreType::BusinessEconomy: return 9;     // No direct category; fallback GK
        default: return 9;
    }
}

std::string genreToString(GenreType g) {
    switch (g) {
        case GenreType::Science: return "Science";
        case GenreType::Sports: return "Sports";
        case GenreType::Movies: return "Movies";
        case GenreType::History: return "History";
        case GenreType::Technology: return "Technology";
        case GenreType::GeneralKnowledge: return "General Knowledge";
        case GenreType::Literature: return "Literature";
        case GenreType::LogicPuzzles: return "Logic Puzzles";
        case GenreType::Mythology: return "Mythology";
        case GenreType::Geography: return "Geography";
        case GenreType::Music: return "Music";
        case GenreType::ArtCulture: return "Art & Culture";
        case GenreType::VideoGames: return "Video Games";
        case GenreType::SpaceAstronomy: return "Space & Astronomy";
        case GenreType::CurrentAffairs: return "Current Affairs";
        case GenreType::BusinessEconomy: return "Business & Economy";
        default: return "General Knowledge";
    }
}

} // namespace bow
