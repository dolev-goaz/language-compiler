#pragma once
#include <iostream>
#include <optional>
#include <vector>

enum class TokenType {
    none = 0,
    semicol,
    exit,
    int_lit,
    open_paren,
    close_paren,
};

struct TokenMeta {
    int line_num;
    int line_pos;
};

struct Token {
    TokenMeta meta;
    TokenType type;
    std::string value;
};

class TokenParser {
   public:
    TokenParser(std::string src) : m_src(src) {}
    std::vector<Token> tokenize();

   private:
    const std::string m_src;
    size_t m_line;
    size_t m_col;
    size_t m_char_ind;
    std::vector<Token> tokens;

    std::optional<char> peek(size_t offset = 0);
    char consume();

    void consume_word();
    void consume_number();
    bool try_consume(char character);
};