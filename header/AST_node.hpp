#pragma once
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct ASTIntLiteral {
    std::string value;
};

struct ASTIdentifier {
    std::string value;
};

struct ASTExpression {
    std::variant<ASTIdentifier, ASTIntLiteral> expression;
};

struct ASTStatementExit {
    ASTExpression status_code;
};

struct ASTStatementVar {
    std::string name;
    std::optional<ASTExpression> value;
};

struct ASTStatement {
    std::variant<ASTStatementExit, ASTStatementVar> statement;
};

struct ASTProgram {
    std::vector<ASTStatement> statements;
};
