#pragma once
#include "generator.hpp"

struct Generator::StatementVisitor {
    Generator& generator;

    void operator()(const ASTStatementExit& exit) const { generator.generate_exit(exit); }
};

struct Generator::ExpressionVisitor {
    Generator& generator;

    std::string operator()(const ASTIdentifier& identifier) const {
        std::cerr << "Not Implemented Yet!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string operator()(const ASTIntLiteral& literal) const { return literal.value; }
};