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
    if (!peek().has_value()) return std::nullopt;
    if (test_peek(TokenType::minus)) {
        // NOTE: could add support for --, ++, so on
        return UnaryOperation::negate;
    }
    return std::nullopt;
}

UnaryOperation Parser::assert_consume_unary_operation() {
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
            ASTAtomicExpression{.start_token_meta = meta, .value = *func_call.get()});
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

std::shared_ptr<ASTExpression> Parser::try_parse_expr_lhs() {
    auto unary = try_parse_unary();
    if (unary != nullptr) {
        return std::make_shared<ASTExpression>(ASTExpression{
            .start_token_meta = unary->start_token_meta,
            .data_type = nullptr,
            .expression = unary,
        });
    }
    auto atomic = try_parse_atomic();
    if (atomic != nullptr) {
        return std::make_shared<ASTExpression>(ASTExpression{
            .start_token_meta = atomic->start_token_meta,
            .data_type = nullptr,
            .expression = atomic,
        });
    }
    return nullptr;
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
        expr_lhs->expression = std::make_shared<ASTBinExpression>(
            ASTBinExpression{.start_token_meta = new_expr_lhs.get()->start_token_meta,
                             .operation = binOperation,
                             .lhs = new_expr_lhs,
                             .rhs = new_expr_rhs});
    }

    return *expr_lhs.get(); // TODO: should probably just return the shared_ptr
}
