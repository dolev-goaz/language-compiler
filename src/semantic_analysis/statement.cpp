#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

void SemanticAnalyzer::analyze_statement(ASTStatement& statement) {
    return std::visit(SemanticAnalyzer::StatementVisitor{this}, statement.statement);
}

void SemanticAnalyzer::analyze_statement_exit(const std::shared_ptr<ASTStatementExit>& exit) {
    auto& expression = exit->status_code;
    auto analysis_result = analyze_expression(expression);
    expression.data_type = analysis_result.data_type;
    expression.is_literal = analysis_result.is_literal;
    auto expected_data_type = BasicType::makeBasicType(BasicDataType::INT16);
    if (expression.data_type != expected_data_type) {
        assert_cast_expression(expression, expected_data_type, !expression.is_literal);
    }
}

void SemanticAnalyzer::analyze_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_declare) {
    auto& start_token_meta = var_declare->start_token_meta;
    std::shared_ptr<DataType> data_type = create_data_type(var_declare->data_type_tokens);
    if (data_type->is_void()) {
        throw SemanticAnalyzerException("Variables can not be of void type", start_token_meta);
    }
    bool is_initialized = var_declare->value.has_value();
    var_declare->data_type = data_type;
    try {
        m_symbol_table.insert(var_declare->name, SymbolTable::Variable{
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

    auto& expression = var_declare->value.value();
    auto rhs_analysis = analyze_expression(expression, data_type);
    expression.data_type = rhs_analysis.data_type;
    if (expression.data_type->is_void()) {
        throw SemanticAnalyzerException("Can not assign 'void' to variables", start_token_meta);
    }
    if (expression.data_type != data_type) {
        assert_cast_expression(expression, data_type, !rhs_analysis.is_literal);
    }
}

void SemanticAnalyzer::analyze_statement_var_assign(const std::shared_ptr<ASTStatementAssign>& var_assign) {
    auto& meta = var_assign->start_token_meta;
    auto& lhs = var_assign->lhs;
    auto& rhs = var_assign->value;

    auto lhs_analysis = analyze_expression_lhs(*lhs, true);
    lhs->data_type = lhs_analysis.data_type;

    auto rhs_analysis = analyze_expression(rhs, lhs->data_type);
    rhs.data_type = rhs_analysis.data_type;
    rhs.is_literal = rhs_analysis.is_literal;
    if (rhs.data_type->is_void()) {
        throw SemanticAnalyzerException("Can't assign 'void' to variables", meta);
    }
    if (rhs.data_type != lhs->data_type) {
        assert_cast_expression(rhs, lhs->data_type, !rhs_analysis.is_literal);
    }
}

void SemanticAnalyzer::analyze_statement_scope(const std::shared_ptr<ASTStatementScope>& scope) {
    this->m_symbol_table.enterScope();
    for (auto& statement : scope->statements) {
        analyze_statement(*statement);
    }
    this->m_symbol_table.exitScope();
}

void SemanticAnalyzer::analyze_statement_if(const std::shared_ptr<ASTStatementIf>& _if) {
    auto& expression = _if->expression;
    auto& success_statement = *_if->success_statement;
    expression.data_type = analyze_expression(expression).data_type;
    analyze_statement(success_statement);
    if (_if->fail_statement != nullptr) {
        auto& fail_statement = *_if->fail_statement;
        analyze_statement(fail_statement);
    }
}

void SemanticAnalyzer::analyze_statement_while(const std::shared_ptr<ASTStatementWhile>& while_statement) {
    // NOTE: identical to analysis of if statements
    auto& expression = while_statement->expression;
    auto& success_statement = *while_statement->success_statement;
    expression.data_type = analyze_expression(expression).data_type;
    analyze_statement(success_statement);
}

void SemanticAnalyzer::analyze_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement) {
    (void)function_statement;  // ignore unused
    assert(false && "Should never reach here. Function statements are to be parsed seperately");
}
