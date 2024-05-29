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
    bool is_initialized;
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
    struct ExpressionVisitor;
    struct StatementVisitor;
    struct ExpressionAnalysisResult;

    void analyze_function_header(ASTStatementFunction& func);
    void analyze_function_body(ASTStatementFunction& func);
    void analyze_function_param(ASTFunctionParam& param);

    ExpressionAnalysisResult analyze_expression(ASTExpression& expression);

    ExpressionAnalysisResult analyze_expression_identifier(ASTIdentifier& identifier);
    ExpressionAnalysisResult analyze_expression_int_literal(ASTIntLiteral& int_literal);
    ExpressionAnalysisResult analyze_expression_char_literal(ASTCharLiteral& char_literal);
    ExpressionAnalysisResult analyze_expression_atomic(const std::shared_ptr<ASTAtomicExpression>& atomic);
    ExpressionAnalysisResult analyze_expression_binary(const std::shared_ptr<ASTBinExpression>& binExpr);
    ExpressionAnalysisResult analyze_expression_parenthesis(const ASTParenthesisExpression& paren_expr);
    ExpressionAnalysisResult analyze_function_call(ASTFunctionCall& function_call_expr);

    void analyze_statement(ASTStatement& statement);

    void analyze_statement_exit(const std::shared_ptr<ASTStatementExit>& exit);
    void analyze_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_declare);
    void analyze_statement_var_assign(const std::shared_ptr<ASTStatementAssign>& var_assign);
    void analyze_statement_scope(const std::shared_ptr<ASTStatementScope>& scope);
    void analyze_statement_if(const std::shared_ptr<ASTStatementIf>& _if);
    void analyze_statement_while(const std::shared_ptr<ASTStatementWhile>& while_statement);
    void analyze_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement);
    void analyze_statement_return(const std::shared_ptr<ASTStatementReturn>& return_statement);

    static void assert_cast_expression(ASTExpression& expression, DataType data_type, bool show_warning);

    static void semantic_warning(const std::string& message, const TokenMeta& position);

    ASTProgram m_prog;
    SymbolTable::SemanticScopeStack m_symbol_table;
    SymbolTable::SemanticFunctionTable m_function_table;
    std::string m_current_function_name;
};