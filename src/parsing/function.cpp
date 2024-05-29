#include "parser.hpp"

std::shared_ptr<ASTStatementFunction> Parser::parse_statement_function() {
    if (!test_peek(TokenType::_function)) {
        return nullptr;
    }
    auto statement_begin_meta = consume().value().meta;
    auto data_type_tokens = consume_data_type_tokens();
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
        .return_data_type_tokens = data_type_tokens,
        .return_data_type = nullptr,
    });
}

std::vector<ASTFunctionParam> Parser::parse_function_params() {
    std::vector<ASTFunctionParam> parameters;
    while (peek().has_value() && peek().value().type != TokenType::close_paren) {
        if (parameters.size() > 0) {
            assert_consume(TokenType::comma, "Expected comma after parameter and before closing paren ')'");
        }
        auto meta = peek().value().meta;
        auto data_type_tokens = consume_data_type_tokens();
        auto param_name = assert_consume(TokenType::identifier, "Expected parameter name");
        // TODO: check for initial value
        parameters.push_back(ASTFunctionParam{
            .start_token_meta = meta,
            .data_type_tokens = data_type_tokens,
            .data_type = nullptr,
            .name = param_name.value.value(),
        });
    }

    return parameters;
}

std::vector<ASTExpression> Parser::parse_function_call_params() {
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

std::shared_ptr<ASTStatementReturn> Parser::parse_statement_return() {
    if (!test_peek(TokenType::_return)) {
        return nullptr;
    }
    auto statement_begin_meta = consume().value().meta;
    auto possible_expression = parse_expression();
    assert_consume(TokenType::semicol, "Expected semicolon ';' after return statement");
    return std::make_shared<ASTStatementReturn>(ASTStatementReturn{
        .start_token_meta = statement_begin_meta,
        .expression = possible_expression,
    });
}

std::shared_ptr<ASTFunctionCall> Parser::parse_function_call() {
    if (!test_peek(TokenType::identifier, 0) || !test_peek(TokenType::open_paren, 1)) {
        return nullptr;
    }

    auto name_token = consume().value();
    consume();  // open parenthesis
    auto params = parse_function_call_params();
    assert_consume(TokenType::close_paren, "Expected closing parenthesis ')' after functionc call");
    auto value = ASTFunctionCall{
        .start_token_meta = name_token.meta,
        .parameters = params,
        .function_name = name_token.value.value(),
        .return_data_type = nullptr,
    };

    return std::make_shared<ASTFunctionCall>(value);
}