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
    SemanticAnalyzer(ASTProgram program) : m_prog(program) {}
    void analyze();

   private:
    void analyze_function_header(ASTStatementFunction& func);
    void analyze_function_body(ASTStatementFunction& func);
    void analyze_scope(const std::vector<std::shared_ptr<ASTStatement>>& statements, std::string function_name = "");
    void analyze_function_param(ASTFunctionParam& param);

    ASTProgram m_prog;
    SymbolTable::SemanticScopeStack m_symbol_table;
    SymbolTable::SemanticFunctionTable m_function_table;
    struct ExpressionVisitor;
    struct StatementVisitor;
};