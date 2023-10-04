#pragma once
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <vector>

#include "./error/lexer_error.hpp"

enum class TokenType {
    none = 0,
    semicol,
    exit,
    int_lit,
    identifier,
    open_paren,
    close_paren,
    eq,
};

extern std::map<std::string, TokenType> tokenMappingsKeywords;
extern std::map<char, TokenType> tokenMappingsSymbols;

struct TokenMeta {
    size_t line_num;
    size_t line_pos;
};

struct Token {
    TokenMeta meta;
    TokenType type;
    std::optional<std::string> value;
};

// Lexical analysis unit
class Lexer {
   public:
    Lexer(std::string src) : m_src(src) {}
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