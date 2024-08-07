#include "parser.hpp"

std::map<TokenType, BinOperation> singleCharBinOperationMapping = {
    {TokenType::plus, BinOperation::add},       {TokenType::minus, BinOperation::subtract},
    {TokenType::star, BinOperation::multiply},  {TokenType::fslash, BinOperation::divide},
    {TokenType::percent, BinOperation::modulo},
};

BinOperation Parser::peek_binary_operation() {
    if (!peek().has_value()) return BinOperation::NONE;

    // check for multi-token operations
    if (test_peek(TokenType::eq)) {
        //"="
        if (test_peek(TokenType::eq, 1)) {
            //"=="
            return BinOperation::eq;
        }
        // NOTE: return assign here(in the future)
    }
    if (test_peek(TokenType::open_triangle)) {
        //"<"
        consume();
        if (test_peek(TokenType::eq, 1)) {
            //"<="
            return BinOperation::le;
        }
        return BinOperation::lt;
    }
    if (test_peek(TokenType::close_triangle)) {
        //">"
        if (test_peek(TokenType::eq, 1)) {
            //">="
            return BinOperation::ge;
        }
        return BinOperation::gt;
    }

    // check for single-token operations
    if (singleCharBinOperationMapping.count(peek().value().type) > 0) {
        return singleCharBinOperationMapping.at(peek().value().type);
    }

    return BinOperation::NONE;
}

BinOperation Parser::try_consume_binary_operation() {
    auto to_consume = peek_binary_operation();
    if (to_consume == BinOperation::NONE) {
        return BinOperation::NONE;
    }

    size_t consume_count = 0;
    switch (to_consume) {
        case BinOperation::add:
        case BinOperation::subtract:
        case BinOperation::multiply:
        case BinOperation::divide:
        case BinOperation::modulo:
        case BinOperation::lt:
        case BinOperation::gt:
            consume_count = 1;
            break;
        case BinOperation::eq:
        case BinOperation::le:
        case BinOperation::ge:
            consume_count = 2;
            break;
        default:
            assert(false && "Unknown binary operation- " + (int)to_consume);
    }
    for (size_t i = 0; i < consume_count; i++) {
        consume();
    }

    return to_consume;
}

std::optional<UnaryOperation> Parser::peek_unary_operation() {
    static_assert((int)UnaryOperation::operationCount - 1 == 3, "Implemented unary operations without updating parser");

    if (!peek().has_value()) return std::nullopt;
    if (test_peek(TokenType::minus)) {
        // NOTE: could add support for --, ++, so on
        return UnaryOperation::negate;
    }
    if (test_peek(TokenType::ampersand)) {
        return UnaryOperation::dereference;
    }
    if (test_peek(TokenType::star)) {
        return UnaryOperation::reference;
    }
    return std::nullopt;
}

UnaryOperation Parser::assert_consume_unary_operation() {
    static_assert((int)UnaryOperation::operationCount - 1 == 3, "Implemented unary operations without updating parser");

    auto unary_operation_opt = peek_unary_operation();
    if (!unary_operation_opt.has_value()) {
        auto msg = "Expected Unary Operation";
        if (auto token = peek(); token.has_value()) {
            throw ParserException(msg, token.value().meta);
        }
        throw ParserException(msg);
    }

    auto unary_operation = unary_operation_opt.value();
    size_t consume_count = 0;
    switch (unary_operation) {
        case UnaryOperation::negate:
        case UnaryOperation::dereference:
        case UnaryOperation::reference:
            consume_count = 1;
            break;
        default:
            assert(false && "Unknown unary operation- " + (int)unary_operation);
    }
    for (size_t i = 0; i < consume_count; i++) {
        consume();
    }

    return unary_operation;
}

std::optional<int> Parser::binary_operator_precedence(const BinOperation& operation) {
    static_assert((int)BinOperation::operationCount - 1 == 10,
                  "Binary Operations enum changed without changing precendence mapping");
    switch (operation) {
        case BinOperation::multiply:
        case BinOperation::divide:
        case BinOperation::modulo:
            return 14;
        case BinOperation::add:
        case BinOperation::subtract:
            return 13;
        case BinOperation::ge:
        case BinOperation::gt:
        case BinOperation::le:
        case BinOperation::lt:
            return 11;
        case BinOperation::eq:
            return 10;
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
    if (auto func_call = parse_function_call(); func_call != nullptr) {
        return std::make_shared<ASTAtomicExpression>(
            ASTAtomicExpression{.start_token_meta = meta, .value = *func_call});
    }
    if (test_peek(TokenType::identifier)) {
        Token name = consume().value();
        // variable
        return std::make_shared<ASTAtomicExpression>(ASTAtomicExpression{
            .start_token_meta = meta,
            .value = ASTIdentifier{.start_token_meta = meta, .value = name.value.value()},
        });
    }
    if (test_peek(TokenType::quote)) {
        consume();
        auto char_value = assert_consume(TokenType::identifier, "Expected char value");
        assert_consume(TokenType::quote, "Expected closing quote for char value");
        auto& inner_value = char_value.value.value();
        // can't be 0 since we consumed an identifier
        if (inner_value.size() > 1) {
            throw ParserException("Char value can only contain a singular character", char_value.meta);
        }
        return std::make_shared<ASTAtomicExpression>(ASTAtomicExpression{
            .start_token_meta = meta,
            .value = ASTCharLiteral{.start_token_meta = meta, .value = inner_value.at(0)},
        });
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
    if (auto array_initializer = try_parse_array_initializer(); array_initializer.has_value()) {
        return std::make_shared<ASTAtomicExpression>(
            ASTAtomicExpression{.start_token_meta = meta, .value = array_initializer.value()});
    }

    return nullptr;
}

std::shared_ptr<ASTUnaryExpression> Parser::try_parse_unary() {
    if (!peek_unary_operation().has_value()) {
        return nullptr;
    }
    auto unary_operation = assert_consume_unary_operation();
    // TODO: don't allow binary operation here
    // for example, -x + 5 should be parse (-x) + 5, not -(x + 5)
    auto operand = try_parse_expr_lhs();
    if (operand == nullptr) {
        auto nextToken = peek();
        if (nextToken.has_value()) {
            throw ParserException("Expected operand for unary expression", nextToken.value().meta);
        } else {
            throw ParserException("Expected operand for unary expression");
        }
    }
    return std::make_shared<ASTUnaryExpression>(ASTUnaryExpression{
        .start_token_meta = operand->start_token_meta,
        .operation = unary_operation,
        .expression = operand,
    });
}

std::shared_ptr<ASTArrayIndexExpression> Parser::try_parse_array_indexing(
    const std::shared_ptr<ASTExpression>& operand) {
    if (!test_peek(TokenType::open_square)) {
        return nullptr;
    }
    consume();  // consume open square
    auto index = parse_expression();
    if (!index.has_value()) {
        auto nextToken = peek();
        if (nextToken.has_value()) {
            throw ParserException("Expected index expression", nextToken.value().meta);
        } else {
            throw ParserException("Expected index expression");
        }
    }
    assert_consume(TokenType::close_square, "Expected closing ']' after indexing");
    return std::make_shared<ASTArrayIndexExpression>(ASTArrayIndexExpression{
        .start_token_meta = operand->start_token_meta,
        .index = std::make_shared<ASTExpression>(index.value()),
        .expression = operand,
    });
}

std::shared_ptr<ASTExpression> Parser::try_parse_expr_lhs() {
    std::shared_ptr<ASTExpression> expr = nullptr;
    auto unary = try_parse_unary();
    if (unary != nullptr) {
        expr = std::make_shared<ASTExpression>(ASTExpression{
            .start_token_meta = unary->start_token_meta,
            .data_type = nullptr,
            .expression = unary,
        });
    }
    auto atomic = try_parse_atomic();
    if (atomic != nullptr) {
        expr = std::make_shared<ASTExpression>(ASTExpression{
            .start_token_meta = atomic->start_token_meta,
            .data_type = nullptr,
            .expression = atomic,
        });
    }

    if (expr != nullptr) {
        while (test_peek(TokenType::open_square)) {
            expr = std::make_shared<ASTExpression>(ASTExpression{
                .is_literal = false,
                .start_token_meta = expr->start_token_meta,
                .data_type = nullptr,
                .expression = try_parse_array_indexing(expr),
            });
        }
    }
    return expr;
}

ASTExpression Parser::convert_char_to_expression(char ch, const TokenMeta& pos) {
    ASTCharLiteral literal = {
        .start_token_meta = pos,
        .value = ch,
    };
    ASTAtomicExpression expression = {
        .start_token_meta = pos,
        .value = literal,
    };
    return ASTExpression{
        .is_literal = true,
        .start_token_meta = pos,
        .data_type = nullptr,
        .expression = std::make_shared<ASTAtomicExpression>(expression),
    };
}

std::optional<ASTArrayInitializer> Parser::try_parse_string_as_array_initializer() {
    if (!test_peek(TokenType::string)) {
        return std::nullopt;
    }
    auto start_token = consume().value();
    auto str = start_token.value.value();
    std::vector<ASTExpression> characters;
    for (auto&& character : str) {
        characters.push_back(convert_char_to_expression(character, start_token.meta));
    }
    characters.push_back(convert_char_to_expression('\0', start_token.meta));

    return ASTArrayInitializer{
        .start_token_meta = start_token.meta,
        .initialize_values = characters,
    };
}

std::optional<ASTArrayInitializer> Parser::try_parse_array_initializer() {
    auto string_array_initializer = try_parse_string_as_array_initializer();
    if (string_array_initializer.has_value()) {
        return string_array_initializer;
    }

    if (!test_peek(TokenType::open_curly)) {
        return std::nullopt;
    }
    auto start_token = consume().value();
    // NOTE: pretty much the same as parse_function_call_params
    std::vector<ASTExpression> members;
    while (peek().has_value() && peek().value().type != TokenType::close_curly) {
        if (members.size() > 0) {
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
        members.push_back(expression.value());
    }

    assert_consume(TokenType::close_curly, "Expected '}' after array initializer");

    return ASTArrayInitializer{
        .start_token_meta = start_token.meta,
        .initialize_values = members,
    };
}

std::optional<ASTExpression> Parser::parse_expression(const int min_prec) {
    auto expr_lhs = try_parse_expr_lhs();
    if (expr_lhs == nullptr) {
        return std::nullopt;
    }
    while (true) {
        auto binOperation = peek_binary_operation();
        if (binOperation == BinOperation::NONE) {
            break;
        }
        std::optional<int> currentPrecedence = binary_operator_precedence(binOperation);
        if (!currentPrecedence.has_value() || currentPrecedence.value() < min_prec) {
            break;
        }
        try_consume_binary_operation();
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
            .start_token_meta = expr_lhs->start_token_meta,
            .data_type = nullptr,
            .expression = expr_lhs->expression,
        });
        auto new_expr_rhs = std::make_shared<ASTExpression>(rhs.value());
        expr_lhs->expression = std::make_shared<ASTBinExpression>(ASTBinExpression{
            .start_token_meta = new_expr_lhs->start_token_meta,
            .operation = binOperation,
            .lhs = new_expr_lhs,
            .rhs = new_expr_rhs,
        });
    }

    return *expr_lhs;  // TODO: should probably just return the shared_ptr
}
