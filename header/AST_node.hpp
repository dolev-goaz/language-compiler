#pragma once
#include <string>
#include <variant>
#include <vector>

#include "./token.hpp"

struct ASTIntLiteral {
    Token value;
};

struct ASTIdentifier {
    Token value;
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
