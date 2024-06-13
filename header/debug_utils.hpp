#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "AST_node.hpp"

namespace debug_utils {
std::string print_indentation(int level);

std::string visualize_ast(const std::shared_ptr<ASTProgram>& program);

std::string visualize_expression(const std::shared_ptr<ASTExpression>& expr);

std::string visualize_statement(const std::shared_ptr<ASTStatement>& stmt, int level);

std::string visualize_function_call(const ASTFunctionCall& funcCall);

std::string visualize_atomic_expression(const ASTAtomicExpression& atomicExpr);

std::string visualize_initializer_expression(const ASTArrayInitializer& initializer_expr);

std::string visualize_statement_exit(const ASTStatementExit& stmt);

std::string visualize_statement_var(const ASTStatementVar& stmt);

std::string visualize_statement_assign(const ASTStatementAssign& stmt);

std::string visualize_statement_scope(const ASTStatementScope& stmt, int level);

std::string visualize_statement_if(const ASTStatementIf& stmt, int level);

std::string visualize_statement_while(const ASTStatementWhile& stmt, int level);

std::string visualize_statement_function(const ASTStatementFunction& stmt, int level);

std::string visualize_statement_return(const ASTStatementReturn& stmt);
}