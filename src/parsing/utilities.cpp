#include "parser.hpp"

void Parser::finalize_consumption() { parse_stack.finalize_consumption(); }
void Parser::undo_consumption() {
    size_t consumed_in_layer = parse_stack.get_consume_count();
    undo_consumption(consumed_in_layer);
}
void Parser::undo_consumption(size_t count) {
    parse_stack.undo_consumption(count);
    m_token_index -= count;
}

std::optional<Token> Parser::consume_raw() {
    if (m_token_index >= m_tokens.size()) {
        return std::nullopt;
    }

    return m_tokens.at(m_token_index++);
}

std::optional<Token> Parser::consume() {
    auto token = consume_raw();
    if (token.has_value()) {
        parse_stack.consume(1);
    }
    return token;
}

std::optional<Token> Parser::peek(int offset) {
    if (m_token_index + offset >= m_tokens.size()) {
        return std::nullopt;
    }
    return m_tokens.at(m_token_index + offset);
}
std::optional<Token> Parser::try_consume(TokenType type) { return this->test_peek(type) ? consume() : std::nullopt; }

Token Parser::assert_consume(TokenType type, const std::string& msg) {
    std::optional<Token> consumed = try_consume(type);
    if (consumed.has_value()) {
        return consumed.value();
    }
    if (auto token = peek(); token.has_value()) {
        throw ParserException(msg, token.value().meta);
    }

    // no next token to peek- EOF
    throw ParserException(msg);
}

bool Parser::test_peek(TokenType type, int offset) {
    return peek(offset).has_value() && peek(offset).value().type == type;
}

std::vector<Token> Parser::consume_data_type_tokens() {
    std::vector<Token> out;

    out.push_back(assert_consume(TokenType::identifier, "Expected identifier for data type"));

    while (test_peek(TokenType::star) || test_peek(TokenType::open_square)) {
        if (test_peek(TokenType::open_square)) {
            // array declaration
            out.push_back(consume().value());
            if (test_peek(TokenType::int_lit)) {
                out.push_back(consume().value());  // consume array size- EXPLICIT array size
            }
            out.push_back(assert_consume(TokenType::close_square, "Expected closing bracket ']'"));
        } else {
            // star- pointer type
            out.push_back(consume().value());
        }
    }

    return out;
}

std::vector<Token> Parser::consume_array_modifier_tokens() {
    // NOTE: could be used for array indexing
    std::vector<Token> out;

    while (test_peek(TokenType::open_square)) {
        out.push_back(consume().value());
        out.push_back(assert_consume(TokenType::int_lit, "Expected array size"));
        out.push_back(assert_consume(TokenType::close_square, "Expected closing bracket ']'"));
    }

    return out;
}