#include <SerialLineParser.hpp>

#include <iostream>

std::vector<SerialLineParser::Token> SerialLineParser::compileFormat(const std::string& format) {
    std::vector<Token> tokens;
    std::string literal;

    size_t i = 0;

    auto tryVariable = [&] {
        if (i + 3 > format.size() || format[i + 1] != '{') return false;

        if (!literal.empty()) {
            tokens.emplace_back(Token {
                .type = TokenType::Literal,
                .text = literal,
            });
            literal.clear();
        }

        // consume '${'
        i += 2;

        std::string name;
        while (i < format.size()) {
            char c = format[i];
            if (c == '}') break;
            name += c;
            i++;
        }

        tokens.emplace_back(Token {
            .type = TokenType::Double,
            .text = name,
        });

        // consume '}'
        i++;

        return true;
    };

    while (i < format.size()) {
        char c = format[i];

        if (c == '$') {
            if (tryVariable()) continue;
        }

        literal += c;
        i++;
    }

    return tokens;
}

void setPropertyOnSample(TimedRocketState& sample, const std::string& name, void* value) {
    if (name == "t") sample.time = *static_cast<double*>(value);

    else if (name == "qw") sample.state.orientation.w() = *static_cast<double*>(value);
    else if (name == "qx") sample.state.orientation.x() = *static_cast<double*>(value);
    else if (name == "qy") sample.state.orientation.y() = *static_cast<double*>(value);
    else if (name == "qz") sample.state.orientation.z() = *static_cast<double*>(value);

    else if (name == "px") sample.state.position.x() = *static_cast<double*>(value);
    else if (name == "py") sample.state.position.y() = *static_cast<double*>(value);
    else if (name == "pz") sample.state.position.z() = *static_cast<double*>(value);
}

SerialLineParser::SerialLineParser(const std::string& format) : _tokens(compileFormat(format)) { }

std::optional<TimedRocketState> SerialLineParser::Parse(const std::string& line) {
    TimedRocketState sample;
    sample.time = -1.0;

    size_t pos = 0;

    for (size_t i = 0; i < _tokens.size(); i++) {
        const Token& token = _tokens[i];

        if (token.type == TokenType::Literal) {
            if (line.compare(pos, token.text.size(), token.text) != 0) {
                std::cerr << "Line does not match the expected format: '" <<
                    line.substr(pos, token.text.size()) <<
                    "' != '" <<
                    token.text << "'" << std::endl;
                return std::nullopt;
            }
            pos += token.text.size();
            continue;
        }

        size_t end = line.size();

        if (i + 1 < _tokens.size() && _tokens[i + 1].type == TokenType::Literal) {
            const std::string& nextLiteral = _tokens[i + 1].text;

            end = line.find(nextLiteral, pos);

            if (end == std::string::npos) {
                std::cerr << "Line does not match the expected format: expected delimiter not found" << std::endl;
                return std::nullopt;
            }
        }

        std::string valueText = line.substr(pos, end - pos);

        switch (token.type) {
            case TokenType::Double: {
                char* endPtr = nullptr;

                double value = std::strtod(valueText.c_str(), &endPtr);

                if (endPtr != valueText.c_str() + valueText.size()) {
                    std::cerr << "Line does not match the expected format: invalid double '" <<
                        valueText << "'" << std::endl;
                    return std::nullopt;
                }

                setPropertyOnSample(sample, token.text, &value);
                break;
            }
            default:
                break;
        }

        pos = end;
    }

    if (pos != line.size()) {
        std::cerr << "Line does not match the expected format: unexpected trailing data '" <<
            line.substr(pos) << "'" << std::endl;
        return std::nullopt;
    }

    return sample;
}
