#include "../header/parser.hpp"

std::optional<Token> Parser::consume() {
    if (m_token_index >= m_tokens.size()) {
        return std::nullopt;
    }

    return m_tokens.at(m_token_index++);
}
std::optional<Token> Parser::peek(int offset) {
    if (m_token_index + offset >= m_tokens.size()) {
        return std::nullopt;
    }
    return m_tokens.at(m_token_index + offset);
}
std::optional<Token> Parser::try_consume(TokenType type) {
    return this->test_peek(type) ? consume() : std::nullopt;
}

Token Parser::assert_consume(TokenType type, const std::string& msg) {
    std::optional<Token> consumed = try_consume(type);
    if (consumed.has_value()) {
        return consumed.value();
    }
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

bool Parser::test_peek(TokenType type) {
    return peek().has_value() && peek().value().type == type;
}

std::optional<ASTExpression> Parser::parse_expression() {
    if (test_peek(TokenType::int_lit)) {
        ASTIntLiteral literal;
        literal.value = consume().value();
        return literal;
    }
    if (test_peek(TokenType::idenfitier)) {
        ASTIdentifier identifier;
        identifier.value = consume().value();
        return identifier;
    }

    return std::nullopt;
}

std::optional<ASTStatement> Parser::parse_statement() {
    if (test_peek(TokenType::exit)) {
        consume();  // consume 'exit' token
        assert_consume(TokenType::open_paren, "Expected '('");
        auto expression = parse_expression();
        if (!expression.has_value()) {
            std::cerr << "Invalid expression" << std::endl;
            exit(EXIT_FAILURE);
        }
        assert_consume(TokenType::close_paren, "Expected ')'");
        assert_consume(TokenType::semicol, "Expected ';'");

        ASTStatementExit statement;
        statement.code = expression.value();

        return statement;
    }

    return std::nullopt;
}

ASTProgram Parser::parse_program() {
    ASTProgram result;

    while (peek().has_value()) {
        auto statement = parse_statement();
        if (!statement.has_value()) {
            std::cerr << "Invalid statement" << std::endl;
            exit(EXIT_FAILURE);
        }

        result.statements.push_back(statement.value());
    }

    return result;
}