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
        throw ParserException(msg, token.value().meta);
    }

    // no next token to peek- EOF
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
            assert(false && "Binary operation not implemented- " + (int)operation);
    }
}

std::shared_ptr<ASTAtomicExpression> Parser::try_parse_atomic() {
    auto token = peek();
    if (!token.has_value()) return nullptr;
    auto meta = token.value().meta;
    if (test_peek(TokenType::int_lit)) {
        Token token = consume().value();
        return std::make_shared<ASTAtomicExpression>(ASTAtomicExpression{
            .start_token_meta = meta, .value = ASTIntLiteral{.start_token_meta = meta, .value = token.value.value()}});
    }
    if (test_peek(TokenType::identifier)) {
        Token token = consume().value();
        return std::make_shared<ASTAtomicExpression>(ASTAtomicExpression{
            .start_token_meta = meta, .value = ASTIdentifier{.start_token_meta = meta, .value = token.value.value()}});
    }
    if (test_peek(TokenType::open_paren)) {
        consume();  // consume open paren '('
        auto expression = parse_expression();
        if (!expression.has_value()) {
            auto nextToken = peek();
            if (nextToken.has_value()) {
                throw ParserException("Expected expression after opening parenthesis '('", nextToken.value().meta);
            } else {
                throw ParserException("Expected expression after opening parenthesis '('");
            }
        }
        assert_consume(TokenType::close_paren, "Expected closing parenthesis ')' after open paranthesis.");
        return std::make_shared<ASTAtomicExpression>(ASTAtomicExpression{
            .start_token_meta = meta,
            .value =
                ASTParenthesisExpression{
                    .start_token_meta = meta,
                    .expression = std::make_shared<ASTExpression>(expression.value()),
                },
        });
    }

    return nullptr;
}

std::optional<ASTExpression> Parser::parse_expression(const int min_prec) {
    auto atomic = try_parse_atomic();
    if (atomic == nullptr) {
        return std::nullopt;
    }
    auto expr_lhs = ASTExpression{
        .start_token_meta = atomic.get()->start_token_meta,
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
            auto nextToken = peek();
            if (nextToken.has_value()) {
                throw ParserException("Expected RHS expression", nextToken.value().meta);
            } else {
                throw ParserException("Expected RHS expression");
            }
        }
        auto new_expr_lhs = std::make_shared<ASTExpression>(ASTExpression{
            .start_token_meta = expr_lhs.start_token_meta,
            .data_type = DataType::NONE,
            .expression = expr_lhs.expression,
        });
        auto new_expr_rhs = std::make_shared<ASTExpression>(rhs.value());
        expr_lhs.expression = std::make_shared<ASTBinExpression>(
            ASTBinExpression{.start_token_meta = new_expr_lhs.get()->start_token_meta,
                             .operation = binOperation,
                             .lhs = new_expr_lhs,
                             .rhs = new_expr_rhs});
    }

    return expr_lhs;
}

std::shared_ptr<ASTStatementIf> Parser::parse_statement_if() {
    if (!test_peek(TokenType::_if)) return nullptr;
    const auto& statement_begin_meta = consume().value().meta;

    // NOTE: this is the same logic as in exit. could maybe refactor this?
    assert_consume(TokenType::open_paren, "Expected '(' after 'if' statement");
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Invalid expression parameter", statement_begin_meta);
    }
    assert_consume(TokenType::close_paren, "Expected ')' after expression");

    auto success_statement = parse_statement();
    if (success_statement == nullptr) {
        throw ParserException("Expected statement after 'if' condition", statement_begin_meta);
    }

    auto if_statement = std::make_shared<ASTStatementIf>(ASTStatementIf{
        .start_token_meta = statement_begin_meta,
        .expression = expression.value(),
        .success_statement = success_statement,
        .fail_statement = nullptr,
    });

    if (test_peek(TokenType::_else)) {
        // consume else token
        auto else_begin_meta = consume().value().meta;
        auto fail_statement = parse_statement();
        if (success_statement == nullptr) {
            throw ParserException("Expected statement after 'else' keyword", else_begin_meta);
        }
        if_statement.get()->fail_statement = fail_statement;
    }

    return if_statement;
}

std::shared_ptr<ASTStatementScope> Parser::parse_statement_scope() {
    if (!test_peek(TokenType::open_curly)) return nullptr;
    const TokenMeta statement_begin_meta = consume().value().meta;
    std::vector<std::shared_ptr<ASTStatement>> statements;
    while (peek().has_value() && !test_peek(TokenType::close_curly)) {
        statements.push_back(parse_statement());
    }
    assert_consume(TokenType::close_curly, "Expected '}'");
    return std::make_shared<ASTStatementScope>(
        ASTStatementScope{.start_token_meta = statement_begin_meta, .statements = statements});
}

std::shared_ptr<ASTStatementWhile> Parser::parse_statement_while() {
    // NOTE: pretty much identical to parse_statement_if
    if (!test_peek(TokenType::_while)) return nullptr;
    const auto& statement_begin_meta = consume().value().meta;

    // NOTE: this is the same logic as in exit. could maybe refactor this?
    assert_consume(TokenType::open_paren, "Expected '(' after 'while' statement");
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Invalid expression parameter", statement_begin_meta);
    }
    assert_consume(TokenType::close_paren, "Expected ')' after expression");

    auto success_statement = parse_statement();
    if (success_statement == nullptr) {
        throw ParserException("Expected statement after 'while' condition", statement_begin_meta);
    }

    return std::make_shared<ASTStatementWhile>(ASTStatementWhile{
        .start_token_meta = statement_begin_meta,
        .expression = expression.value(),
        .success_statement = success_statement,
    });
}

std::shared_ptr<ASTStatementExit> Parser::parse_statement_exit() {
    // exit([expression]);
    if (!test_peek(TokenType::exit)) return nullptr;

    Token statement_begin = consume().value();  // consume 'exit' token
    assert_consume(TokenType::open_paren, "Expected '(' after function 'exit'");
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Invalid expression parameter", statement_begin.meta);
    }
    assert_consume(TokenType::close_paren, "Expected ')' after expression");
    assert_consume(TokenType::semicol, "Expected ';' after function call");

    return std::make_shared<ASTStatementExit>(
        ASTStatementExit{.start_token_meta = statement_begin.meta, .status_code = std::move(expression.value())});
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
            throw ParserException(error_stream.str(), d_type_token.meta);
        }

        value = ASTExpression{
            .start_token_meta = expression.value().start_token_meta,
            .data_type = DataType::NONE,
            .expression = std::move(expression.value().expression),
        };
    }

    assert_consume(TokenType::semicol, "Expected ';' after variable delcaration");

    return std::make_shared<ASTStatementVar>(ASTStatementVar{
        .start_token_meta = d_type_token.meta,
        .data_type_str = d_type_token.value.value(),
        .data_type = DataType::NONE,
        .name = identifier.value.value(),
        .value = std::move(value),
    });
}

std::shared_ptr<ASTStatementAssign> Parser::parse_statement_var_assign() {
    // only if its in the form [identifier] = ....
    if (!test_peek(TokenType::identifier, 0) || !test_peek(TokenType::eq, 1)) {
        return nullptr;
    }

    auto identifier = consume().value();
    auto& statement_meta = identifier.meta;
    consume();  // eq operator
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Expected RHS expression", statement_meta);
    }
    assert_consume(TokenType::semicol, "Expected ';' after variable assignment");

    return std::make_shared<ASTStatementAssign>(ASTStatementAssign{
        .start_token_meta = statement_meta,
        .name = identifier.value.value(),
        .value = expression.value(),
    });
}

std::shared_ptr<ASTStatementFunction> Parser::parse_statement_function() {
    if (!test_peek(TokenType::_function)) {
        return nullptr;
    }
    auto statement_begin_meta = consume().value().meta;
    auto func_name = assert_consume(TokenType::identifier, "Expected function name");
    assert_consume(TokenType::open_paren, "Expected '(' after function name");

    auto parameters = parse_function_params();

    assert_consume(TokenType::close_paren, "Expected ')' after function params");
    auto statement = parse_statement();
    if (statement == nullptr) {
        throw ParserException("Invalid function statement after parenthesis", statement_begin_meta);
    }

    return std::make_shared<ASTStatementFunction>(ASTStatementFunction{
        .start_token_meta = statement_begin_meta,
        .name = func_name.value.value(),
        .parameters = parameters,
        .statement = statement,
    });
}
std::vector<ASTFunctionParam> Parser::parse_function_params() {
    std::vector<ASTFunctionParam> parameters;
    while (peek().has_value() && peek().value().type != TokenType::close_paren) {
        if (parameters.size() > 0) {
            assert_consume(TokenType::comma, "Expected comma after parameter and before closing paren ')'");
        }
        auto datatype = assert_consume(TokenType::identifier, "Expected parameter datatype");
        auto param_name = assert_consume(TokenType::identifier, "Expected parameter name");
        // TODO: check for initial value
        parameters.push_back(ASTFunctionParam{
            .start_token_meta = datatype.meta,
            .data_type_str = datatype.value.value(),
            .data_type = DataType::NONE,
            .name = param_name.value.value(),
        });
    }

    return parameters;
}

std::shared_ptr<ASTStatementFunctionCall> Parser::parse_statement_function_call() {
    if (!(test_peek(TokenType::identifier, 0) && test_peek(TokenType::open_paren, 1))) {
        return nullptr;
    }
    auto func_name_token = consume().value();
    consume();  // consume '('
    auto params = parse_statement_function_call_params();
    assert_consume(TokenType::close_paren, "Expected closing parenthesis ')'");
    assert_consume(TokenType::semicol, "Expected semicolon ';'");
    return std::make_shared<ASTStatementFunctionCall>(ASTStatementFunctionCall{
        .start_token_meta = func_name_token.meta,
        .parameters = params,
        .function_name = func_name_token.value.value(),
    });
}

std::vector<ASTExpression> Parser::parse_statement_function_call_params() {
    std::vector<ASTExpression> parameters;
    while (peek().has_value() && peek().value().type != TokenType::close_paren) {
        if (parameters.size() > 0) {
            assert_consume(TokenType::comma, "Expected comma after parameter and before closing paren ')'");
        }
        auto expression = parse_expression();
        if (!expression.has_value()) {
            auto nextToken = peek();
            if (nextToken.has_value()) {
                throw ParserException("Expected parameter expression", nextToken.value().meta);
            } else {
                throw ParserException("Expected expression after opening parenthesis '('");
            }
        }
        // TODO: check for initial value
        parameters.push_back(expression.value());
    }

    return parameters;
}

// throws ParserException if couldn't parse statement
std::shared_ptr<ASTStatement> Parser::parse_statement() {
    // check for exit statement
    auto nextToken = peek();
    if (!nextToken.has_value()) {
        throw ParserException("Expected statement");
    }
    auto meta = nextToken.value().meta;
    if (auto exit_statement = parse_statement_exit(); exit_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(exit_statement)});
    }

    // check for variable declaration statement
    if (auto var_declare_statement = parse_statement_var_declare(); var_declare_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(var_declare_statement)});
    }

    if (auto scope_statement = parse_statement_scope(); scope_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(scope_statement)});
    }

    if (auto if_statement = parse_statement_if(); if_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(if_statement)});
    }

    if (auto while_statement = parse_statement_while(); while_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(while_statement)});
    }

    if (auto assign_statement = parse_statement_var_assign(); assign_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(assign_statement)});
    }

    if (auto func_statement = parse_statement_function(); func_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(func_statement)});
    }

    if (auto func_call_statement = parse_statement_function_call(); func_call_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(func_call_statement)});
    }

    // fallback- no matching statement found
    throw ParserException("Invalid statement", meta);
}

ASTProgram Parser::parse_program() {
    ASTProgram result;

    while (peek().has_value()) {
        auto type = peek().value().type;
        // skip through comments
        if (type == TokenType::comment) {
            consume();
            continue;
        }
        auto statement = parse_statement();
        if (type == TokenType::_function) {
            // NOTE: could probably figure out a better way to know the statement is a function statement
            auto& func_statement = statement.get()->statement;
            result.functions.push_back(std::get<std::shared_ptr<ASTStatementFunction>>(func_statement));
            continue;
        }
        result.statements.push_back(statement);
    }

    return result;
}