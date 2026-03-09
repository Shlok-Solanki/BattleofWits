#include "UiTheme.h"

#ifdef HAS_SFML

namespace bow::ui {

Theme defaultTheme() {
    return Theme{
        /*bgTop*/        sf::Color(60, 30, 90),
        /*bgBottom*/     sf::Color(20, 10, 30),
        /*panelFill*/    sf::Color(40, 20, 70),
        /*panelOutline*/ sf::Color(220, 190, 80),
        /*buttonFill*/   sf::Color(50, 30, 85),
        /*buttonOutline*/sf::Color(220, 190, 80),
        /*accent*/       sf::Color(255, 215, 0)
    };
}

bool loadSystemFont(sf::Font& font) {
    const char* candidates[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf"
    };
    for (auto path : candidates) {
        if (font.openFromFile(path)) return true;
    }
    return false;
}

void drawBackground(sf::RenderTarget& target, const Theme& theme, sf::Vector2u size) {
    sf::Vertex quad[4];
    quad[0].position = {0.f, 0.f};
    quad[1].position = {static_cast<float>(size.x), 0.f};
    quad[2].position = {0.f, static_cast<float>(size.y)};
    quad[3].position = {static_cast<float>(size.x), static_cast<float>(size.y)};
    quad[0].color = quad[1].color = theme.bgTop;
    quad[2].color = quad[3].color = theme.bgBottom;

    sf::RenderStates states;
    target.draw(quad, 4, sf::PrimitiveType::TriangleStrip, states);
}

void stylePanel(sf::RectangleShape& rect, const Theme& theme) {
    rect.setFillColor(theme.panelFill);
    rect.setOutlineThickness(3.f);
    rect.setOutlineColor(theme.panelOutline);
}

void styleButton(sf::RectangleShape& rect, const Theme& theme) {
    rect.setFillColor(theme.buttonFill);
    rect.setOutlineThickness(2.f);
    rect.setOutlineColor(theme.buttonOutline);
}

} // namespace bow::ui

#endif // HAS_SFML
