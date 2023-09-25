#pragma once
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "AST_node.hpp"

class Generator {
   public:
    Generator(ASTProgram program) : m_prog(program) {}
    std::string generate_program();

   private:
    struct Variable;

    void generate_statement(const ASTStatement& statement);
    void generate_exit(const ASTStatementExit& exit_statement);

    // Pushes the result expression onto the stack
    void generate_expression(const ASTExpression& expression);

    std::stringstream m_generated;
    const ASTProgram m_prog;

    // map from variable name to variable details
    std::map<std::string, Generator::Variable> m_variables;
    size_t m_stack_size;

    struct StatementVisitor;
    struct ExpressionVisitor;

    struct Variable {
        // stack location in bytes
        size_t stack_location_bytes;

        // variable size in bytes
        size_t size_bytes;
    };
};