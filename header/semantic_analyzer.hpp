#pragma once
#include <assert.h>

#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "./error/sem_analyze_error.hpp"
#include "AST_node.hpp"
#include "globals.hpp"
#include "scope_stack.hpp"

extern std::map<std::string, DataType> datatype_mapping;

namespace SymbolTable {
struct Variable {
    TokenMeta start_token_meta;
    DataType data_type;
};
struct FunctionHeader {
    TokenMeta start_token_meta;
    DataType data_type;
    bool found_return_statement;
    std::vector<ASTFunctionParam> parameters;
};
using SemanticScopeStack = ScopeStack<Variable>;
using SemanticFunctionTable = std::map<std::string, SymbolTable::FunctionHeader>;
};  // namespace SymbolTable

class SemanticAnalyzer {
   public:
    SemanticAnalyzer(ASTProgram program) : m_prog(program), m_current_function_name("") {}
    void analyze();

   private:
    void analyze_function_header(ASTStatementFunction& func);
    void analyze_function_body(ASTStatementFunction& func);
    void analyze_function_param(ASTFunctionParam& param);

    DataType analyze_expression(ASTExpression& expression);

    DataType analyze_expression_identifier(ASTIdentifier& identifier);
    DataType analyze_expression_int_literal(ASTIntLiteral& ignored);
    DataType analyze_expression_atomic(const std::shared_ptr<ASTAtomicExpression>& atomic);
    DataType analyze_expression_binary(const std::shared_ptr<ASTBinExpression>& binExpr);
    DataType analyze_expression_parenthesis(const ASTParenthesisExpression& paren_expr);
    DataType analyze_expression_function_call(ASTFunctionCall& function_call_expr);

    void analyze_statement(ASTStatement& statement);

    void analyze_statement_exit(const std::shared_ptr<ASTStatementExit>& exit);
    void analyze_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_declare);
    void analyze_statement_var_assign(const std::shared_ptr<ASTStatementAssign>& var_assign);
    void analyze_statement_scope(const std::shared_ptr<ASTStatementScope>& scope);
    void analyze_statement_if(const std::shared_ptr<ASTStatementIf>& _if);
    void analyze_statement_while(const std::shared_ptr<ASTStatementWhile>& while_statement);
    void analyze_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement);
    void analyze_statement_return(const std::shared_ptr<ASTStatementReturn>& return_statement);

    static void semantic_warning(const std::string& message, const TokenMeta& position);

    bool is_int_literal(ASTExpression& expression);

    ASTProgram m_prog;
    SymbolTable::SemanticScopeStack m_symbol_table;
    SymbolTable::SemanticFunctionTable m_function_table;
    std::string m_current_function_name;
    struct ExpressionVisitor;
    struct StatementVisitor;
};