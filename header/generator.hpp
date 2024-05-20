#pragma once
#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "AST_node.hpp"
#include "scope_stack.hpp"

extern std::map<DataType, size_t> data_type_size_bytes;

class Generator {
   public:
    Generator(ASTProgram program) : m_prog(program), m_stack_size(0), m_condition_counter(0) {}
    std::string generate_program();

   private:
    struct Variable;

    void generate_statement(const ASTStatement& statement);
    // Pushes the result expression onto the stack
    void generate_expression(const ASTExpression& expression);

    void generate_expression_identifier(const ASTIdentifier& identifier, size_t size_bytes);
    void generate_expression_int_literal(const ASTIntLiteral& literal, size_t size_bytes);
    void generate_expression_binary(const std::shared_ptr<ASTBinExpression>& binary, size_t size_bytes);

    void generate_statement_exit(const ASTStatementExit& exit_statement);
    void generate_statement_var_declare(const ASTStatementVar& var_statement);
    void generate_statement_var_assignment(const ASTStatementAssign& var_assign_statement);
    void generate_statement_scope(const ASTStatementScope& scope_statement);
    void generate_statement_if(const ASTStatementIf& if_statement);
    void generate_statement_while(const ASTStatementWhile& while_statement);
    void generate_statement_function(const ASTStatementFunction& function_statement);

    void enter_scope();
    void exit_scope();

    int get_variable_stack_offset(Generator::Variable& variable_data);
    Generator::Variable assert_get_variable_data(std::string variable_name);

    // push a value from the stack to the stack
    // data_size - the size of the original data
    // requested_size- the size of the data we want to store(type narrowing)
    void push_stack_offset(int offset, size_t data_size, size_t requested_size);

    // push a literal value to the stack
    void push_stack_literal(const std::string& value, size_t size);

    // push a register to the stack
    void push_stack_register(const std::string& reg, size_t size);

    // pop from the stack into a register
    void pop_stack_register(const std::string& reg, size_t size);

    std::stringstream m_generated;
    const ASTProgram m_prog;

    // map from variable name to variable details
    ScopeStack<Generator::Variable> m_stack;
    size_t m_stack_size;
    size_t m_condition_counter;

    struct StatementVisitor;
    struct ExpressionVisitor;

    struct Variable {
        // stack location in bytes
        size_t stack_location_bytes;

        // variable size in bytes
        size_t size_bytes;
    };
};