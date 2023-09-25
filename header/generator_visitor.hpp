#pragma once
#include "generator.hpp"

struct Generator::StatementVisitor {
    Generator& generator;

    void operator()(const ASTStatementExit& exit) const { generator.generate_exit(exit); }
};

struct Generator::ExpressionVisitor {
    Generator& generator;

    void operator()(const ASTIdentifier& identifier) const {
        std::cerr << "Not Implemented Yet!" << std::endl;
        exit(EXIT_FAILURE);
    }

    void operator()(const ASTIntLiteral& literal) const {
        generator.m_generated << "\tpush QWORD " << literal.value << std::endl;
    }
};