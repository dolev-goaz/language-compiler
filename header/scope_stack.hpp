#pragma once
#include <map>
#include <sstream>
#include <string>
#include <vector>

template <typename T>
class ScopeStack {
   public:
    typedef std::map<std::string, T> scope;
    void enterScope() { scope_stack.push_back(std::map<std::string, T>()); }
    std::optional<scope> exitScope() {
        if (!scope_stack.empty()) {
            auto topScope = std::move(scope_stack.back());
            scope_stack.pop_back();
            return topScope;
        }
        return std::nullopt;
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

        scope& current_scope = scope_stack.back();
        if (current_scope.count(identifier) > 0) {
            // std::stringstream err_stream;
            // err_stream << "Variable '" << identifier << "' already exists in the current scope.";
            // throw SemanticAnalyzerException(err_stream.str(), variable_data.start_token_meta);
            std::cerr << "Variable '" << identifier << "' already exists in the current scope." << std::endl;
            exit(EXIT_FAILURE);
        }
        current_scope[identifier] = variable_data;
    }
    bool lookup(const std::string& identifier, T& variable_data) const {
        // throw error if no existing scope
        for (auto scope_it = scope_stack.rbegin(); scope_it != scope_stack.rend(); ++scope_it) {
            scope currentScope = *scope_it;
            for (auto variable_it = currentScope.begin(); variable_it != currentScope.end(); ++variable_it) {
                if (variable_it->first == identifier) {
                    variable_data = variable_it->second;
                    return true;
                }
            }
        }
        return false;
    }

   private:
    std::vector<scope> scope_stack;
};