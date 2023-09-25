#include "../header/lexer.hpp"

bool number_verify(std::string num_str) {
    // Hexadecimal 0[xX][0-9a-fA-F]+
    // Binary 0[bB][01]+
    // Decimal \d+
    std::regex pattern("^(0[xX][0-9A-Fa-f]+|0[bB][01]+|\\d+)$");

    // Use std::regex_match to check if the string matches the pattern
    return std::regex_match(num_str, pattern);
}

std::map<std::string, TokenType> tokenMappingsKeywords = {
    {"exit", TokenType::exit},
};
std::map<char, TokenType> tokenMappingsSymbols = {
    {';', TokenType::semicol},
    {'(', TokenType::open_paren},
    {')', TokenType::close_paren},
};

char Lexer::consume() {
    char current = m_src.at(m_char_ind++);
    if (current == '\n') {
        m_line += 1;
        m_col = 1;
    } else {
        m_col += 1;
    }

    return current;
}

std::optional<char> Lexer::peek(size_t offset) {
    if (m_char_ind + offset >= m_src.length()) {
        return std::nullopt;
    }
    const char current = m_src.at(m_char_ind + offset);
    if (current == 0 || current == EOF) {
        return std::nullopt;
    }
    return current;
}

bool Lexer::try_consume(char character) {
    if (!this->peek().has_value() || this->peek().value() != character) {
        return false;
    }
    TokenMeta meta = {.line_num = m_line, .line_pos = m_col};
    Token token = {
        .meta = meta,
        .type = tokenMappingsSymbols[character],
        .value = std::nullopt,
    };
    this->tokens.push_back(token);

    this->consume();
    return true;
}

void Lexer::consume_word() {
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
                         ? TokenType::identifier
                         : tokenMappingsKeywords[buffer];

    TokenMeta meta = {.line_num = line, .line_pos = col};
    Token token = {
        .meta = meta,
        .type = type,
        .value = std::nullopt,
    };
    this->tokens.push_back(token);
}

void Lexer::consume_number() {
    std::string buffer;
    size_t line = m_line, col = m_col;
    // check first character is a number
    if (!this->peek().has_value() || !std::isdigit(this->peek().value())) {
        return;
    }
    // rest of the characters can be alphabet or numeric(200, 0x1f, 0b011)
    while (this->peek().has_value() && std::isalnum(this->peek().value())) {
        buffer.push_back(this->consume());
    }

    if (!number_verify(buffer)) {
        printf("Invalid number literal: '%s'\n", buffer.c_str());
        printf("Line: %lu, Column: %lu\n", line, col);
        exit(EXIT_FAILURE);
    }

    TokenMeta meta = {.line_num = line, .line_pos = col};
    Token token = {
        .meta = meta,
        .type = TokenType::int_lit,
        .value = buffer,
    };
    this->tokens.push_back(token);
}

std::vector<Token> Lexer::tokenize() {
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