#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "AST_node.hpp"

class SymbolTable {
   public:
    void enterScope();
    void exitScope();
    bool insert(const std::string& identifier, int value);
    bool lookup(const std::string& identifier, int& value) const;

   private:
    std::stack<std::map<std::string, int>> scope_stack;
};

class SemanticAnalyzer {
   public:
    SemanticAnalyzer(ASTProgram program) : m_prog(program) {}
    SymbolTable analyze();

   private:
    const ASTProgram m_prog;
    SymbolTable m_symbol_table;
};