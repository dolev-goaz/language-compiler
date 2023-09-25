#pragma once
#include <iostream>
#include <sstream>
#include <string>

#include "AST_node.hpp"

class Generator {
   public:
    Generator(ASTProgram program) : m_prog(program) {}
    std::string generate_program();

   private:
    void generate_statement(const ASTStatement& statement);
    void generate_exit(const ASTStatementExit& exit_statement);

    // Pushes the result expression onto the stack
    void generate_expression(const ASTExpression& expression);

    std::stringstream m_generated;
    const ASTProgram m_prog;

    struct StatementVisitor;
    struct ExpressionVisitor;
};