#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

void SemanticAnalyzer::analyze_statement(ASTStatement& statement) {
    return std::visit(SemanticAnalyzer::StatementVisitor{this}, statement.statement);
}

void SemanticAnalyzer::analyze_statement_exit(const std::shared_ptr<ASTStatementExit>& exit) {
    auto& expression = exit.get()->status_code;
    expression.data_type = analyze_expression(expression).data_type;
}

void SemanticAnalyzer::analyze_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_declare) {
    auto& start_token_meta = var_declare.get()->start_token_meta;
    std::vector<Token> type_tokens;
    type_tokens.insert(type_tokens.end(), var_declare->data_type_tokens.begin(), var_declare->data_type_tokens.end());
    type_tokens.insert(type_tokens.end(), var_declare->array_modifiers.begin(), var_declare->array_modifiers.end());
    
    std::shared_ptr<DataType> data_type = create_data_type(type_tokens);
    if (data_type->isVoid()) {
        throw SemanticAnalyzerException("Variables can not be of void type", start_token_meta);
    }
    bool is_initialized = var_declare.get()->value.has_value();
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
    auto rhs_analysis = analyze_expression(expression);
    expression.data_type = rhs_analysis.data_type;
    if (expression.data_type->isVoid()) {
        throw SemanticAnalyzerException("Can not assign 'void' to variables", start_token_meta);
    }
    if (expression.data_type != data_type) {
        assert_cast_expression(expression, data_type, !rhs_analysis.is_literal);
    }
}

void SemanticAnalyzer::analyze_statement_var_assign(const std::shared_ptr<ASTStatementAssign>& var_assign) {
    auto& meta = var_assign.get()->start_token_meta;
    auto& name = var_assign.get()->name;
    auto& expression = var_assign.get()->value;
    SymbolTable::Variable* variableData = nullptr;
    if (!m_symbol_table.lookup(name, &variableData)) {
        std::stringstream error;
        error << "Assignment of variable '" << name << "' which does not exist in current scope";
        throw SemanticAnalyzerException(error.str(), meta);
    }
    auto rhs_analysis = analyze_expression(expression);
    expression.data_type = rhs_analysis.data_type;
    variableData->is_initialized = true;
    if (expression.data_type->isVoid()) {
        throw SemanticAnalyzerException("Can't assign 'void' to variables", meta);
    }
    if (expression.data_type != variableData->data_type) {
        assert_cast_expression(expression, variableData->data_type, !rhs_analysis.is_literal);
    }
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
    expression.data_type = analyze_expression(expression).data_type;
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
    expression.data_type = analyze_expression(expression).data_type;
    analyze_statement(success_statement);
}

void SemanticAnalyzer::analyze_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement) {
    (void)function_statement;  // ignore unused
    assert(false && "Should never reach here. Function statements are to be parsed seperately");
}
