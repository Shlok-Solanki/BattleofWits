#include "Game.h"
#include "ApiClient.h"
#include <iostream>

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <sstream>
#include <cstdio>
#include <optional>
#include <array>
#include <cmath>
#include <cstdint>
#include "UiTheme.h"
#include <filesystem>
#include "Lifeline.h"
#endif

namespace bow {

Game::Game() = default;

void Game::setPlayerName(const std::string& name) { player_ = Player{name}; }
void Game::selectGenre(GenreType genre) { selectedGenre_ = genre; }

void Game::startSampleRound() {
    auto qs = ApiClient::fetchQuestions(selectedGenre_, 1);
    if (qs.empty()) {
        std::cout << "Failed to fetch question.\n";
        return;
    }
    const auto& q = qs.front();

#ifndef HAS_SFML
    // Console fallback
    std::cout << "Category: " << q.category << "\n";
    std::cout << q.text << "\n";
    char labels[4] = {'A','B','C','D'};
    for (int i = 0; i < 4; ++i) {
        std::cout << labels[i] << ") " << q.options[i] << "\n";
    }
#else
    // --- SFML UI with top bar (timer, score, question number, logo) and question/options ---
    auto dm = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(dm, "Battle of Wits", sf::Style::Default, sf::State::Windowed);
    auto theme = ui::defaultTheme();

    // Font
    sf::Font font; ui::loadSystemFont(font);

    // Sounds (optional)
    sf::SoundBuffer bufClick, bufCorrect, bufWrong, bufLifeline, bufTick;
    std::optional<sf::Sound> sClick, sCorrect, sWrong, sLifeline, sTick;
    auto tryLoadSound = [&](sf::SoundBuffer& buf, const char* file) -> bool {
        auto p = std::filesystem::path("assets");
        p /= file;
        if (std::filesystem::exists(p)) return buf.loadFromFile(p.string());
        return false;
    };
    if (tryLoadSound(bufClick, "click.wav")) sClick.emplace(bufClick);
    if (tryLoadSound(bufCorrect, "correct.wav")) sCorrect.emplace(bufCorrect);
    if (tryLoadSound(bufWrong, "wrong.wav")) sWrong.emplace(bufWrong);
    if (tryLoadSound(bufLifeline, "lifeline.wav")) sLifeline.emplace(bufLifeline);
    if (tryLoadSound(bufTick, "tick.wav")) { sTick.emplace(bufTick); sTick->setVolume(70.f); }

    // Load small logo (optional)
    std::optional<sf::Sprite> logo;
    sf::Texture logoTex;
    try {
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
        }
    } catch (...) { }

    auto wrapText = [&](const std::string& s, unsigned int size, float maxWidth) {
        sf::Text t(font, "", size);
        std::string out, line;
        std::istringstream iss(s);
        std::string word;
        while (iss >> word) {
            std::string trial = line.empty() ? word : line + " " + word;
            t.setString(trial);
            if (t.getLocalBounds().size.x > maxWidth) {
                if (!line.empty()) { out += line + "\n"; line = word; }
                else { out += trial + "\n"; line.clear(); }
            } else {
                line = std::move(trial);
            }
        }
        if (!line.empty()) out += line;
        return out;
    };

    // Layout metrics
    const float margin = 24.f;
    const float topBarH = 72.f;
    const sf::Vector2f boxSize{window.getSize().x - 2*margin, 220.f};

    // Top bar background
    sf::RectangleShape topBar({static_cast<float>(window.getSize().x), topBarH});
    topBar.setPosition({0.f, 0.f});
    topBar.setFillColor(sf::Color(45, 22, 70));
    topBar.setOutlineThickness(0.f);

    // Score, question number, timer
    sf::Text scoreText(font, "Score: " + std::to_string(player_.getScore()), 20);
    scoreText.setFillColor(sf::Color(230, 230, 240));
    scoreText.setPosition({margin, 20.f});

    sf::Text qnumText(font, "Q 1/1", 20);
    qnumText.setFillColor(sf::Color(210, 200, 230));
    qnumText.setPosition({margin + 160.f, 20.f});

    sf::Text timerText(font, "00:30", 22);
    timerText.setFillColor(theme.accent);
    // Right align timer
    auto timerBounds = timerText.getLocalBounds();
    timerText.setPosition({static_cast<float>(window.getSize().x) - margin - timerBounds.size.x, 18.f});

    // Centered logo/title in top bar
    sf::Text titleFallback(font, "Battle of Wits", 22);
    titleFallback.setFillColor(sf::Color(245, 240, 230));
    auto centerX = static_cast<float>(window.getSize().x) * 0.5f;
    if (logo.has_value()) {
        float maxH = topBarH - 16.f;
        auto sz = logoTex.getSize();
        float scale = maxH / static_cast<float>(sz.y);
        logo->setScale({scale, scale});
        auto lb = logo->getGlobalBounds();
        logo->setPosition({centerX - lb.size.x * 0.5f, (topBarH - lb.size.y) * 0.5f});
    } else {
        auto b = titleFallback.getLocalBounds();
        titleFallback.setPosition({centerX - b.size.x * 0.5f, (topBarH - b.size.y) * 0.5f - 6.f});
    }

    // Question panel
    sf::RectangleShape questionBox(boxSize);
    questionBox.setPosition({margin, margin + topBarH});
    ui::stylePanel(questionBox, theme);

    sf::Text categoryText(font, "Category: " + q.category, 18);
    categoryText.setFillColor(sf::Color(180, 180, 200));
    categoryText.setPosition(sf::Vector2f{margin + 12.f, margin + 8.f + topBarH});

    sf::Text questionText(font, wrapText(q.text, 22, boxSize.x - 24.f), 22);
    questionText.setFillColor(sf::Color::White);
    questionText.setPosition(sf::Vector2f{margin + 12.f, margin + 36.f + topBarH});

    // Options as buttons
    std::array<sf::RectangleShape,4> optionRects;
    std::vector<sf::Text> optionLabels;
    optionLabels.reserve(4);
    std::array<bool,4> optionDisabled{false,false,false,false};
    std::array<int,4> audiencePct{0,0,0,0};
    const char* labels = "ABCD";
    float btnW = (boxSize.x - 24.f);
    float btnH = 56.f;
    float startY = margin + topBarH + boxSize.y + 32.f;
    for (int i = 0; i < 4; ++i) {
        optionRects[i].setSize({btnW, btnH});
        optionRects[i].setPosition({margin + 12.f, startY + i*(btnH + 16.f)});
        ui::styleButton(optionRects[i], theme);

        std::string text = std::string(1, labels[i]) + ") " + q.options[i];
        optionLabels.emplace_back(font, text, 20);
        optionLabels.back().setFillColor(sf::Color(230, 230, 240));
        auto pos = optionRects[i].getPosition();
        optionLabels.back().setPosition(sf::Vector2f{pos.x + 12.f, pos.y + 14.f});
    }

    // Lifeline panel and buttons
    enum class LType { FiftyFifty, Audience, Skip, Hint };
    struct LBtn { sf::RectangleShape rect; sf::Text label; std::string tip; bool used=false; LType type; };
    std::vector<LBtn> lifelines;
    auto mkBtn = [&](const std::string& t, LType tp){
        LBtn b{sf::RectangleShape({140.f, 44.f}), sf::Text(font, t, 18), std::string{}, false, tp};
        ui::styleButton(b.rect, theme);
        b.label.setFillColor(sf::Color(235,235,245));
        return b;
    };
    lifelines.push_back(mkBtn("50-50", LType::FiftyFifty));
    lifelines.back().tip = "Remove two wrong answers";
    lifelines.push_back(mkBtn("Audience", LType::Audience));
    lifelines.back().tip = "Show audience poll %";
    lifelines.push_back(mkBtn("Skip", LType::Skip));
    lifelines.back().tip = "Skip this question";
    lifelines.push_back(mkBtn("Hint", LType::Hint));
    lifelines.back().tip = "Show a hint (if available)";

    float rowW = lifelines.size()*140.f + (lifelines.size()-1)*12.f;
    float baseX = margin + 12.f + btnW - rowW;
    float baseY = startY - (44.f + 18.f);
    for (size_t i = 0; i < lifelines.size(); ++i) {
        lifelines[i].rect.setPosition({baseX + static_cast<float>(i)*(140.f + 12.f), baseY});
        auto p = lifelines[i].rect.getPosition();
        lifelines[i].label.setPosition({p.x + 14.f, p.y + 10.f});
    }
    sf::RectangleShape lifelinePanel({rowW + 24.f, 44.f + 24.f});
    lifelinePanel.setPosition({baseX - 12.f, baseY - 12.f});
    ui::stylePanel(lifelinePanel, theme);

    // Tooltip
    sf::Text tooltip(font, "", 16);
    tooltip.setFillColor(sf::Color(240,240,255));
    sf::RectangleShape tooltipBg({0,0});
    tooltipBg.setFillColor(sf::Color(30,20,50,220));
    tooltipBg.setOutlineThickness(1.5f);
    tooltipBg.setOutlineColor(theme.accent);
    std::optional<size_t> hoverBtn;
    sf::Vector2f mousePos{0.f,0.f};

    auto dimOption = [&](int i){
        optionDisabled[i] = true;
        optionRects[i].setFillColor(sf::Color(35, 18, 55));
        optionLabels[i].setFillColor(sf::Color(150,150,170));
    };

    auto hit = [&](const sf::RectangleShape& r, sf::Vector2f p){
        return r.getGlobalBounds().contains(p);
    };

    // Optional hint text
    std::optional<sf::Text> hintText;
    float hintAlpha = 0.f;

    // Timer
    const int totalSecs = 30;
    sf::Clock clk; // counts down
    sf::Clock frameClock; // for dt animations
    int lastRemain = totalSecs;
    int selection = -1;
    std::optional<int> hoverOption;
    float audienceAnim[4] = {0.f,0.f,0.f,0.f};
    while (window.isOpen()) {
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            if (auto m = ev->getIf<sf::Event::MouseMoved>()) {
                mousePos = {static_cast<float>(m->position.x), static_cast<float>(m->position.y)};
                hoverBtn.reset();
                hoverOption.reset();
                for (size_t i = 0; i < lifelines.size(); ++i) if (hit(lifelines[i].rect, mousePos)) { hoverBtn = i; break; }
                for (int i = 0; i < 4; ++i) if (!optionDisabled[i] && hit(optionRects[i], mousePos)) { hoverOption = i; break; }
            }
            if (auto m = ev->getIf<sf::Event::MouseButtonPressed>()) {
                if (m->button == sf::Mouse::Button::Left) {
                    sf::Vector2f p{static_cast<float>(m->position.x), static_cast<float>(m->position.y)};
                    // Lifelines first
                    for (auto & b : lifelines) b.rect.setOutlineColor(theme.buttonOutline);
                    bool lifelineConsumed = false;
                    if (sClick) sClick->play();
                    for (size_t i = 0; i < lifelines.size(); ++i) {
                        auto &b = lifelines[i];
                        if (hit(b.rect, p) && !b.used) {
                            b.used = true; b.rect.setOutlineColor(theme.accent);
                            switch (b.type) {
                                case LType::FiftyFifty: {
                                    auto hide = Lifeline::applyFiftyFifty(q);
                                    for (int idx : hide) dimOption(idx);
                                    lifelineConsumed = true;
                                } break;
                                case LType::Audience: {
                                    auto dist = Lifeline::audiencePoll(q);
                                    for (int k = 0; k < 4 && k < (int)dist.size(); ++k) { audiencePct[k] = dist[k]; audienceAnim[k] = 0.f; }
                                    lifelineConsumed = true;
                                } break;
                                case LType::Skip: {
                                    lifelineConsumed = true;
                                    window.close();
                                } break;
                                case LType::Hint: {
                                    if (q.hint.has_value() && !q.hint->empty()) {
                                        std::string wrapped = wrapText(*q.hint, 18, boxSize.x - 24.f);
                                        hintText.emplace(font, "Hint: " + wrapped, 18);
                                        auto y = questionText.getPosition().y + questionText.getLocalBounds().size.y + 8.f;
                                        hintText->setPosition({questionText.getPosition().x, y});
                                        hintAlpha = 0.f;
                                    }
                                    lifelineConsumed = true;
                                } break;
                            }
                        }
                    }
                    if (!lifelineConsumed) {
                        for (int i = 0; i < 4; ++i) {
                            if (optionDisabled[i]) continue;
                            if (hit(optionRects[i], p)) {
                                selection = i;
                                for (int j = 0; j < 4; ++j) {
                                    optionRects[j].setOutlineColor(theme.buttonOutline);
                                }
                                optionRects[i].setOutlineColor(theme.accent);
                                break;
                            }
                        }
                    }
                    if (lifelineConsumed && sLifeline) sLifeline->play();
                }
            }
            if (auto key = ev->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Enter) {
                    window.close();
                }
            }
        }

        // Hover visuals for lifelines
        for (size_t i = 0; i < lifelines.size(); ++i) {
            auto &b = lifelines[i];
            if (b.used) { b.rect.setFillColor(sf::Color(40, 25, 60)); b.label.setFillColor(sf::Color(170,170,190)); b.rect.setOutlineColor(theme.buttonOutline); b.rect.setOutlineThickness(2.f); }
            else if (hoverBtn && *hoverBtn == i) { b.rect.setOutlineColor(theme.accent); b.label.setFillColor(theme.accent); b.rect.setOutlineThickness(3.f); }
            else { b.rect.setOutlineColor(theme.buttonOutline); b.label.setFillColor(sf::Color(235,235,245)); b.rect.setFillColor(theme.buttonFill); b.rect.setOutlineThickness(2.f); }
        }

        // Update timer display
        float dt = frameClock.restart().asSeconds();
        int elapsed = static_cast<int>(clk.getElapsedTime().asSeconds());
        int remain = std::max(0, totalSecs - elapsed);
        if (remain < lastRemain && sTick && remain <= 5) sTick->play();
        lastRemain = remain;
        int mm = remain / 60, ss = remain % 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
        timerText.setString(buf);
        auto tb = timerText.getLocalBounds();
        timerText.setPosition({static_cast<float>(window.getSize().x) - margin - tb.size.x, 18.f});
        if (remain <= 5) {
            float t = clk.getElapsedTime().asSeconds();
            float a = 0.5f + 0.5f*std::sin(t*8.f);
sf::Color c(255, static_cast<std::uint8_t>(120 + 100*a), static_cast<std::uint8_t>(120 + 100*a));
            timerText.setFillColor(c);
        } else timerText.setFillColor(theme.accent);

        // Animate hint fade-in
        if (hintText) {
            hintAlpha = std::min(1.f, hintAlpha + dt*3.f);
auto col = sf::Color(210,200,230, static_cast<std::uint8_t>(255*hintAlpha));
            hintText->setFillColor(col);
        }

        // Animate audience percentages
        for (int i = 0; i < 4; ++i) if (audiencePct[i] > 0) audienceAnim[i] = std::min(1.f, audienceAnim[i] + dt*3.f);

        if (remain == 0 && selection == -1) {
            // Time's up – flash and close
            window.clear();
            ui::drawBackground(window, theme, window.getSize());
            window.draw(topBar);
            window.draw(scoreText);
            window.draw(qnumText);
            if (logo.has_value()) window.draw(*logo); else window.draw(titleFallback);
            window.draw(questionBox);
            window.draw(categoryText);
            window.draw(questionText);
            if (hintText) window.draw(*hintText);
            for (int i = 0; i < 4; ++i) { window.draw(optionRects[i]); window.draw(optionLabels[i]); }
            window.draw(timerText);
            window.display();
            sf::sleep(sf::milliseconds(600));
            window.close();
        }

        window.clear();
        ui::drawBackground(window, theme, window.getSize());
        window.draw(topBar);
        window.draw(scoreText);
        window.draw(qnumText);
        if (logo.has_value()) window.draw(*logo); else window.draw(titleFallback);
        window.draw(questionBox);
        window.draw(categoryText);
        window.draw(questionText);
        if (hintText) window.draw(*hintText);
        // Lifeline panel
        window.draw(lifelinePanel);
        for (auto &b : lifelines) { window.draw(b.rect); window.draw(b.label); }
        // Options and audience %
        for (int i = 0; i < 4; ++i) {
            // Hover accent
            if (!optionDisabled[i] && hoverOption && *hoverOption == i && selection == -1) {
                optionRects[i].setOutlineThickness(3.f);
                optionRects[i].setOutlineColor(theme.accent);
            } else {
                optionRects[i].setOutlineThickness(2.f);
            }
            window.draw(optionRects[i]);
            window.draw(optionLabels[i]);
            if (audiencePct[i] > 0) {
                sf::Text pct(font, std::to_string(audiencePct[i]) + "%", 18);
auto alpha = static_cast<std::uint8_t>(255 * audienceAnim[i]);
                pct.setFillColor(sf::Color(220, 210, 240, alpha));
                auto r = optionRects[i].getGlobalBounds();
                pct.setPosition({r.position.x + r.size.x - 48.f, r.position.y + 14.f});
                float s = 0.8f + 0.2f * audienceAnim[i];
                pct.setScale({s, s});
                window.draw(pct);
            }
        }
        window.draw(timerText);

        // Tooltip draw
        if (hoverBtn && !lifelines[*hoverBtn].used) {
            tooltip.setString(lifelines[*hoverBtn].tip);
            auto b = tooltip.getLocalBounds();
            tooltipBg.setSize({b.size.x + 16.f, b.size.y + 12.f});
            sf::Vector2f pos = {mousePos.x + 16.f, mousePos.y + 16.f};
            tooltipBg.setPosition(pos);
            tooltip.setPosition({pos.x + 8.f, pos.y + 6.f});
            window.draw(tooltipBg);
            window.draw(tooltip);
        }

        window.display();

        // If user clicked, show correctness and exit after a short delay
        if (selection != -1) {
            bool correct = (selection == q.correctIndex);
            if (sClick) sClick->play();
            if (correct) player_.addScore(10);
            if (correct && sCorrect) sCorrect->play();
            if (!correct && sWrong) sWrong->play();
            scoreText.setString("Score: " + std::to_string(player_.getScore()));
            optionRects[selection].setFillColor(correct ? sf::Color(30,120,60) : sf::Color(120,30,30));
            window.clear();
            ui::drawBackground(window, theme, window.getSize());
            window.draw(topBar);
            window.draw(scoreText);
            window.draw(qnumText);
            if (logo.has_value()) window.draw(*logo); else window.draw(titleFallback);
            window.draw(questionBox);
            window.draw(categoryText);
            window.draw(questionText);
            if (hintText) window.draw(*hintText);
            for (int i = 0; i < 4; ++i) { window.draw(optionRects[i]); window.draw(optionLabels[i]); }
            window.draw(timerText);
            window.display();
            sf::sleep(sf::milliseconds(900));
            window.close();
        }
    }
#endif
}


void Game::startRound(int amount) {
    if (amount <= 0) amount = 1;
    auto qs = ApiClient::fetchQuestions(selectedGenre_, amount);
    if (qs.empty()) {
        std::cout << "Failed to fetch questions.\n";
        return;
    }
    int total = static_cast<int>(qs.size());
    for (int idx = 0; idx < total; ++idx) {
        const auto& q = qs[idx];
#ifndef HAS_SFML
        // Console fallback per-question
        std::cout << "Q " << (idx+1) << "/" << total << "\n";
        std::cout << "Category: " << q.category << "\n";
        std::cout << q.text << "\n";
        char labels[4] = {'A','B','C','D'};
        for (int i = 0; i < 4; ++i) {
            std::cout << labels[i] << ") " << q.options[i] << "\n";
        }
#else
        // Render one question using the same UI as sample round, but with dynamic Q/total and persistent score
        auto dm = sf::VideoMode::getDesktopMode();
        sf::RenderWindow window(dm, "Battle of Wits", sf::Style::Default, sf::State::Windowed);
        auto theme = ui::defaultTheme();

        sf::Font font; ui::loadSystemFont(font);

        // Optional small logo in top bar
        std::optional<sf::Sprite> logo; sf::Texture logoTex;
        try {
            const char* exts[] = {"png","jpg","jpeg","webp"};
            std::filesystem::path found;
            for (auto ext : exts) { auto p = std::filesystem::path("assets"); p /= std::string("logo.") + ext; if (std::filesystem::exists(p)) { found = p; break; } }
            if (!found.empty()) { logoTex = sf::Texture(found); logo.emplace(logoTex); }
        } catch (...) {}

        auto wrapText = [&](const std::string& s, unsigned int size, float maxWidth) {
            sf::Text t(font, "", size);
            std::string out, line; std::istringstream iss(s); std::string word;
            while (iss >> word) {
                std::string trial = line.empty()? word : line + " " + word;
                t.setString(trial);
                if (t.getLocalBounds().size.x > maxWidth) { if (!line.empty()) { out += line + "\n"; line = word; } else { out += trial + "\n"; line.clear(); } }
                else { line = std::move(trial); }
            }
            if (!line.empty()) out += line; return out;
        };

        const float margin = 24.f; const float topBarH = 72.f;
        const sf::Vector2f boxSize{window.getSize().x - 2*margin, 220.f};
        sf::RectangleShape topBar({static_cast<float>(window.getSize().x), topBarH});
        topBar.setPosition({0.f,0.f}); topBar.setFillColor(sf::Color(45,22,70));

        sf::Text scoreText(font, "Score: " + std::to_string(player_.getScore()), 20);
        scoreText.setFillColor(sf::Color(230,230,240)); scoreText.setPosition({margin,20.f});
        sf::Text qnumText(font, "Q " + std::to_string(idx+1) + "/" + std::to_string(total), 20);
        qnumText.setFillColor(sf::Color(210,200,230)); qnumText.setPosition({margin + 160.f, 20.f});
        sf::Text timerText(font, "00:30", 22); timerText.setFillColor(theme.accent);
        auto tb = timerText.getLocalBounds(); timerText.setPosition({static_cast<float>(window.getSize().x) - margin - tb.size.x, 18.f});

        sf::Text titleFallback(font, "Battle of Wits", 22); titleFallback.setFillColor(sf::Color(245,240,230));
        auto centerX = static_cast<float>(window.getSize().x) * 0.5f;
        if (logo.has_value()) { float maxH = topBarH - 16.f; auto sz = logoTex.getSize(); float scale = maxH / static_cast<float>(sz.y); logo->setScale({scale,scale}); auto lb = logo->getGlobalBounds(); logo->setPosition({centerX - lb.size.x*0.5f, (topBarH - lb.size.y)*0.5f}); }
        else { auto b = titleFallback.getLocalBounds(); titleFallback.setPosition({centerX - b.size.x*0.5f, (topBarH - b.size.y)*0.5f - 6.f}); }

        sf::RectangleShape questionBox(boxSize); questionBox.setPosition({margin, margin + topBarH}); ui::stylePanel(questionBox, theme);
        sf::Text categoryText(font, "Category: " + q.category, 18); categoryText.setFillColor(sf::Color(180,180,200)); categoryText.setPosition({margin + 12.f, margin + 8.f + topBarH});
        sf::Text questionText(font, wrapText(q.text, 22, boxSize.x - 24.f), 22); questionText.setFillColor(sf::Color::White); questionText.setPosition({margin + 12.f, margin + 36.f + topBarH});

        std::array<sf::RectangleShape,4> optionRects; std::vector<sf::Text> optionLabels; optionLabels.reserve(4);
        std::array<bool,4> optionDisabled{false,false,false,false}; std::array<int,4> audiencePct{0,0,0,0};
        const char* labels = "ABCD"; float btnW = (boxSize.x - 24.f); float btnH = 56.f; float startY = margin + topBarH + boxSize.y + 32.f;
        for (int i = 0; i < 4; ++i) { optionRects[i].setSize({btnW, btnH}); optionRects[i].setPosition({margin + 12.f, startY + i*(btnH + 16.f)}); ui::styleButton(optionRects[i], theme); std::string text = std::string(1, labels[i]) + ") " + q.options[i]; optionLabels.emplace_back(font, text, 20); optionLabels.back().setFillColor(sf::Color(230,230,240)); auto pos = optionRects[i].getPosition(); optionLabels.back().setPosition({pos.x + 12.f, pos.y + 14.f}); }

        enum class LType { FiftyFifty, Audience, Skip, Hint };
        struct LBtn { sf::RectangleShape rect; sf::Text label; std::string tip; bool used=false; LType type; };
        std::vector<LBtn> lifelines; auto mkBtn = [&](const std::string& t, LType tp){ LBtn b{sf::RectangleShape({140.f,44.f}), sf::Text(font, t, 18), std::string{}, false, tp}; ui::styleButton(b.rect, theme); b.label.setFillColor(sf::Color(235,235,245)); return b; };
        lifelines.push_back(mkBtn("50-50", LType::FiftyFifty)); lifelines.back().tip = "Remove two wrong answers";
        lifelines.push_back(mkBtn("Audience", LType::Audience)); lifelines.back().tip = "Show audience poll %";
        lifelines.push_back(mkBtn("Skip", LType::Skip)); lifelines.back().tip = "Skip this question";
        lifelines.push_back(mkBtn("Hint", LType::Hint)); lifelines.back().tip = "Show a hint (if available)";
        float rowW = lifelines.size()*140.f + (lifelines.size()-1)*12.f; float baseX = margin + 12.f + btnW - rowW; float baseY = startY - (44.f + 18.f);
        for (size_t i = 0; i < lifelines.size(); ++i) { lifelines[i].rect.setPosition({baseX + static_cast<float>(i)*(140.f + 12.f), baseY}); auto p = lifelines[i].rect.getPosition(); lifelines[i].label.setPosition({p.x + 14.f, p.y + 10.f}); }
        sf::RectangleShape lifelinePanel({rowW + 24.f, 44.f + 24.f}); lifelinePanel.setPosition({baseX - 12.f, baseY - 12.f}); ui::stylePanel(lifelinePanel, theme);

        sf::Text tooltip(font, "", 16); tooltip.setFillColor(sf::Color(240,240,255)); sf::RectangleShape tooltipBg({0,0}); tooltipBg.setFillColor(sf::Color(30,20,50,220)); tooltipBg.setOutlineThickness(1.5f); tooltipBg.setOutlineColor(theme.accent); std::optional<size_t> hoverBtn; sf::Vector2f mousePos{0.f,0.f};
        auto dimOption = [&](int i){ optionDisabled[i] = true; optionRects[i].setFillColor(sf::Color(35,18,55)); optionLabels[i].setFillColor(sf::Color(150,150,170)); };
        auto hit = [&](const sf::RectangleShape& r, sf::Vector2f p){ return r.getGlobalBounds().contains(p); };
        std::optional<sf::Text> hintText; float hintAlpha = 0.f;

        const int totalSecs = 30; sf::Clock clk; sf::Clock frameClock; int lastRemain = totalSecs; int selection = -1; std::optional<int> hoverOption; float audienceAnim[4] = {0.f,0.f,0.f,0.f};

        while (window.isOpen()) {
            while (auto ev = window.pollEvent()) {
                if (ev->is<sf::Event::Closed>()) window.close();
                if (auto m = ev->getIf<sf::Event::MouseMoved>()) { mousePos = {static_cast<float>(m->position.x), static_cast<float>(m->position.y)}; hoverBtn.reset(); hoverOption.reset(); for (size_t i = 0; i < lifelines.size(); ++i) if (hit(lifelines[i].rect, mousePos)) { hoverBtn = i; break; } for (int i = 0; i < 4; ++i) if (!optionDisabled[i] && hit(optionRects[i], mousePos)) { hoverOption = i; break; } }
                if (auto m = ev->getIf<sf::Event::MouseButtonPressed>()) {
                    if (m->button == sf::Mouse::Button::Left) {
                        sf::Vector2f p{static_cast<float>(m->position.x), static_cast<float>(m->position.y)};
                        for (auto & b : lifelines) b.rect.setOutlineColor(theme.buttonOutline);
                        bool lifelineConsumed = false;
                        for (size_t i = 0; i < lifelines.size(); ++i) {
                            auto &b = lifelines[i];
                            if (hit(b.rect, p) && !b.used) {
                                b.used = true; b.rect.setOutlineColor(theme.accent);
                                switch (b.type) {
                                    case LType::FiftyFifty: { auto hide = Lifeline::applyFiftyFifty(q); for (int idx2 : hide) dimOption(idx2); lifelineConsumed = true; } break;
                                    case LType::Audience: { auto dist = Lifeline::audiencePoll(q); for (int k = 0; k < 4 && k < (int)dist.size(); ++k) { audiencePct[k] = dist[k]; audienceAnim[k] = 0.f; } lifelineConsumed = true; } break;
                                    case LType::Skip: { lifelineConsumed = true; window.close(); } break;
                                    case LType::Hint: { if (q.hint.has_value() && !q.hint->empty()) { std::string wrapped = wrapText(*q.hint, 18, boxSize.x - 24.f); hintText.emplace(font, "Hint: " + wrapped, 18); auto y = questionText.getPosition().y + questionText.getLocalBounds().size.y + 8.f; hintText->setPosition({questionText.getPosition().x, y}); hintAlpha = 0.f; } lifelineConsumed = true; } break;
                                }
                            }
                        }
                        if (!lifelineConsumed) {
                            for (int i = 0; i < 4; ++i) { if (optionDisabled[i]) continue; if (hit(optionRects[i], p)) { selection = i; for (int j = 0; j < 4; ++j) { optionRects[j].setOutlineColor(theme.buttonOutline); } optionRects[i].setOutlineColor(theme.accent); break; } }
                        }
                    }
                }
                if (auto key = ev->getIf<sf::Event::KeyPressed>()) { if (key->code == sf::Keyboard::Key::Enter) { window.close(); } }
            }

            // Hover visuals
            for (size_t i = 0; i < lifelines.size(); ++i) { auto &b = lifelines[i]; if (b.used) { b.rect.setFillColor(sf::Color(40,25,60)); b.label.setFillColor(sf::Color(170,170,190)); b.rect.setOutlineColor(theme.buttonOutline); b.rect.setOutlineThickness(2.f);} else if (hoverBtn && *hoverBtn == i) { b.rect.setOutlineColor(theme.accent); b.label.setFillColor(theme.accent); b.rect.setOutlineThickness(3.f);} else { b.rect.setOutlineColor(theme.buttonOutline); b.label.setFillColor(sf::Color(235,235,245)); b.rect.setFillColor(theme.buttonFill); b.rect.setOutlineThickness(2.f);} }

            // Timer and small animations
            float dt = frameClock.restart().asSeconds(); int elapsed = static_cast<int>(clk.getElapsedTime().asSeconds()); int remain = std::max(0, 30 - elapsed);
            int mm = remain / 60, ss = remain % 60; char buf[16]; std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss); timerText.setString(buf); auto tb2 = timerText.getLocalBounds(); timerText.setPosition({static_cast<float>(window.getSize().x) - margin - tb2.size.x, 18.f}); if (remain <= 5) { float t = clk.getElapsedTime().asSeconds(); float a = 0.5f + 0.5f*std::sin(t*8.f); sf::Color c(255, static_cast<std::uint8_t>(120 + 100*a), static_cast<std::uint8_t>(120 + 100*a)); timerText.setFillColor(c);} else timerText.setFillColor(theme.accent);
            if (hintText) { hintAlpha = std::min(1.f, hintAlpha + dt*3.f); auto col = sf::Color(210,200,230, static_cast<std::uint8_t>(255*hintAlpha)); hintText->setFillColor(col);} for (int i = 0; i < 4; ++i) if (audiencePct[i] > 0) ;

            if (remain == 0 && selection == -1) { window.clear(); ui::drawBackground(window, theme, window.getSize()); window.draw(topBar); window.draw(scoreText); window.draw(qnumText); if (logo.has_value()) window.draw(*logo); else window.draw(titleFallback); window.draw(questionBox); window.draw(categoryText); window.draw(questionText); if (hintText) window.draw(*hintText); for (int i = 0; i < 4; ++i) { window.draw(optionRects[i]); window.draw(optionLabels[i]); } window.draw(timerText); window.display(); sf::sleep(sf::milliseconds(600)); window.close(); }

            window.clear(); ui::drawBackground(window, theme, window.getSize()); window.draw(topBar); window.draw(scoreText); window.draw(qnumText); if (logo.has_value()) window.draw(*logo); else window.draw(titleFallback); window.draw(questionBox); window.draw(categoryText); window.draw(questionText); if (hintText) window.draw(*hintText); window.draw(lifelinePanel); for (auto &b : lifelines) { window.draw(b.rect); window.draw(b.label);} for (int i = 0; i < 4; ++i) { if (!optionDisabled[i] && hoverOption && *hoverOption == i && selection == -1) { optionRects[i].setOutlineThickness(3.f); optionRects[i].setOutlineColor(theme.accent);} else { optionRects[i].setOutlineThickness(2.f);} window.draw(optionRects[i]); window.draw(optionLabels[i]); if (audiencePct[i] > 0) { sf::Text pct(font, std::to_string(audiencePct[i]) + "%", 18); pct.setFillColor(sf::Color(220,210,240)); auto r = optionRects[i].getGlobalBounds(); pct.setPosition({r.position.x + r.size.x - 48.f, r.position.y + 14.f}); window.draw(pct);} }
            window.draw(timerText);
            if (hoverBtn && !lifelines[*hoverBtn].used) { tooltip.setString(lifelines[*hoverBtn].tip); auto b = tooltip.getLocalBounds(); tooltipBg.setSize({b.size.x + 16.f, b.size.y + 12.f}); sf::Vector2f pos = {mousePos.x + 16.f, mousePos.y + 16.f}; tooltipBg.setPosition(pos); tooltip.setPosition({pos.x + 8.f, pos.y + 6.f}); window.draw(tooltipBg); window.draw(tooltip);} window.display();

            if (selection != -1) { bool correct = (selection == q.correctIndex); if (correct) player_.addScore(10); scoreText.setString("Score: " + std::to_string(player_.getScore())); optionRects[selection].setFillColor(correct ? sf::Color(30,120,60) : sf::Color(120,30,30)); window.clear(); ui::drawBackground(window, theme, window.getSize()); window.draw(topBar); window.draw(scoreText); window.draw(qnumText); if (logo.has_value()) window.draw(*logo); else window.draw(titleFallback); window.draw(questionBox); window.draw(categoryText); window.draw(questionText); if (hintText) window.draw(*hintText); for (int i = 0; i < 4; ++i) { window.draw(optionRects[i]); window.draw(optionLabels[i]); } window.draw(timerText); window.display(); sf::sleep(sf::milliseconds(900)); window.close(); }
        }
#endif
    }
}


void Game::startRoundMulti(const std::vector<GenreType>& genres, int amount) {
    auto qs = ApiClient::fetchQuestionsMulti(genres, amount);
    if (qs.empty()) {
        std::cout << "Failed to fetch questions.\n";
        return;
    }
    const int total = std::min((int)qs.size(), amount);
#ifndef HAS_SFML
    for (int idx = 0; idx < total; ++idx) {
        const auto& q = qs[idx];
        std::cout << "Q " << (idx+1) << "/" << total << "\n";
        std::cout << "Category: " << q.category << "\n";
        std::cout << q.text << "\n";
        char labels[4] = {'A','B','C','D'};
        for (int i = 0; i < 4; ++i) std::cout << labels[i] << ") " << q.options[i] << "\n";
    }
    return;
#else
    // Single persistent window with lifelines and right-side ladder
    auto dm = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(dm, "Battle of Wits", sf::Style::Default, sf::State::Windowed);
    auto theme = ui::defaultTheme();
    sf::Font font; ui::loadSystemFont(font);

    // Logo (optional)
    std::optional<sf::Sprite> logo; sf::Texture logoTex;
    try { const char* exts[] = {"png","jpg","jpeg","webp"}; std::filesystem::path found; for (auto ext : exts) { auto p = std::filesystem::path("assets"); p /= std::string("logo.") + ext; if (std::filesystem::exists(p)) { found = p; break; } } if (!found.empty()) { logoTex = sf::Texture(found); logo.emplace(logoTex); } } catch (...) {}

    auto wrapText = [&](const std::string& s, unsigned int size, float maxWidth) {
        sf::Text t(font, "", size); std::string out, line; std::istringstream iss(s); std::string word;
        while (iss >> word) { std::string trial = line.empty()? word : line + " " + word; t.setString(trial); if (t.getLocalBounds().size.x > maxWidth) { if (!line.empty()) { out += line + "\n"; line = word; } else { out += trial + "\n"; line.clear(); } } else { line = std::move(trial);} }
        if (!line.empty()) out += line; return out;
    };

    // Ladder (15 levels) and values
    const std::vector<std::string> ladder = {
        "1,000","2,000","3,000","5,000","10,000","20,000","40,000","80,000","1,60,000","3,20,000","6,40,000","12,50,000","25,00,000","50,00,000","1 Crore"
    };
    const std::vector<int> ladderVal = {1000,2000,3000,5000,10000,20000,40000,80000,160000,320000,640000,1250000,2500000,5000000,10000000};

    // Layout constants
    const float margin = 24.f; const float topBarH = 72.f; const float sidebarW = 260.f;
    const sf::Vector2f contentBoxSize{window.getSize().x - sidebarW - 3*margin, 220.f};

    // Top bar
    sf::RectangleShape topBar({static_cast<float>(window.getSize().x), topBarH}); topBar.setPosition({0.f,0.f}); topBar.setFillColor(sf::Color(45,22,70));
    sf::Text scoreText(font, "Score: 0", 20); scoreText.setFillColor(sf::Color(230,230,240)); scoreText.setPosition({margin,20.f});
    sf::Text qnumText(font, "Q 1/" + std::to_string(total), 20); qnumText.setFillColor(sf::Color(210,200,230)); qnumText.setPosition({margin + 160.f, 20.f});
    sf::Text timerText(font, "00:30", 22); timerText.setFillColor(theme.accent);

    sf::Text titleFallback(font, "Battle of Wits", 22); titleFallback.setFillColor(sf::Color(245,240,230));

    // Question panel (left)
    sf::RectangleShape questionBox(contentBoxSize); questionBox.setPosition({margin, margin + topBarH}); ui::stylePanel(questionBox, theme);
    sf::Text categoryText(font, "", 18); categoryText.setFillColor(sf::Color(180,180,200));
    sf::Text questionText(font, "", 22); questionText.setFillColor(sf::Color::White);

    // Options
    std::array<sf::RectangleShape,4> optionRects; std::vector<sf::Text> optionLabels; // filled per-question
    const char* labels = "ABCD"; float btnW = (contentBoxSize.x - 24.f); float btnH = 56.f; float startYBase = margin + topBarH + contentBoxSize.y + 32.f;

    // Lifelines (persistent across questions)
    enum class LType { FiftyFifty, Audience, Skip, Hint };
    struct LBtn { sf::RectangleShape rect; sf::Text label; std::string tip; bool used=false; LType type; };
    std::vector<LBtn> lifelines; auto mkBtn = [&](const std::string& t, LType tp){ LBtn b{sf::RectangleShape({120.f,44.f}), sf::Text(font, t, 18), std::string{}, false, tp}; ui::styleButton(b.rect, theme); b.label.setFillColor(sf::Color(235,235,245)); return b; };
    lifelines.push_back(mkBtn("50-50", LType::FiftyFifty)); lifelines.back().tip = "Remove two wrong answers";
    lifelines.push_back(mkBtn("Audience", LType::Audience)); lifelines.back().tip = "Show audience poll %";
    lifelines.push_back(mkBtn("Skip", LType::Skip)); lifelines.back().tip = "Skip this question";
    lifelines.push_back(mkBtn("Hint", LType::Hint)); lifelines.back().tip = "Show a hint (if available)";

    // Sidebar ladder panel (right)
    sf::RectangleShape sidePanel({sidebarW, static_cast<float>(window.getSize().y) - topBarH});
    sidePanel.setPosition({static_cast<float>(window.getSize().x) - sidebarW, topBarH}); ui::stylePanel(sidePanel, theme);

    // Tooltip for lifelines
    sf::Text tooltip(font, "", 16); tooltip.setFillColor(sf::Color(240,240,255)); sf::RectangleShape tooltipBg({0,0}); tooltipBg.setFillColor(sf::Color(30,20,50,220)); tooltipBg.setOutlineThickness(1.5f); tooltipBg.setOutlineColor(theme.accent);

    // Per-question variables
    auto resetQuestion = [&](int idx){
        const auto& q = qs[idx];
        categoryText.setString("Category: " + q.category);
        categoryText.setPosition({margin + 12.f, margin + 8.f + topBarH});
        questionText.setString(wrapText(q.text, 22, contentBoxSize.x - 24.f));
        questionText.setPosition({margin + 12.f, margin + 36.f + topBarH});
        // Options
        float startY = startYBase;
        optionLabels.clear(); optionLabels.reserve(4);
        for (int i = 0; i < 4; ++i) {
            optionRects[i].setSize({btnW, btnH}); optionRects[i].setPosition({margin + 12.f, startY + i*(btnH + 16.f)}); ui::styleButton(optionRects[i], theme);
            std::string text = std::string(1, labels[i]) + ") " + q.options[i]; optionLabels.emplace_back(font, text, 20); optionLabels.back().setFillColor(sf::Color(230,230,240)); auto pos = optionRects[i].getPosition(); optionLabels.back().setPosition({pos.x + 12.f, pos.y + 14.f});
        }
    };

    // Mouse/hover
    std::optional<size_t> hoverBtn; sf::Vector2f mousePos{0.f,0.f}; std::optional<int> hoverOption;

    // Audience pct & 50-50 per-question state
    std::array<int,4> audiencePct{0,0,0,0}; std::array<bool,4> optionDisabled{false,false,false,false}; std::optional<sf::Text> hintText; float hintAlpha = 0.f;

    int current = 0; bool endRound = false; sf::Clock frameClock; sf::Clock timerClk; const int totalSecs = 30;

    auto layoutTop = [&](){ auto tb = timerText.getLocalBounds(); timerText.setPosition({static_cast<float>(window.getSize().x) - sidebarW - margin - tb.size.x, 18.f}); };

    // Set static positions
    if (logo) {
        float maxH = topBarH - 16.f; auto sz = logoTex.getSize(); float scale = maxH / static_cast<float>(sz.y); logo->setScale({scale,scale}); auto lb = logo->getGlobalBounds(); logo->setPosition({(window.getSize().x - sidebarW)*0.5f - lb.size.x*0.5f, (topBarH - lb.size.y)*0.5f});
    } else { auto b = titleFallback.getLocalBounds(); titleFallback.setPosition({(window.getSize().x - sidebarW)*0.5f - b.size.x*0.5f, (topBarH - b.size.y)*0.5f - 6.f}); }

    // Place lifelines above options, right-aligned within content area
    auto placeLifelines = [&](){ float rowW = lifelines.size()*120.f + (lifelines.size()-1)*10.f; float baseX = margin + 12.f + btnW - rowW; float baseY = startYBase - (44.f + 18.f); for (size_t i = 0; i < lifelines.size(); ++i) { lifelines[i].rect.setPosition({baseX + static_cast<float>(i)*(120.f + 10.f), baseY}); auto p = lifelines[i].rect.getPosition(); lifelines[i].label.setPosition({p.x + 12.f, p.y + 10.f}); } };
    placeLifelines();

    resetQuestion(current); qnumText.setString("Q " + std::to_string(current+1) + "/" + std::to_string(total)); timerClk.restart(); layoutTop();

    while (window.isOpen()) {
        // Events
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) { window.close(); endRound = true; }
            if (auto rs = ev->getIf<sf::Event::Resized>()) { /* simple: ignore dynamic resize */ }
            if (auto mm = ev->getIf<sf::Event::MouseMoved>()) { mousePos = {static_cast<float>(mm->position.x), static_cast<float>(mm->position.y)}; hoverBtn.reset(); hoverOption.reset(); for (size_t i = 0; i < lifelines.size(); ++i) if (lifelines[i].rect.getGlobalBounds().contains(mousePos)) { hoverBtn = i; break; } for (int i = 0; i < 4; ++i) if (!optionDisabled[i] && optionRects[i].getGlobalBounds().contains(mousePos)) { hoverOption = i; break; } }
            if (auto mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    sf::Vector2f p{static_cast<float>(mb->position.x), static_cast<float>(mb->position.y)};
                    // Lifelines
                    for (size_t i = 0; i < lifelines.size(); ++i) {
                        auto &b = lifelines[i];
                        if (b.rect.getGlobalBounds().contains(p) && !b.used) {
                            b.used = true; b.rect.setOutlineColor(theme.accent);
                            const auto& q = qs[current];
                            if (b.type == LType::FiftyFifty) { auto hide = Lifeline::applyFiftyFifty(q); for (int idx : hide) { optionDisabled[idx] = true; optionRects[idx].setFillColor(sf::Color(35,18,55)); optionLabels[idx].setFillColor(sf::Color(150,150,170)); } }
                            else if (b.type == LType::Audience) { auto dist = Lifeline::audiencePoll(q); for (int k=0;k<4 && k<(int)dist.size();++k) audiencePct[k]=dist[k]; }
                            else if (b.type == LType::Skip) { // skip question as wrong but do not end round
                                current++;
                                if (current >= total) { endRound = true; window.close(); break; }
                                audiencePct = {0,0,0,0}; optionDisabled = {false,false,false,false}; hintText.reset(); hintAlpha=0.f; resetQuestion(current); qnumText.setString("Q " + std::to_string(current+1) + "/" + std::to_string(total)); timerClk.restart();
                            }
                            else if (b.type == LType::Hint) { const auto& q2 = qs[current]; if (q2.hint) { std::string wrapped = wrapText(*q2.hint, 18, contentBoxSize.x - 24.f); hintText.emplace(font, "Hint: " + wrapped, 18); hintText->setPosition({questionText.getPosition().x, questionText.getPosition().y + questionText.getLocalBounds().size.y + 8.f}); hintAlpha = 0.f; } }
                        }
                    }
                    // Options
                    for (int i = 0; i < 4; ++i) {
                        if (optionDisabled[i]) continue;
                        if (optionRects[i].getGlobalBounds().contains(p)) {
                            // Validate answer
                            bool correct = (i == qs[current].correctIndex);
                            optionRects[i].setOutlineColor(theme.accent);
                            optionRects[i].setFillColor(correct ? sf::Color(30,120,60) : sf::Color(120,30,30));
                            window.clear(); ui::drawBackground(window, theme, window.getSize());
                            window.draw(topBar); window.draw(scoreText); window.draw(qnumText); if (logo) window.draw(*logo); else window.draw(titleFallback);
                            window.draw(questionBox); window.draw(categoryText); window.draw(questionText);
                            for (int j = 0; j < 4; ++j) { window.draw(optionRects[j]); window.draw(optionLabels[j]); }
                            window.draw(timerText); window.draw(sidePanel);
                            // draw ladder
                            float rowH = 26.f; float y0 = topBarH + 16.f; for (int li = ladder.size()-1, disp=ladder.size(); li>=0; --li, --disp) { sf::Text tnum(font, std::to_string(disp), 18); sf::Text tamt(font, ladder[li], 18); bool hilite = (disp-1 == current); tnum.setFillColor(hilite? theme.accent : sf::Color(220,210,240)); tamt.setFillColor(hilite? theme.accent : sf::Color(220,210,240)); float x = window.getSize().x - sidebarW + 14.f; float y = y0 + (ladder.size()-1-li)*rowH; tnum.setPosition({x, y}); auto lb = tamt.getLocalBounds(); tamt.setPosition({x + sidebarW - 28.f - lb.size.x, y}); window.draw(tnum); window.draw(tamt);} window.display();
                            if (correct) { player_.addScore(ladderVal[current]); scoreText.setString("Score: " + std::to_string(player_.getScore())); current++; if (current >= total) { endRound = true; window.close(); } else { audiencePct = {0,0,0,0}; optionDisabled = {false,false,false,false}; hintText.reset(); hintAlpha=0.f; resetQuestion(current); qnumText.setString("Q " + std::to_string(current+1) + "/" + std::to_string(total)); timerClk.restart(); } }
                            else { endRound = true; sf::sleep(sf::milliseconds(700)); window.close(); }
                            break;
                        }
                    }
                }
            }
        }

        // Animations and timer update
        float dt = frameClock.restart().asSeconds(); int elapsed = static_cast<int>(timerClk.getElapsedTime().asSeconds()); int remain = std::max(0, totalSecs - elapsed); int mm = remain/60, ss = remain%60; char buf[16]; std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss); timerText.setString(buf); layoutTop(); if (remain<=5) { float t = timerClk.getElapsedTime().asSeconds(); float a = 0.5f + 0.5f*std::sin(t*8.f); sf::Color c(255, static_cast<std::uint8_t>(120 + 100*a), static_cast<std::uint8_t>(120 + 100*a)); timerText.setFillColor(c);} else timerText.setFillColor(theme.accent);
        if (hintText) { hintAlpha = std::min(1.f, hintAlpha + dt*3.f); auto col = sf::Color(210,200,230, static_cast<std::uint8_t>(255*hintAlpha)); hintText->setFillColor(col);}        
        if (remain == 0) { endRound = true; window.close(); }

        // Hover visuals for lifelines
        for (size_t i = 0; i < lifelines.size(); ++i) {
            auto &b = lifelines[i];
            if (b.used) { b.rect.setFillColor(sf::Color(40,25,60)); b.label.setFillColor(sf::Color(170,170,190)); b.rect.setOutlineColor(theme.buttonOutline); b.rect.setOutlineThickness(2.f); }
            else if (hoverBtn && *hoverBtn == i) { b.rect.setOutlineColor(theme.accent); b.label.setFillColor(theme.accent); b.rect.setOutlineThickness(3.f); }
            else { b.rect.setOutlineColor(theme.buttonOutline); b.label.setFillColor(sf::Color(235,235,245)); b.rect.setFillColor(theme.buttonFill); b.rect.setOutlineThickness(2.f); }
        }
        // Draw frame
        window.clear(); ui::drawBackground(window, theme, window.getSize());
        window.draw(topBar); window.draw(scoreText); window.draw(qnumText); if (logo) window.draw(*logo); else window.draw(titleFallback);
        window.draw(questionBox); window.draw(categoryText); window.draw(questionText);
        for (int i = 0; i < 4; ++i) { if (hoverOption && *hoverOption == i) { optionRects[i].setOutlineThickness(3.f); optionRects[i].setOutlineColor(theme.accent);} else optionRects[i].setOutlineThickness(2.f); window.draw(optionRects[i]); window.draw(optionLabels[i]); if (audiencePct[i] > 0) { sf::Text pct(font, std::to_string(audiencePct[i]) + "%", 18); pct.setFillColor(sf::Color(220,210,240)); auto r = optionRects[i].getGlobalBounds(); pct.setPosition({r.position.x + r.size.x - 48.f, r.position.y + 14.f}); window.draw(pct);} }
        for (auto &b : lifelines) { window.draw(b.rect); window.draw(b.label); }
        if (hintText) window.draw(*hintText);
        window.draw(timerText);
        // Sidebar ladder drawing
        window.draw(sidePanel); float rowH = 26.f; float y0 = topBarH + 16.f; for (int li = ladder.size()-1, disp=ladder.size(); li>=0; --li, --disp) { sf::Text tnum(font, std::to_string(disp), 18); sf::Text tamt(font, ladder[li], 18); bool hilite = (disp-1 == current); tnum.setFillColor(hilite? theme.accent : sf::Color(220,210,240)); tamt.setFillColor(hilite? theme.accent : sf::Color(220,210,240)); float x = window.getSize().x - sidebarW + 14.f; float y = y0 + (ladder.size()-1-li)*rowH; tnum.setPosition({x, y}); auto lb = tamt.getLocalBounds(); tamt.setPosition({x + sidebarW - 28.f - lb.size.x, y}); window.draw(tnum); window.draw(tamt);}        
        // Tooltip
        if (hoverBtn) { tooltip.setString(""); /* minimal */ }
        window.display();

        if (endRound) break;
    }
#endif
}

} // namespace bow
