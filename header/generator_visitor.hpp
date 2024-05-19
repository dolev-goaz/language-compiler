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
    void operator()(const std::shared_ptr<ASTStatementAssign>& var_assign) const {
        generator.generate_statement_var_assignment(*var_assign.get());
    }
    void operator()(const std::shared_ptr<ASTStatementScope>& scope) const {
        generator.generate_statement_scope(*scope.get());
    }
    void operator()(const std::shared_ptr<ASTStatementIf>& if_statement) const {
        generator.generate_statement_if(*if_statement.get());
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
        generator.generate_expression_binary(binary, size);
    }

    void operator()(const std::shared_ptr<ASTAtomicExpression>& atomic) const {
        std::visit(Generator::ExpressionVisitor{.generator = generator, .size = size}, atomic.get()->value);
    }

    void operator()(const ASTParenthesisExpression& paren_expr) const {
        auto& inner_expression = *paren_expr.expression.get();
        std::visit(Generator::ExpressionVisitor{.generator = generator, .size = size}, inner_expression.expression);
    }
};