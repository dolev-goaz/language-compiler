#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "data_type.hpp"
#include "lexer.hpp"

enum class BinOperation {
    NONE = 0,
    add,
    subtract,
    multiply,
    divide,
    modulo,

    eq,  // comparison equals
    lt,
    le,
    gt,
    ge,
    operationCount,  // used for assertions
};

enum class UnaryOperation {
    NONE = 0,
    negate,          // -x
    dereference,     // &x
    reference,       // *x
    operationCount,  // used for assertions
};

struct ASTExpression;
struct ASTStatement;

struct ASTCharLiteral {
    TokenMeta start_token_meta;
    char value;
};

struct ASTIntLiteral {
    TokenMeta start_token_meta;
    // literal value- 0x1f, 15, 0b1001..
    std::string value;
};

struct ASTIdentifier {
    TokenMeta start_token_meta;
    // literal value- variable name, function name..
    std::string value;
};

struct ASTParenthesisExpression {
    TokenMeta start_token_meta;
    std::shared_ptr<ASTExpression> expression;
};

struct ASTFunctionCall {
    TokenMeta start_token_meta;
    std::vector<ASTExpression> parameters;
    std::string function_name;
    std::shared_ptr<DataType> return_data_type;
};

struct ASTArrayInitializer {
    TokenMeta start_token_meta;
    std::vector<ASTExpression> initialize_values;
};

struct ASTAtomicExpression {
    TokenMeta start_token_meta;
    std::variant<ASTIntLiteral, ASTIdentifier, ASTParenthesisExpression, ASTFunctionCall, ASTCharLiteral,
                 ASTArrayInitializer>
        value;
};

struct ASTBinExpression {
    TokenMeta start_token_meta;
    BinOperation operation;
    std::shared_ptr<ASTExpression> lhs;
    std::shared_ptr<ASTExpression> rhs;
};

struct ASTUnaryExpression {
    TokenMeta start_token_meta;
    UnaryOperation operation;
    std::shared_ptr<ASTExpression> expression;
};

struct ASTArrayIndexExpression {
    TokenMeta start_token_meta;
    std::shared_ptr<ASTExpression> index;
    std::shared_ptr<ASTExpression> expression;
};

struct ASTExpression {
    bool is_literal = false;
    TokenMeta start_token_meta;
    std::shared_ptr<DataType> data_type;
    std::variant<std::shared_ptr<ASTAtomicExpression>, std::shared_ptr<ASTBinExpression>,
                 std::shared_ptr<ASTUnaryExpression>, std::shared_ptr<ASTArrayIndexExpression>>
        expression;
};

struct ASTStatementExit {
    TokenMeta start_token_meta;
    ASTExpression status_code;
};

struct ASTStatementVar {
    TokenMeta start_token_meta;
    std::vector<Token> data_type_tokens;

    std::shared_ptr<DataType> data_type;
    std::string name;
    std::optional<ASTExpression> value;
};

struct ASTStatementAssign {
    TokenMeta start_token_meta;
    std::shared_ptr<ASTExpression> lhs;
    ASTExpression value;
};

struct ASTStatementScope {
    TokenMeta start_token_meta;
    std::vector<std::shared_ptr<ASTStatement>> statements;
};

struct ASTStatementIf {
    TokenMeta start_token_meta;
    ASTExpression expression;
    std::shared_ptr<ASTStatement> success_statement;
    std::shared_ptr<ASTStatement> fail_statement;
};

struct ASTStatementWhile {
    TokenMeta start_token_meta;
    ASTExpression expression;
    std::shared_ptr<ASTStatement> success_statement;
};

// NOTE: same as ASTStatementVar
struct ASTFunctionParam {
    TokenMeta start_token_meta;
    std::vector<Token> data_type_tokens;
    std::shared_ptr<DataType> data_type;
    std::string name;
    // std::optional<ASTExpression> initial_value; // TODO: support initial value
};

struct ASTStatementFunction {
    TokenMeta start_token_meta;
    std::string name;
    std::vector<ASTFunctionParam> parameters;
    std::shared_ptr<ASTStatement> statement;

    std::vector<Token> return_data_type_tokens;
    std::shared_ptr<DataType> return_data_type;
};

struct ASTStatementReturn {
    TokenMeta start_token_meta;
    std::optional<ASTExpression> expression;
};

struct ASTStatement {
    TokenMeta start_token_meta;
    std::variant<
        std::shared_ptr<ASTStatementExit>, std::shared_ptr<ASTStatementVar>, std::shared_ptr<ASTStatementScope>,
        std::shared_ptr<ASTStatementIf>, std::shared_ptr<ASTStatementAssign>, std::shared_ptr<ASTStatementWhile>,
        std::shared_ptr<ASTStatementFunction>, std::shared_ptr<ASTStatementReturn>, std::shared_ptr<ASTFunctionCall>>
        statement;
};

struct ASTProgram {
    std::vector<std::shared_ptr<ASTStatement>> statements;
    std::vector<std::shared_ptr<ASTStatementFunction>> functions;
};
