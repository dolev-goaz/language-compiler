#include "../header/token.hpp"

std::map<std::string, TokenType> tokenMappingsKeywords = {
    {"exit", TokenType::exit},
};
std::map<char, TokenType> tokenMappingsSymbols = {
    {';', TokenType::semicol},
    {'(', TokenType::open_paren},
    {')', TokenType::close_paren},
};

char TokenParser::consume() {
    char current = m_src.at(m_char_ind++);
    if (current == '\n') {
        m_line += 1;
        m_col = 1;
    } else {
        m_col += 1;
    }

    return current;
}

std::optional<char> TokenParser::peek(size_t offset) {
    if (m_char_ind + offset >= m_src.length()) {
        return std::nullopt;
    }
    const char current = m_src.at(m_char_ind + offset);
    if (current == 0 || current == EOF) {
        return std::nullopt;
    }
    return current;
}

bool TokenParser::try_consume(char character) {
    if (!this->peek().has_value() || this->peek().value() != character) {
        return false;
    }
    TokenMeta meta = {.line_num = m_line, .line_pos = m_col};
    Token token = {
        .meta = meta,
        .type = tokenMappingsSymbols[character],
        .value = std::nullopt,
    };

    this->consume();
    return true;
}

void TokenParser::consume_word() {
    std::string buffer;
    size_t line = m_line, col = m_col;
    // check first character is an alphabet char
    if (!this->peek().has_value() || !std::isalpha(this->peek().value())) {
        return;
    }
    // rest of the characters can be alphabet or numeric
    while (this->peek().has_value() && std::isalnum(this->peek().value())) {
        buffer.push_back(this->consume());
    }

    // keyword doesn't exist
    TokenType type = tokenMappingsKeywords.count(buffer) == 0
                         ? TokenType::idenfitier
                         : tokenMappingsKeywords[buffer];

    TokenMeta meta = {.line_num = line, .line_pos = col};
    Token token = {
        .meta = meta,
        .type = tokenMappingsKeywords[buffer],
        .value = std::nullopt,
    };
    this->tokens.push_back(token);
}

void TokenParser::consume_number() {
    std::string buffer;
    size_t line = m_line, col = m_col;
    while (this->peek().has_value() && std::isdigit(this->peek().value())) {
        buffer.push_back(this->consume());
    }

    TokenMeta meta = {.line_num = line, .line_pos = col};
    Token token = {
        .meta = meta,
        .type = TokenType::int_lit,
        .value = buffer,
    };
    this->tokens.push_back(token);
}

std::vector<Token> TokenParser::tokenize() {
    m_line = 1, m_col = 1;  // for token metadata
    m_char_ind = 0;

    while (peek().has_value()) {
        char current = peek().value();
        if (std::isspace(current)) {
            this->consume();
            continue;
        }
        if (std::isalpha(current)) {
            this->consume_word();
            continue;
        }
        if (std::isdigit(current)) {
            this->consume_number();
            continue;
        }

        // consume special characters

        bool found = false;
        for (auto &pair : tokenMappingsSymbols) {
            // first contains the key
            if (this->try_consume(pair.first)) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Invalid character: '" << current << "'" << std::endl
                      << "Character code: " << (int)current << std::endl;
            printf("Line: %lu, Column: %lu\n", m_line, m_col);
            exit(EXIT_FAILURE);
        }
    }

    return this->tokens;
}