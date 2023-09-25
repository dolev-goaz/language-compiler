#include "../header/lexer.hpp"

bool number_verify(const std::string& num_str) {
    // Hexadecimal 0[xX][0-9a-fA-F]+
    // Binary 0[bB][01]+
    // Decimal \d+
    std::regex pattern("^(0[xX][0-9A-Fa-f]+|0[bB][01]+|\\d+)$");

    return std::regex_match(num_str, pattern);
}

std::map<std::string, TokenType> tokenMappingsKeywords = {
    {"exit", TokenType::exit},
    {"var", TokenType::var},
};
std::map<char, TokenType> tokenMappingsSymbols = {
    {';', TokenType::semicol},
    {'(', TokenType::open_paren},
    {')', TokenType::close_paren},
    {'=', TokenType::eq},
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

    // if we reached the end of the string or the end of the file,
    // there is nothing to peek.
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

    TokenType type;
    std::optional<std::string> value;

    bool keyword_exists = tokenMappingsKeywords.count(buffer) > 0;

    if (keyword_exists) {
        type = tokenMappingsKeywords[buffer];
        value = std::nullopt;
    } else {
        type = TokenType::identifier;
        value = buffer;
    }

    TokenMeta meta = {.line_num = line, .line_pos = col};
    Token token = {
        .meta = meta,
        .type = type,
        .value = value,
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
        std::stringstream error_stream;
        error_stream << "Invalid Number Literal " << buffer;
        throw LexerException(error_stream.str(), line, col);
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
    // initialize m_line to the first line, m_col to the first column
    m_line = 1, m_col = 1;
    // initialize m_char_ind to 0(no characters were read yet)
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
        for (auto& pair : tokenMappingsSymbols) {
            // first contains the key
            if (this->try_consume(pair.first)) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::stringstream err_message;
            err_message << "Invalid Character '" << current << "'" << std::endl;
            err_message << "Character Code: " << (int)current;
            throw LexerException(err_message.str(), m_line, m_col);
        }
    }

    return this->tokens;
}