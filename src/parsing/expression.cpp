#include "parser.hpp"

std::map<TokenType, BinOperation> singleCharBinOperationMapping = {
    {TokenType::plus, BinOperation::add},       {TokenType::minus, BinOperation::subtract},
    {TokenType::star, BinOperation::multiply},  {TokenType::fslash, BinOperation::divide},
    {TokenType::percent, BinOperation::modulo},
};
BinOperation Parser::try_consume_binary_operation() {
    if (!peek().has_value()) return BinOperation::NONE;

    // check for multi-token operations
    if (test_peek(TokenType::eq)) {
        //"="
        consume();
        if (test_peek(TokenType::eq)) {
            //"=="
            consume();
            return BinOperation::eq;
        }
        // NOTE: return assign here(in the future)
    }
    if (test_peek(TokenType::open_triangle)) {
        //"<"
        consume();
        if (test_peek(TokenType::eq)) {
            //"<="
            consume();
            return BinOperation::le;
        }
        return BinOperation::lt;
    }
    if (test_peek(TokenType::close_triangle)) {
        //">"
        consume();
        if (test_peek(TokenType::eq)) {
            //">="
            consume();
            return BinOperation::ge;
        }
        return BinOperation::gt;
    }

    // check for single-token operations
    if (singleCharBinOperationMapping.count(peek().value().type) > 0) {
        auto current = consume();
        return singleCharBinOperationMapping.at(current.value().type);
    }

    return BinOperation::NONE;
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
    if (test_peek(TokenType::identifier)) {
        // TODO: could refactor this code to seperate methods
        Token name = consume().value();
        std::variant<ASTIntLiteral, ASTIdentifier, ASTParenthesisExpression, ASTFunctionCallExpression> value;
        if (test_peek(TokenType::open_paren)) {
            // function call
            consume();
            auto params = parse_statement_function_call_params();
            assert_consume(TokenType::close_paren, "Expected closing parenthesis ')' after functionc call");
            value = ASTFunctionCallExpression{
                .start_token_meta = name.meta,
                .parameters = params,
                .function_name = name.value.value(),
                .return_data_type = DataType::NONE,
            };
        } else {
            // variable
            value = ASTIdentifier{.start_token_meta = meta, .value = name.value.value()};
        }
        return std::make_shared<ASTAtomicExpression>(ASTAtomicExpression{
            .start_token_meta = meta,
            .value = value,
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
        auto binOperation = try_consume_binary_operation();
        if (binOperation == BinOperation::NONE) {
            break;
        }
        std::optional<int> currentPrecedence = binary_operator_precedence(binOperation);
        if (!currentPrecedence.has_value() || currentPrecedence.value() < min_prec) {
            break;
        }
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