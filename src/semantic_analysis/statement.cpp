#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

void SemanticAnalyzer::analyze_statement(ASTStatement& statement) {
    return std::visit(SemanticAnalyzer::StatementVisitor{this}, statement.statement);
}

void SemanticAnalyzer::analyze_statement_exit(const std::shared_ptr<ASTStatementExit>& exit) {
    auto& expression = exit.get()->status_code;
    expression.data_type = analyze_expression(expression);
}

void SemanticAnalyzer::analyze_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_declare) {
    auto& start_token_meta = var_declare.get()->start_token_meta;
    if (datatype_mapping.count(var_declare.get()->data_type_str) == 0) {
        std::stringstream errorMessage;
        errorMessage << "Unknown data type '" << var_declare.get()->data_type_str << "'";
        throw SemanticAnalyzerException(errorMessage.str(), start_token_meta);
    }
    DataType data_type = datatype_mapping.at(var_declare.get()->data_type_str);
    bool is_initialized = var_declare.get()->value.has_value();
    if (data_type == DataType::_void) {
        throw SemanticAnalyzerException("Variables can not be of void type", start_token_meta);
    }
    var_declare.get()->data_type = data_type;
    try {
        m_symbol_table.insert(var_declare.get()->name, SymbolTable::Variable{
                                                           .start_token_meta = start_token_meta,
                                                           .data_type = data_type,
                                                           .is_initialized = is_initialized,
                                                       });
    } catch (const ScopeStackException& e) {
        throw SemanticAnalyzerException(e.what(), start_token_meta);
    }

    if (!is_initialized) {
        return;
    }

    auto& expression = var_declare.get()->value.value();
    DataType rhs_data_type = analyze_expression(expression);
    if (rhs_data_type == DataType::_void) {
        throw SemanticAnalyzerException("Can not assign variables to 'void'", start_token_meta);
    }
    // IN THE FUTURE: when there are more types, check for type compatibility
    if (rhs_data_type != data_type) {
        // type narrowing/widening
        if (!is_int_literal(expression)) {
            semantic_warning("Assignment operation of different data types. Data will be narrowed/widened.",
                             start_token_meta);
        }
        // if rhs is int literal we can just cast it, no need to warn about it
        rhs_data_type = data_type;
    }
    expression.data_type = rhs_data_type;
}

void SemanticAnalyzer::analyze_statement_var_assign(const std::shared_ptr<ASTStatementAssign>& var_assign) {
    auto& meta = var_assign.get()->start_token_meta;
    auto& name = var_assign.get()->name;
    auto& expression = var_assign.get()->value;
    SymbolTable::Variable* variableData = nullptr;
    if (m_symbol_table.lookup(name, variableData)) {
        std::stringstream error;
        error << "Assignment of variable '" << name << "' which does not exist in current scope";
        throw SemanticAnalyzerException(error.str(), meta);
    }
    expression.data_type = analyze_expression(expression);
    if (expression.data_type == DataType::_void) {
        throw SemanticAnalyzerException("Can't assign variables to void value", meta);
    }
    if (expression.data_type != variableData->data_type) {
        if (!is_int_literal(expression)) {
            semantic_warning("Assignment operation of different data types. Data will be narrowed/widened.", meta);
        }
        expression.data_type = variableData->data_type;
    }

    variableData->is_initialized = true;
}

void SemanticAnalyzer::analyze_statement_scope(const std::shared_ptr<ASTStatementScope>& scope) {
    this->m_symbol_table.enterScope();
    for (auto& statement : scope.get()->statements) {
        analyze_statement(*statement.get());
    }
    this->m_symbol_table.exitScope();
}

void SemanticAnalyzer::analyze_statement_if(const std::shared_ptr<ASTStatementIf>& _if) {
    auto& expression = _if.get()->expression;
    auto& success_statement = *_if.get()->success_statement.get();
    expression.data_type = analyze_expression(expression);
    analyze_statement(success_statement);
    if (_if.get()->fail_statement != nullptr) {
        auto& fail_statement = *_if.get()->fail_statement.get();
        analyze_statement(fail_statement);
    }
}

void SemanticAnalyzer::analyze_statement_while(const std::shared_ptr<ASTStatementWhile>& while_statement) {
    // NOTE: identical to analysis of if statements
    auto& expression = while_statement.get()->expression;
    auto& success_statement = *while_statement.get()->success_statement.get();
    expression.data_type = analyze_expression(expression);
    analyze_statement(success_statement);
}

void SemanticAnalyzer::analyze_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement) {
    (void)function_statement;  // ignore unused
    assert(false && "Should never reach here. Function statements are to be parsed seperately");
}
