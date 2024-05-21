#pragma once
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "error/scope_stack_error.hpp"

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
        if (scope_stack.empty()) {
            std::stringstream err_stream;
            err_stream << "Variable '" << identifier << "' cannot be declared outside of a scope.";
            throw ScopeStackException(err_stream.str());
        }

        scope& current_scope = scope_stack.back();
        if (current_scope.count(identifier) > 0) {
            std::stringstream err_stream;
            err_stream << "Variable '" << identifier << "' already exists in the current scope.";
            throw ScopeStackException(err_stream.str());
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
    bool is_variable_global(const std::string& identifier) {
        if (scope_stack.empty()) {
            return false;
        }
        scope firstScope = scope_stack.at(0);
        return (firstScope.count(identifier) > 0);
    }

   private:
    std::vector<scope> scope_stack;
};