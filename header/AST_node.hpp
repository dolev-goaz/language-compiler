#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

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
};

struct ASTExpression;

struct ASTIntLiteral {
    // literal value- 0x1f, 15, 0b1001..
    std::string value;
};

struct ASTIdentifier {
    // literal value- variable name, function name..
    std::string value;
};

struct ASTAtomicExpression {
    std::variant<ASTIntLiteral, ASTIdentifier> value;
};

struct ASTBinExpression {
    BinOperation operation;
    std::unique_ptr<ASTExpression> lhs;
    std::unique_ptr<ASTExpression> rhs;
};

struct ASTExpression {
    DataType data_type;
    std::variant<ASTAtomicExpression, ASTBinExpression> expression;
};

struct ASTStatementExit {
    ASTExpression status_code;
};

struct ASTStatementVar {
    std::string data_type_str;
    DataType data_type;
    std::string name;
    std::optional<ASTExpression> value;
};

struct ASTStatement {
    std::variant<ASTStatementExit, ASTStatementVar> statement;
};

struct ASTProgram {
    std::vector<ASTStatement> statements;
};
