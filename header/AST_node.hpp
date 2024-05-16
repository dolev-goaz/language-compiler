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

struct ASTAtomicExpression {
    TokenMeta start_token_meta;
    std::variant<ASTIntLiteral, ASTIdentifier> value;
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

struct ASTStatement {
    TokenMeta start_token_meta;
    std::variant<std::shared_ptr<ASTStatementExit>, std::shared_ptr<ASTStatementVar>> statement;
};

struct ASTProgram {
    std::vector<std::shared_ptr<ASTStatement>> statements;
};
