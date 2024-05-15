#include "../header/parser.hpp"

std::map<TokenType, BinOperation> binOperationMapping = {
    {TokenType::plus, BinOperation::add},       {TokenType::minus, BinOperation::subtract},
    {TokenType::star, BinOperation::multiply},  {TokenType::fslash, BinOperation::divide},
    {TokenType::percent, BinOperation::modulo},
};

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
        throw ParserException(msg, token.value().meta.line_num, token.value().meta.line_pos);
    }

    throw ParserException(msg);
}

bool Parser::test_peek(TokenType type, int offset) {
    return peek(offset).has_value() && peek(offset).value().type == type;
}

std::optional<int> Parser::binary_operator_precedence(const BinOperation& operation) {
    static_assert((int)BinOperation::operationCount - 1 == 5,
                  "Binary Operations enum changed without changing precendence mapping");
    switch (operation) {
        case BinOperation::add:
        case BinOperation::subtract:
            return 0;
        case BinOperation::multiply:
        case BinOperation::divide:
        case BinOperation::modulo:
            return 1;
        default:
            // TODO: could raise exception here
            return std::nullopt;
    }
}

std::shared_ptr<ASTAtomicExpression> Parser::try_parse_atomic() {
    if (test_peek(TokenType::int_lit)) {
        Token token = consume().value();
        return std::make_shared<ASTAtomicExpression>(
            ASTAtomicExpression{.value = ASTIntLiteral{.value = token.value.value()}});
    }
    if (test_peek(TokenType::identifier)) {
        Token token = consume().value();
        return std::make_shared<ASTAtomicExpression>(
            ASTAtomicExpression{.value = ASTIdentifier{.value = token.value.value()}});
    }

    return nullptr;
}

std::optional<ASTExpression> Parser::parse_expression(const int min_prec) {
    auto atomic = try_parse_atomic();
    if (atomic == nullptr) {
        return std::nullopt;
    }
    auto expr_lhs = ASTExpression{
        .data_type = DataType::NONE,
        .expression = atomic,
    };
    while (true) {
        auto binOperator = peek();
        if (!binOperator.has_value() || binOperationMapping.count(binOperator.value().type) == 0) {
            // no binary operator after lhs
            break;
        }
        auto binOperation = binOperationMapping.at(binOperator.value().type);
        std::optional<int> currentPrecedence = binary_operator_precedence(binOperation);
        if (!currentPrecedence.has_value() || currentPrecedence.value() < min_prec) {
            break;
        }
        consume();  // consume the token now that we know it's a binary operator
        auto rhs = parse_expression(currentPrecedence.value() + 1);
        if (!rhs.has_value()) {
            // TODO: raise exception
            std::cerr << "EXPECTED RHS EXPRESSION" << std::endl;
            exit(EXIT_FAILURE);
        }
        auto new_expr_lhs = std::make_shared<ASTExpression>(ASTExpression{
            .data_type = DataType::NONE,
            .expression = expr_lhs.expression,
        });
        auto new_expr_rhs = std::make_shared<ASTExpression>(rhs.value());
        expr_lhs.expression = std::make_shared<ASTBinExpression>(
            ASTBinExpression{.operation = binOperation, .lhs = new_expr_lhs, .rhs = new_expr_rhs});
    }

    return expr_lhs;
}

std::shared_ptr<ASTStatementExit> Parser::parse_statement_exit() {
    // exit([expression]);
    if (!test_peek(TokenType::exit)) return nullptr;

    Token statement_begin = consume().value();  // consume 'exit' token
    assert_consume(TokenType::open_paren, "Expected '(' after function 'exit'");
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Invalid expression parameter", statement_begin.meta.line_num,
                              statement_begin.meta.line_pos);
    }
    assert_consume(TokenType::close_paren, "Expected ')' after expression");
    assert_consume(TokenType::semicol, "Expected ';' after function call");

    return std::make_shared<ASTStatementExit>(ASTStatementExit{.status_code = std::move(expression.value())});
}

std::shared_ptr<ASTStatementVar> Parser::parse_statement_var_declare() {
    // [d_type] [identifier];
    // [d_type] [identifier] = [expression];
    if (!(test_peek(TokenType::identifier, 0) && test_peek(TokenType::identifier, 1))) return nullptr;
    Token d_type_token = consume().value();  // consume data type

    Token identifier = consume().value();  // consume identifier
    std::optional<ASTExpression> value = std::nullopt;

    if (test_peek(TokenType::eq)) {
        consume();  // consume 'eq' token
        auto expression = parse_expression();
        if (!expression.has_value()) {
            std::stringstream error_stream;
            error_stream << "Invalid initialize value for variable '" << identifier.value.value() << "'";
            throw ParserException(error_stream.str(), d_type_token.meta.line_num, d_type_token.meta.line_pos);
        }

        value = ASTExpression{
            .data_type = DataType::NONE,
            .expression = std::move(expression.value().expression),
        };
    }

    assert_consume(TokenType::semicol, "Expected ';' after variable delcaration");

    return std::make_shared<ASTStatementVar>(ASTStatementVar{
        .data_type_str = d_type_token.value.value(),
        .data_type = DataType::NONE,
        .name = identifier.value.value(),
        .value = std::move(value),
    });
}

std::shared_ptr<ASTStatement> Parser::parse_statement() {
    // check for exit statement
    if (auto exit_statement = parse_statement_exit(); exit_statement != nullptr) {
        return std::make_shared<ASTStatement>(ASTStatement{.statement = std::move(exit_statement)});
    }

    // check for variable declaration statement
    if (auto var_declare_statement = parse_statement_var_declare(); var_declare_statement != nullptr) {
        return std::make_shared<ASTStatement>(ASTStatement{.statement = std::move(var_declare_statement)});
    }

    // fallback- no matching statement found
    return nullptr;
}

ASTProgram Parser::parse_program() {
    ASTProgram result;

    while (peek().has_value()) {
        auto statement = parse_statement();
        if (statement == nullptr) {
            Token token = peek().value();
            throw ParserException("Invalid statement", token.meta.line_num, token.meta.line_pos);
        }

        result.statements.push_back(std::move(statement));
    }

    return result;
}