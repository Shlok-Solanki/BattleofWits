#pragma once

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>

namespace bow::ui {

struct Theme {
    sf::Color bgTop;
    sf::Color bgBottom;
    sf::Color panelFill;
    sf::Color panelOutline;
    sf::Color buttonFill;
    sf::Color buttonOutline;
    sf::Color accent;
};

Theme defaultTheme();

// Attempts to load a readable system font (Arial/Segoe/Calibri). Returns true on success.
bool loadSystemFont(sf::Font& font);

// Draw a simple vertical gradient background filling the target size.
void drawBackground(sf::RenderTarget& target, const Theme& theme, sf::Vector2u size);

void stylePanel(sf::RectangleShape& rect, const Theme& theme);
void styleButton(sf::RectangleShape& rect, const Theme& theme);

} // namespace bow::ui

#endif // HAS_SFML
