#include "Utils.h"
#include <algorithm>
#include <array>
#include <random>
#include <unordered_map>
#include <cctype>

namespace bow {

// Convert code point to UTF-8 and append
static void appendUtf8(uint32_t cp, std::string& out) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// HTML entity decoder (covers common named entities and numeric codes like &#225; &#xE1;)
std::string htmlDecode(const std::string& s) {
    static const std::unordered_map<std::string, std::string> named = {
        {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&#039;", "'"},
        {"&nbsp;", " "}, {"&ndash;", "-"}, {"&mdash;", "-"}, {"&hellip;", "..."},
        {"&ldquo;", "\""}, {"&rdquo;", "\""}, {"&lsquo;", "'"}, {"&rsquo;", "'"},
        {"&aacute;", "á"}, {"&eacute;", "é"}, {"&iacute;", "í"}, {"&oacute;", "ó"}, {"&uacute;", "ú"},
        {"&Aacute;", "Á"}, {"&Eacute;", "É"}, {"&Iacute;", "Í"}, {"&Oacute;", "Ó"}, {"&Uacute;", "Ú"},
        {"&ntilde;", "ñ"}, {"&Ntilde;", "Ñ"}, {"&uuml;", "ü"}, {"&Uuml;", "Ü"},
        {"&shy;", ""}
    };

    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '&') {
            size_t j = s.find(';', i + 1);
            if (j != std::string::npos) {
                std::string token = s.substr(i, j - i + 1);
                // Named entity
                auto it = named.find(token);
                if (it != named.end()) {
                    out += it->second;
                    i = j;
                    continue;
                }
                // Numeric decimal &#NNN; or hex &#xNN; / &#XNN;
                if (token.size() > 3 && token[1] == '#') {
                    uint32_t cp = 0;
                    if (token[2] == 'x' || token[2] == 'X') {
                        for (size_t k = 3; k + i < j; ++k) {
                            char c = s[i + k];
                            cp <<= 4;
                            if (c >= '0' && c <= '9') cp |= (c - '0');
                            else if (c >= 'a' && c <= 'f') cp |= (10 + c - 'a');
                            else if (c >= 'A' && c <= 'F') cp |= (10 + c - 'A');
                        }
                    } else {
                        for (size_t k = 2; k + i < j; ++k) {
                            char c = s[i + k];
                            if (!std::isdigit(static_cast<unsigned char>(c))) { cp = 0; break; }
                            cp = cp * 10 + (c - '0');
                        }
                    }
                    if (cp != 0) {
                        appendUtf8(cp, out);
                        i = j;
                        continue;
                    }
                }
            }
        }
        out.push_back(s[i]);
    }
    return out;
}

void shuffleOptions(std::array<std::string,4>& options, int& correctIndex) {
    std::array<int,4> idx{0,1,2,3};
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(idx.begin(), idx.end(), rng);

    std::array<std::string,4> copy = options;
    for (int i = 0; i < 4; ++i) {
        options[i] = copy[idx[i]];
    }
    for (int i = 0; i < 4; ++i) {
        if (idx[i] == correctIndex) { correctIndex = i; break; }
    }
}

} // namespace bow
