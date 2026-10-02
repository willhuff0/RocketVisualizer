#pragma once

#include "IRocketTimeline.hpp"

#include <string>
#include <vector>
#include <optional>

class SerialLineParser {
    enum class TokenType {
        Literal,
        Double,
    };

    struct Token {
        TokenType type;
        std::string text;
    };

    static std::vector<Token> compileFormat(const std::string& format);

public:
    explicit SerialLineParser(const std::string& format);

    std::optional<TimedRocketState> Parse(const std::string& line);

private:
    std::vector<Token> _tokens;
};