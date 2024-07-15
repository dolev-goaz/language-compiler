#pragma once
#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "AST_node.hpp"
#include "scope_stack.hpp"

extern std::map<size_t, std::string> size_bytes_to_size_keyword;
extern std::map<size_t, std::string> size_bytes_to_register;
extern std::map<BinOperation, std::string> comparison_operation;

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
    void generate_expression_char_literal(const ASTCharLiteral& literal, size_t size_bytes);
    void generate_expression_array_initializer(const ASTArrayInitializer& array_initializer);

    void generate_expression_unary(const std::shared_ptr<ASTUnaryExpression>& unary, size_t size_bytes);
    void generate_expression_binary(const std::shared_ptr<ASTBinExpression>& binary, size_t size_bytes);
    void generate_expression_array_index(const std::shared_ptr<ASTArrayIndexExpression>& array_index,
                                         size_t return_size_bytes);
    void generate_expression_function_call(const ASTFunctionCall& function_call_expr, size_t return_size_bytes);

    void generate_statement_exit(const std::shared_ptr<ASTStatementExit>& exit_statement);
    void generate_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_statement);
    void generate_statement_var_assignment(const std::shared_ptr<ASTStatementAssign>& var_assign_statement);
    void generate_statement_scope(const std::shared_ptr<ASTStatementScope>& scope_statement);
    void generate_statement_if(const std::shared_ptr<ASTStatementIf>& if_statement);
    void generate_statement_while(const std::shared_ptr<ASTStatementWhile>& while_statement);
    void generate_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement);
    void generate_statement_return(const std::shared_ptr<ASTStatementReturn>& return_statement);

    void enter_scope();
    void exit_scope();

    void load_memory_address_var(const ASTIdentifier& variable);
    void load_memory_address_arr_index(const std::shared_ptr<ASTArrayIndexExpression>& index_expr);
    void load_memory_address_expr(const ASTExpression& expression);
    Generator::Variable assert_get_variable_data(std::string variable_name);

    // push a literal value to the stack
    void push_stack_literal(const std::string& value, size_t size);

    // push a register to the stack
    void push_stack_register(const std::string& reg, size_t size);

    // pop from the stack into a register
    void pop_stack_register(const std::string& reg, size_t register_size, size_t requested_size);

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

        // variable data type
        std::shared_ptr<DataType> data_type;

        // TODO: size_bytes can be inferred from data_type
    };
};