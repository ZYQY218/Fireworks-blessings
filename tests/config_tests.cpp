#include "../app_config.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        std::istringstream empty("");
        const AppConfig defaults = readConfig(empty);
        require(defaults.name == "FRIEND" && defaults.autoFireworks && defaults.volume == 0.5f,
            "Missing configuration must have usable defaults");

        std::istringstream customized(
            "\xEF\xBB\xBF# UTF-8 BOM and CRLF\r\n"
            " title = Happy Anniversary \r\nsubtitle=You = happiness\r\n"
            "name=朋友\r\nvolume=0.75\r\nauto_fireworks=FALSE\r\n");
        const AppConfig config = readConfig(customized);
        require(config.title == "Happy Anniversary" && config.subtitle == "You = happiness",
            "Trim keys/values while preserving equals inside text");
        require(config.name == "朋友" && config.volume == 0.75f && !config.autoFireworks,
            "Read UTF-8 text, volume and boolean");

        std::istringstream invalid(
            "bad line\nunknown=value\nvolume=oops\nvolume=nan\nvolume=inf\n"
            "volume=0.3junk\nauto_fireworks=maybe\n");
        const AppConfig fallback = readConfig(invalid);
        require(fallback.volume == 0.5f && fallback.autoFireworks,
            "Malformed fields must keep their defaults");

        std::istringstream high("volume=8\n");
        std::istringstream low("volume=-2\n");
        require(readConfig(high).volume == 1.0f && readConfig(low).volume == 0.0f,
            "Clamp volume to the valid range");
        std::istringstream hidden("title=\nsubtitle=\nname=\n");
        const AppConfig blank = readConfig(hidden);
        require(blank.title.empty() && blank.subtitle.empty() && blank.name.empty(),
            "Allow hiding individual text lines");
        std::istringstream longName("name=" + std::string(513, 'x'));
        require(readConfig(longName).name == defaults.name, "Reject oversized text fields");
        std::cout << "CONFIG TESTS OK\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
