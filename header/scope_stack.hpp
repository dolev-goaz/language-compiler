#pragma once
#include <map>
#include <sstream>
#include <stack>
#include <string>

template <typename T>
class ScopeStack {
   public:
    void enterScope() { scope_stack.push(std::map<std::string, T>()); }
    void exitScope() {
        // throw error if no existing scope
        if (!scope_stack.empty()) {
            scope_stack.pop();
        }
    }
    void insert(const std::string& identifier, T variable_data) {
        // TODO: errors thrown here should be converted to generic Scope error
        if (scope_stack.empty()) {
            // std::stringstream err_stream;
            // err_stream << "Variable '" << identifier << "' cannot be declared outside of a scope.";
            // throw SemanticAnalyzerException(err_stream.str(), variable_data.start_token_meta);
            std::cerr << "Variable '" << identifier << "' cannot be declared outside of a scope." << std::endl;
            exit(EXIT_FAILURE);
        }

        scope& current_scope = scope_stack.top();
        if (current_scope.count(identifier) > 0) {
            // std::stringstream err_stream;
            // err_stream << "Variable '" << identifier << "' already exists in the current scope.";
            // throw SemanticAnalyzerException(err_stream.str(), variable_data.start_token_meta);
            std::cerr << "Variable '" << identifier << "' already exists in the current scope." << std::endl;
            exit(EXIT_FAILURE);
        }
        scope_stack.top()[identifier] = variable_data;
    }
    bool lookup(const std::string& identifier, T& variable_data) const {
        // throw error if no existing scope
        for (auto it = scope_stack.top().rbegin(); it != scope_stack.top().rend(); ++it) {
            if (it->first == identifier) {
                variable_data = it->second;
                return true;
            }
        }
        return false;
    }

   private:
    typedef std::map<std::string, T> scope;
    std::stack<scope> scope_stack;
};