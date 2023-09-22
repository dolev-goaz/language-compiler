#pragma once
#include <iostream>
#include <map>
#include <optional>
#include <vector>

enum class TokenType {
    none = 0,
    semicol,
    exit,
    int_lit,
    idenfitier,
    open_paren,
    close_paren,
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

class Tokenizer {
   public:
    Tokenizer(std::string src) : m_src(src) {}
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