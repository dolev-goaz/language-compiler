#pragma once

#include <string>
#include <vector>

struct statement_scope {
    size_t consume_count;
    std::string error_message;
};

class ParseStatementStack {
   public:
    void enterScope() { scope_stack.push_back(statement_scope{}); }
    bool exitScope() {
        if (scope_stack.empty()) {
            return false;
        }
        auto topScope = std::move(scope_stack.back());
        scope_stack.pop_back();
        return true;
    }

    void set_error_msg(const std::string& message) { top_scope().error_message = message; }
    std::string get_error_msg() { return top_scope().error_message; }
    size_t get_consume_count() { return top_scope().consume_count; }

    void finalize_consumption() { top_scope().consume_count = 0; }

    void undo_consumption() { undo_consumption(top_scope().consume_count); }

    void undo_consumption(size_t count) { top_scope().consume_count -= count; }

    void consume(size_t count) { top_scope().consume_count += count; }

   private:
    std::vector<statement_scope> scope_stack;

    statement_scope& top_scope() {
        if (scope_stack.empty()) {
            assert(false && "Should never reach here");
        }
        return scope_stack.back();
    }
};

class ParseStatementStackHandler {
   public:
    ParseStatementStackHandler(ParseStatementStack* scope) : m_stack(scope) { m_stack->enterScope(); }
    ~ParseStatementStackHandler() { m_stack->exitScope(); }

   private:
    ParseStatementStack* m_stack;
};