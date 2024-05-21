#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lexer.hpp"

enum class DataType {
    NONE = 0,
    int_8,
    int_16,
    int_32,
    int_64,
};

enum class BinOperation {
    NONE = 0,
    add,
    subtract,
    multiply,
    divide,
    modulo,
    operationCount,  // used for assertions
};

struct ASTExpression;
struct ASTStatement;

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

// TODO: when adding return values, this should be an expression
struct ASTFunctionCallExpression {
    TokenMeta start_token_meta;
    std::vector<ASTExpression> parameters;
    std::string function_name;
};

struct ASTAtomicExpression {
    TokenMeta start_token_meta;
    std::variant<ASTIntLiteral, ASTIdentifier, ASTParenthesisExpression, ASTFunctionCallExpression> value;
};

struct ASTBinExpression {
    TokenMeta start_token_meta;
    BinOperation operation;
    std::shared_ptr<ASTExpression> lhs;
    std::shared_ptr<ASTExpression> rhs;
};

struct ASTExpression {
    TokenMeta start_token_meta;
    DataType data_type;
    std::variant<std::shared_ptr<ASTAtomicExpression>, std::shared_ptr<ASTBinExpression>> expression;
};

struct ASTStatementExit {
    TokenMeta start_token_meta;
    ASTExpression status_code;
};

struct ASTStatementVar {
    TokenMeta start_token_meta;
    std::string data_type_str;
    DataType data_type;
    std::string name;
    std::optional<ASTExpression> value;
};

struct ASTStatementAssign {
    TokenMeta start_token_meta;
    std::string name;
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
    std::string data_type_str;
    DataType data_type;
    std::string name;
    // std::optional<ASTExpression> initial_value; // TODO: support initial value
};

struct ASTStatementFunction {
    TokenMeta start_token_meta;
    std::string name;
    std::vector<ASTFunctionParam> parameters;
    std::shared_ptr<ASTStatement> statement;
};

struct ASTStatementReturn {
    TokenMeta start_token_meta;
    ASTExpression expression;
};

struct ASTStatement {
    TokenMeta start_token_meta;
    std::variant<std::shared_ptr<ASTStatementExit>, std::shared_ptr<ASTStatementVar>,
                 std::shared_ptr<ASTStatementScope>, std::shared_ptr<ASTStatementIf>,
                 std::shared_ptr<ASTStatementAssign>, std::shared_ptr<ASTStatementWhile>,
                 std::shared_ptr<ASTStatementFunction>, std::shared_ptr<ASTStatementReturn>>
        statement;
};

struct ASTProgram {
    std::vector<std::shared_ptr<ASTStatement>> statements;
    std::vector<std::shared_ptr<ASTStatementFunction>> functions;
};
