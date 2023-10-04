#pragma once
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

struct ASTIntLiteral {
    // literal value- 0x1f, 15, 0b1001..
    std::string value;
};

struct ASTIdentifier {
    // literal value- variable name, function name..
    std::string value;
};

struct ASTExpression {
    DataType data_type;
    std::variant<ASTIdentifier, ASTIntLiteral> expression;
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
