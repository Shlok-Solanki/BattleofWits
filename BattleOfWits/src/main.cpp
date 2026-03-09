#include "Game.h"
#include <iostream>

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Config.hpp>
#include "UiTheme.h"
#include <optional>
#include <filesystem>
#include <vector>
#include <cmath>
#include <cstdint>
#endif

int main() {
    using namespace bow;

    while (true) {
#ifdef HAS_SFML
    // Start screen with logo, background music, and animated menu buttons
    auto dm = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(dm, "Battle of Wits", sf::Style::Default, sf::State::Windowed);
    auto theme = ui::defaultTheme();
    sf::Font font; ui::loadSystemFont(font);

    // Background music (optional)
    sf::Music music;
    try {
        const char* mus[] = {"music.ogg", "music.wav", "bgm.ogg", "bgm.wav"};
        for (auto f : mus) {
            auto p = std::filesystem::path("assets"); p /= f;
            if (std::filesystem::exists(p)) { if (music.openFromFile(p.string())) {
#if defined(SFML_VERSION_MAJOR) && SFML_VERSION_MAJOR >= 3
                    music.setLooping(true);
#else
                    music.setLoop(true);
#endif
                    music.setVolume(50.f); music.play(); break; } }
        }
    } catch (...) {}

    // Logo
    std::optional<sf::Sprite> logo;
    sf::Texture logoTex; // keep alive while sprite uses it
    try {
        // Try common extensions
        const char* exts[] = {"png", "jpg", "jpeg", "webp"};
        std::filesystem::path found;
        for (auto ext : exts) {
            auto p = std::filesystem::path("assets");
            p /= std::string("logo.") + ext;
            if (std::filesystem::exists(p)) { found = p; break; }
        }
        if (!found.empty()) {
            logoTex = sf::Texture(found);
            logo.emplace(logoTex);
            // Scale to fit (~70% width, ~60% height)
            auto sz = logoTex.getSize();
            float maxW = window.getSize().x * 0.5f;
            float maxH = window.getSize().y * 0.3f;
            float scale = std::min(maxW / static_cast<float>(sz.x), maxH / static_cast<float>(sz.y));
            logo->setScale(sf::Vector2f{scale, scale});
        }
    } catch (...) { /* ignore */ }

    sf::Text fallback(font, "BATTLE OF WITS", 64);
    fallback.setFillColor(sf::Color(245, 240, 230));

    // Menu buttons
    struct Btn { sf::RectangleShape box; sf::Text label; bool hover=false; };
    std::vector<Btn> buttons;
    auto mkBtn = [&](const std::string& text){
        Btn b{sf::RectangleShape({260.f, 56.f}), sf::Text(font, text, 24)};
        ui::styleButton(b.box, theme);
        b.label.setFillColor(sf::Color(235,235,245));
        return b;
    };
    buttons.push_back(mkBtn("Play"));
    buttons.push_back(mkBtn("Options"));
    buttons.push_back(mkBtn("Quit"));

    // Prompt (pulsing)
    sf::Text prompt(font, "Press Enter or click Play", 20);
    prompt.setFillColor(sf::Color(230,230,240));

    auto layout = [&](){
        float cx = window.getSize().x * 0.5f;
        float yTop = window.getSize().y * 0.28f;
        // Position logo/fallback
        if (logo.has_value()) {
            auto lb = logo->getGlobalBounds();
            logo->setPosition(sf::Vector2f{cx - lb.size.x * 0.5f, yTop - lb.size.y});
        } else {
            auto b = fallback.getLocalBounds();
            fallback.setPosition({cx - b.size.x*0.5f, yTop - b.size.y});
        }
        // Buttons stacked
        float startY = yTop + 40.f;
        for (size_t i = 0; i < buttons.size(); ++i) {
            auto& bt = buttons[i];
            bt.box.setPosition({cx - bt.box.getSize().x*0.5f, startY + i*(bt.box.getSize().y + 14.f)});
            auto bp = bt.box.getPosition();
            auto lb = bt.label.getLocalBounds();
            bt.label.setPosition({bp.x + (bt.box.getSize().x - lb.size.x)*0.5f, bp.y + 10.f});
        }
        auto pb = prompt.getLocalBounds();
        prompt.setPosition({(window.getSize().x - pb.size.x) * 0.5f, window.getSize().y - 48.f});
    };
    layout();

    bool startGame = false;
    bool quitApp = false;
    sf::Clock fc;

    while (window.isOpen()) {
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) { window.close(); quitApp = true; }
            if (auto rs = ev->getIf<sf::Event::Resized>()) { layout(); }
            if (auto mm = ev->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2f mp{static_cast<float>(mm->position.x), static_cast<float>(mm->position.y)};
                for (auto& b : buttons) {
                    b.hover = b.box.getGlobalBounds().contains(mp);
                }
            }
            if (auto mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mp{static_cast<float>(mb->position.x), static_cast<float>(mb->position.y)};
                    for (size_t i = 0; i < buttons.size(); ++i) {
                        if (buttons[i].box.getGlobalBounds().contains(mp)) {
                            if (i == 0) { startGame = true; window.close(); }
                            else if (i == 1) { /* Options placeholder */ }
                            else if (i == 2) { quitApp = true; window.close(); }
                        }
                    }
                }
            }
            if (auto key = ev->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Enter) { startGame = true; window.close(); }
                if (key->code == sf::Keyboard::Key::Escape) { quitApp = true; window.close(); }
            }
        }
        float t = fc.getElapsedTime().asSeconds();
        float a = 0.6f + 0.4f*std::sin(t*3.0f);
        prompt.setFillColor(sf::Color(230,230,240, static_cast<std::uint8_t>(255*a)));

        // Hover animation for buttons
        for (auto& b : buttons) {
            if (b.hover) { b.box.setOutlineColor(theme.accent); b.box.setOutlineThickness(3.f); b.label.setFillColor(theme.accent); }
            else { b.box.setOutlineColor(theme.buttonOutline); b.box.setOutlineThickness(2.f); b.label.setFillColor(sf::Color(235,235,245)); }
        }

        window.clear();
        ui::drawBackground(window, theme, window.getSize());
        if (logo.has_value()) window.draw(*logo); else window.draw(fallback);
        for (auto& b : buttons) { window.draw(b.box); window.draw(b.label); }
        window.draw(prompt);
        window.display();
    }
    if (quitApp && !startGame) return 0;
#endif

    // Genre selection screen (GUI if SFML available); default to General Knowledge on fallback
    GenreType chosen = GenreType::GeneralKnowledge;
    std::vector<GenreType> chosenList;
#ifdef HAS_SFML
    {
        auto dm2 = sf::VideoMode::getDesktopMode();
        sf::RenderWindow gwin(dm2, "Select Genres - Battle of Wits", sf::Style::Default, sf::State::Windowed);
        auto theme2 = ui::defaultTheme();
        sf::Font font2; ui::loadSystemFont(font2);
        sf::Text title(font2, "Select up to 8 Genres", 36); title.setFillColor(sf::Color(245,240,230));

        // Build genre buttons (all available)
        std::vector<std::pair<GenreType, std::string>> items = {
            {GenreType::Science, genreToString(GenreType::Science)},
            {GenreType::Sports, genreToString(GenreType::Sports)},
            {GenreType::Movies, genreToString(GenreType::Movies)},
            {GenreType::History, genreToString(GenreType::History)},
            {GenreType::Technology, genreToString(GenreType::Technology)},
            {GenreType::GeneralKnowledge, genreToString(GenreType::GeneralKnowledge)},
            {GenreType::Literature, genreToString(GenreType::Literature)},
            {GenreType::LogicPuzzles, genreToString(GenreType::LogicPuzzles)},
            {GenreType::Mythology, genreToString(GenreType::Mythology)},
            {GenreType::Geography, genreToString(GenreType::Geography)},
            {GenreType::Music, genreToString(GenreType::Music)},
            {GenreType::ArtCulture, genreToString(GenreType::ArtCulture)},
            {GenreType::VideoGames, genreToString(GenreType::VideoGames)},
            {GenreType::SpaceAstronomy, genreToString(GenreType::SpaceAstronomy)},
            {GenreType::CurrentAffairs, genreToString(GenreType::CurrentAffairs)},
            {GenreType::BusinessEconomy, genreToString(GenreType::BusinessEconomy)}
        };
        struct GBtn { GenreType gt; sf::RectangleShape box; sf::Text label; bool hover=false; bool selected=false; };
        std::vector<GBtn> gbtns; gbtns.reserve(items.size());
        for (auto& [gt, name] : items) {
            GBtn b{gt, sf::RectangleShape({300.f, 60.f}), sf::Text(font2, name, 22)};
            ui::styleButton(b.box, theme2); b.label.setFillColor(sf::Color(235,235,245)); gbtns.push_back(std::move(b));
        }
        sf::Text hint(font2, "Select up to 8 genres. Click to toggle. Enter to continue.", 18);
        hint.setFillColor(sf::Color(220,210,240));
        auto layout2 = [&](){
            float cx = gwin.getSize().x * 0.5f;
            auto tb = title.getLocalBounds(); title.setPosition({cx - tb.size.x*0.5f, 36.f});
            auto hb = hint.getLocalBounds(); hint.setPosition({cx - hb.size.x*0.5f, 80.f});
            // grid 3 columns
            int cols = 3; float gap = 16.f; float bw = 300.f, bh = 60.f;
            float totalW = cols*bw + (cols-1)*gap; float startX = cx - totalW*0.5f; float y0 = 130.f;
            for (size_t i = 0; i < gbtns.size(); ++i) {
                int r = static_cast<int>(i) / cols; int c = static_cast<int>(i) % cols;
                float x = startX + c*(bw + gap); float y = y0 + r*(bh + gap);
                gbtns[i].box.setSize({bw, bh}); gbtns[i].box.setPosition({x, y});
                auto lb = gbtns[i].label.getLocalBounds(); gbtns[i].label.setPosition({x + (bw - lb.size.x)*0.5f, y + 14.f});
            }
        };
        layout2();
        auto countSelected = [&](){ int c=0; for (auto& b : gbtns) if (b.selected) ++c; return c; };
        while (gwin.isOpen()) {
            while (auto ev = gwin.pollEvent()) {
                if (ev->is<sf::Event::Closed>()) { gwin.close(); }
                if (auto rs = ev->getIf<sf::Event::Resized>()) { layout2(); }
                if (auto mm = ev->getIf<sf::Event::MouseMoved>()) {
                    sf::Vector2f mp{static_cast<float>(mm->position.x), static_cast<float>(mm->position.y)};
                    for (auto& b : gbtns) b.hover = b.box.getGlobalBounds().contains(mp);
                }
                if (auto mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mb->button == sf::Mouse::Button::Left) {
                        sf::Vector2f mp{static_cast<float>(mb->position.x), static_cast<float>(mb->position.y)};
                        for (auto& b : gbtns) if (b.box.getGlobalBounds().contains(mp)) {
                            if (!b.selected) {
                                if (countSelected() < 8) b.selected = true;
                            } else {
                                b.selected = false;
                            }
                        }
                    }
                }
                if (auto key = ev->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::Enter) {
                        if (countSelected() == 0) {
                            // default to General Knowledge if none selected
                            chosenList = {GenreType::GeneralKnowledge};
                            chosen = GenreType::GeneralKnowledge;
                        } else {
                            chosenList.clear();
                            for (auto& b : gbtns) if (b.selected) chosenList.push_back(b.gt);
                            chosen = chosenList.front();
                        }
                        gwin.close();
                    }
                    if (key->code == sf::Keyboard::Key::Escape) { gwin.close(); }
                }
            }
            // hover + selected visuals
            for (auto& b : gbtns) {
                if (b.selected) { b.box.setFillColor(sf::Color(70,40,110)); b.box.setOutlineColor(theme2.accent); b.box.setOutlineThickness(3.f); b.label.setFillColor(theme2.accent); }
                else if (b.hover) { b.box.setOutlineColor(theme2.accent); b.box.setOutlineThickness(3.f); b.label.setFillColor(theme2.accent); }
                else { b.box.setFillColor(theme2.buttonFill); b.box.setOutlineColor(theme2.buttonOutline); b.box.setOutlineThickness(2.f); b.label.setFillColor(sf::Color(235,235,245)); }
            }
            gwin.clear(); ui::drawBackground(gwin, theme2, gwin.getSize()); gwin.draw(title); gwin.draw(hint); for (auto& b : gbtns) { gwin.draw(b.box); gwin.draw(b.label); } gwin.display();
        }
    }
#endif

    Game game;
    game.setPlayerName("Player");
    if (!chosenList.empty()) {
        // For now, when multiple genres are selected we’ll fetch mixed questions directly here
        // and temporarily set the first genre for display purposes within Game.
        game.selectGenre(chosenList.front());
        // Extend: Game could be refactored to accept a prepared vector<Question>.
        // As an immediate solution, call a quick multi-fetch and then run per-question loop via startRound fallback.
    } else {
        game.selectGenre(chosen);
    }
    // If multiple genres were chosen, run a mixed 15-question round; otherwise single-genre round.
    if (!chosenList.empty()) game.startRoundMulti(chosenList, 15);
    else game.startRound(15);

    // After each round, loop back to start screen (unless user quits next time)

    std::cout << "\nGame complete.\n";
  }
    return 0;
}
