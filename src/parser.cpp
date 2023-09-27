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
std::optional<Token> Parser::try_consume(TokenType type) { return this->test_peek(type) ? consume() : std::nullopt; }

Token Parser::assert_consume(TokenType type, const std::string& msg) {
    std::optional<Token> consumed = try_consume(type);
    if (consumed.has_value()) {
        return consumed.value();
    }
    if (auto token = peek(); token.has_value()) {
        throw ParserError(msg, token.value().meta.line_num, token.value().meta.line_pos);
    }

    throw ParserError(msg);
}

bool Parser::test_peek(TokenType type) { return peek().has_value() && peek().value().type == type; }

std::optional<ASTExpression> Parser::parse_expression() {
    if (test_peek(TokenType::int_lit)) {
        Token token = consume().value();
        ASTIntLiteral literal{.value = token.value.value()};

        return ASTExpression{.expression = literal};
    }
    if (test_peek(TokenType::identifier)) {
        Token token = consume().value();
        ASTIdentifier identifier{.value = token.value.value()};
        return ASTExpression{.expression = identifier};
    }

    return std::nullopt;
}

std::optional<ASTStatementExit> Parser::parse_statement_exit() {
    // exit([expression]);
    if (!test_peek(TokenType::exit)) return std::nullopt;

    Token statement_begin = consume().value();  // consume 'exit' token
    assert_consume(TokenType::open_paren, "Expected '(' after function 'exit'");
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserError("Invalid expression parameter", statement_begin.meta.line_num, statement_begin.meta.line_pos);
    }
    assert_consume(TokenType::close_paren, "Expected ')' after expression");
    assert_consume(TokenType::semicol, "Expected ';' after function call");

    return ASTStatementExit{.status_code = expression.value()};
}

std::optional<ASTStatementVar> Parser::parse_statement_var_declare() {
    // var [identifier];
    // var [identifier] = [expression];
    if (!test_peek(TokenType::var)) return std::nullopt;
    Token statement_begin = consume().value();  // consume 'var' token

    Token identifier = assert_consume(TokenType::identifier, "Expected Identifier for variable name");
    std::optional<ASTExpression> value = std::nullopt;

    if (test_peek(TokenType::eq)) {
        consume();  // consume 'eq' token
        auto expression = parse_expression();
        if (!expression.has_value()) {
            std::stringstream error_stream;
            error_stream << "Invalid initialize value for variable '" << identifier.value.value() << "'";
            throw ParserError(error_stream.str(), statement_begin.meta.line_num, statement_begin.meta.line_pos);
        }

        value = ASTExpression{
            .expression = expression.value().expression,
        };
    }

    assert_consume(TokenType::semicol, "Expected ';' after variable delcaration");

    return ASTStatementVar{
        .name = identifier.value.value(),
        .value = value,
    };
}

std::optional<ASTStatement> Parser::parse_statement() {
    // check for exit statement
    if (auto exit_statement = parse_statement_exit(); exit_statement.has_value()) {
        return ASTStatement{.statement = exit_statement.value()};
    }

    // check for variable declaration statement
    if (auto var_declare_statement = parse_statement_var_declare(); var_declare_statement.has_value()) {
        return ASTStatement{.statement = var_declare_statement.value()};
    }

    // fallback- no matching statement found
    return std::nullopt;
}

ASTProgram Parser::parse_program() {
    ASTProgram result;

    while (peek().has_value()) {
        auto statement = parse_statement();
        if (!statement.has_value()) {
            Token token = peek().value();
            throw ParserError("Invalid statement", token.meta.line_num, token.meta.line_pos);
        }

        result.statements.push_back(statement.value());
    }

    return result;
}