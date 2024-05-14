#pragma once
#include "generator.hpp"

struct Generator::StatementVisitor {
    Generator& generator;

    void operator()(const std::shared_ptr<ASTStatementExit>& exit) const {
        generator.generate_statement_exit(*exit.get());
    }
    void operator()(const std::shared_ptr<ASTStatementVar>& var_declare) const {
        generator.generate_statement_var_declare(*var_declare.get());
    }
};

struct Generator::ExpressionVisitor {
    Generator& generator;
    size_t size;

    void operator()(const ASTIdentifier& identifier) const {
        generator.generate_expression_identifier(identifier, size);
    }

    void operator()(const ASTIntLiteral& literal) const { generator.generate_expression_int_literal(literal, size); }

    void operator()(const std::shared_ptr<ASTBinExpression>& binary) const {
        std::cerr << "TODO: Generator Visitor Binary Expression" << std::endl;
        (void)binary;
    }

    void operator()(const std::shared_ptr<ASTAtomicExpression>& atomic) const {
        std::visit(Generator::ExpressionVisitor{.generator = generator, .size = size}, atomic.get()->value);
    }
};