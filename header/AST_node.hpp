#pragma once
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

struct ASTStatement {
    std::variant<ASTStatementExit> statement;
};

struct ASTProgram {
    std::vector<ASTStatement> statements;
};
