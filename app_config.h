#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <istream>
#include <string>

struct AppConfig {
    std::string title = "Happy Birthday";
    std::string subtitle = "Breeze in heart, ease in life";
    std::string name = "FRIEND";
    float volume = 0.5f;
    bool autoFireworks = true;
};

inline std::string trimConfigValue(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

// Flat UTF-8 key=value file. Invalid fields keep their previous/default value.
inline AppConfig readConfig(std::istream& input, AppConfig config = {}) {
    std::string line;
    bool firstLine = true;
    while (std::getline(input, line)) {
        if (firstLine && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        firstLine = false;
        line = trimConfigValue(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = trimConfigValue(line.substr(0, separator));
        const std::string value = trimConfigValue(line.substr(separator + 1));
        if (key == "title" || key == "subtitle" || key == "name") {
            // Bound font-atlas size and layout work for unexpectedly large files.
            if (value.size() > 512) continue;
            if (key == "title") config.title = value;
            else if (key == "subtitle") config.subtitle = value;
            else config.name = value;
        }
        else if (key == "volume") {
            try {
                std::size_t consumed = 0;
                const float volume = std::stof(value, &consumed);
                if (consumed == value.size() && std::isfinite(volume)) {
                    config.volume = std::clamp(volume, 0.0f, 1.0f);
                }
            }
            catch (...) { /* Keep the default for malformed numbers. */ }
        }
        else if (key == "auto_fireworks") {
            std::string boolean = value;
            std::transform(boolean.begin(), boolean.end(), boolean.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (boolean == "true") config.autoFireworks = true;
            else if (boolean == "false") config.autoFireworks = false;
        }
    }
    return config;
}
