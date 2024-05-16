#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "./error/sem_analyze_error.hpp"
#include "AST_node.hpp"

extern std::map<std::string, DataType> datatype_mapping;

class SymbolTable {
   public:
    struct Variable {
        TokenMeta start_token_meta;
        DataType data_type;
    };
    void enterScope();
    void exitScope();
    void insert(const std::string& identifier, Variable variable_data);
    bool lookup(const std::string& identifier, Variable& variable_data) const;

   private:
    typedef std::map<std::string, SymbolTable::Variable> scope;
    std::stack<scope> scope_stack;
};

class SemanticAnalyzer {
   public:
    SemanticAnalyzer(ASTProgram program) : m_prog(program) {}
    void analyze();

   private:
    ASTProgram m_prog;
    SymbolTable m_symbol_table;
    struct ExpressionVisitor;
    struct StatementVisitor;
};