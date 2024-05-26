#include "parser.hpp"

std::shared_ptr<ASTStatementFunction> Parser::parse_statement_function() {
    if (!test_peek(TokenType::_function)) {
        return nullptr;
    }
    auto statement_begin_meta = consume().value().meta;
    auto return_data_type = assert_consume(TokenType::identifier, "Expected data type");
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
        .return_data_type_str = return_data_type.value.value(),
        .return_data_type = DataType::NONE,
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

std::shared_ptr<ASTStatementReturn> Parser::parse_statement_return() {
    if (!test_peek(TokenType::_return)) {
        return nullptr;
    }
    auto statement_begin_meta = consume().value().meta;
    auto expression = parse_expression();
    if (!expression.has_value()) {
        throw ParserException("Expected expression after 'return'", statement_begin_meta);
    }
    assert_consume(TokenType::semicol, "Expected semicolon ';' after return statement");
    return std::make_shared<ASTStatementReturn>(ASTStatementReturn{
        .start_token_meta = statement_begin_meta,
        .expression = expression.value(),
    });
}
