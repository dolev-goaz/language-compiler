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
using SemanticScopeStack = ScopeStack<Variable>;
};  // namespace SymbolTable

class SemanticAnalyzer {
   public:
    SemanticAnalyzer(ASTProgram program) : m_prog(program) {}
    void analyze();

   private:
    void analyze_function_header(ASTStatementFunction& func);
    void analyze_function_body(ASTStatementFunction& func);
    void analyze_scope(const std::vector<std::shared_ptr<ASTStatement>>& statements);
    void analyze_function_param(ASTFunctionParam& param);

    ASTProgram m_prog;
    SymbolTable::SemanticScopeStack m_symbol_table;
    std::map<std::string, std::vector<ASTFunctionParam>> m_function_table;
    struct ExpressionVisitor;
    struct StatementVisitor;
};