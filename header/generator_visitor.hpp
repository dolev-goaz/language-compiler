#pragma once
#include "generator.hpp"

struct Generator::StatementVisitor {
    Generator& generator;

    void operator()(const ASTStatementExit& exit) const { generator.generate_statement_exit(exit); }
    void operator()(const ASTStatementVar& var_declare) const { generator.generate_statement_var_declare(var_declare); }
};

struct Generator::ExpressionVisitor {
    Generator& generator;

    void operator()(const ASTIdentifier& identifier) const { generator.generate_expression_identifier(identifier); }

    void operator()(const ASTIntLiteral& literal) const { generator.generate_expression_int_literal(literal); }
};