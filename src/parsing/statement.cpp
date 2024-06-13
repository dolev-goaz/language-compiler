#include "../header/parser.hpp"

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
        if_statement->fail_statement = fail_statement;
    }

    return if_statement;
}

std::shared_ptr<ASTStatementScope> Parser::parse_statement_scope() {
    if (!test_peek(TokenType::open_curly)) return nullptr;
    const TokenMeta statement_begin_meta = consume().value().meta;
    std::vector<std::shared_ptr<ASTStatement>> statements;
    while (peek().has_value() && !test_peek(TokenType::close_curly)) {
        if (peek().value().type == TokenType::comment) {
            consume();
            continue;
        }
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
    // [d_type] [pointer/array modifiers] [identifier];
    // [d_type] [pointer/array modifiers] [identifier] = [expression];
    if (!(test_peek(TokenType::identifier) && (test_peek(TokenType::star, 1) || test_peek(TokenType::open_square, 1) ||
                                               test_peek(TokenType::identifier, 1)))) {
        return nullptr;
    }
    auto meta = peek().value().meta;

    std::vector<Token> data_type_tokens = consume_data_type_tokens();
    Token identifier = assert_consume(TokenType::identifier, "Expected variable name");

    std::optional<ASTExpression> value = std::nullopt;

    if (test_peek(TokenType::eq)) {
        consume();  // consume 'eq' token
        auto expression = parse_expression();
        if (!expression.has_value()) {
            std::stringstream error_stream;
            error_stream << "Invalid initialize value for variable '" << identifier.value.value() << "'";
            throw ParserException(error_stream.str(), meta);
        }

        value = ASTExpression{
            .start_token_meta = expression.value().start_token_meta,
            .data_type = nullptr,
            .expression = std::move(expression.value().expression),
        };
    }

    assert_consume(TokenType::semicol, "Expected ';' after variable delcaration");

    return std::make_shared<ASTStatementVar>(ASTStatementVar{
        .start_token_meta = meta,
        .data_type_tokens = data_type_tokens,
        .data_type = nullptr,
        .name = identifier.value.value(),
        .value = std::move(value),
    });
}

std::shared_ptr<ASTStatementAssign> Parser::try_parse_statement_var_assign(
    const std::shared_ptr<ASTExpression>& operand) {
    // only if its in the form [identifier] = ....
    if (!test_peek(TokenType::eq)) {
        return nullptr;
    }

    auto& statement_meta = operand->start_token_meta;
    consume();  // eq operator
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Expected RHS expression", statement_meta);
    }
    assert_consume(TokenType::semicol, "Expected ';' after variable assignment");

    return std::make_shared<ASTStatementAssign>(ASTStatementAssign{
        .start_token_meta = statement_meta,
        .lhs = operand,
        .value = expression.value(),
    });
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

    if (auto func_statement = parse_statement_function(); func_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(func_statement)});
    }

    if (auto return_statement = parse_statement_return(); return_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(return_statement)});
    }

    if (auto func_call = parse_function_call(); func_call != nullptr) {
        assert_consume(TokenType::semicol, "Expected ';' after function call statement");
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(func_call)});
    }

    // all statements from here rely on an expression

    auto expression = parse_expression();
    if (!expression.has_value()) {
        // fallback- no matching statement found
        throw ParserException("Invalid statement", meta);
    }
    auto shared_ptr_expr = std::make_shared<ASTExpression>(expression.value());
    if (auto assign_statement = try_parse_statement_var_assign(shared_ptr_expr); assign_statement != nullptr) {
        return std::make_shared<ASTStatement>(
            ASTStatement{.start_token_meta = meta, .statement = std::move(assign_statement)});
    }

    assert(false && "In the future expressions will be statements as well");
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
            auto& func_statement = statement->statement;
            result.functions.push_back(std::get<std::shared_ptr<ASTStatementFunction>>(func_statement));
            continue;
        }
        result.statements.push_back(statement);
    }

    return result;
}