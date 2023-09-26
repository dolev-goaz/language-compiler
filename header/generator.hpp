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
    // Pushes the result expression onto the stack
    void generate_expression(const ASTExpression& expression);

    void generate_expression_identifier(const ASTIdentifier& identifier);
    void generate_expression_int_literal(const ASTIntLiteral& literal);

    void generate_statement_exit(const ASTStatementExit& exit_statement);
    void generate_statement_var_declare(const ASTStatementVar& var_statement);

    // push a value from the stack to the stack
    void push_stack_offset(int offset, size_t size);

    // push a literal value to the stack
    void push_stack_literal(const std::string& value, size_t size);

    // TODO: might need push_stack_register

    // pop from the stack into a register
    void pop_stack_register(const std::string& reg, size_t size);

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