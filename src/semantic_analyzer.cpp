#include "../header/semantic_analyzer.hpp"

SymbolTable SemanticAnalyzer::analyze() { return m_symbol_table; }

void SymbolTable::enterScope() { scope_stack.push(std::map<std::string, int>()); }

void SymbolTable::exitScope() {
    // throw error if no existing scope
    if (!scope_stack.empty()) {
        scope_stack.pop();
    }
}

bool SymbolTable::insert(const std::string& identifier, int value) {
    // throw error if no existing scope
    if (!scope_stack.empty()) {
        scope_stack.top()[identifier] = value;
        return true;
    }
    return false;
}

bool SymbolTable::lookup(const std::string& identifier, int& value) const {
    // throw error if no existing scope
    for (auto it = scope_stack.top().rbegin(); it != scope_stack.top().rend(); ++it) {
        if (it->first == identifier) {
            value = it->second;
            return true;
        }
    }
    return false;
}
