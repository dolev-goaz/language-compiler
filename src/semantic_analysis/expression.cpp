#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

DataType SemanticAnalyzer::analyze_expression(ASTExpression& expression) {
    return std::visit(SemanticAnalyzer::ExpressionVisitor{this}, expression.expression);
}

DataType SemanticAnalyzer::analyze_expression_identifier(ASTIdentifier& identifier) {
    SymbolTable::Variable literal_data;
    if (!m_symbol_table.lookup(identifier.value, literal_data)) {
        std::stringstream errorMessage;
        errorMessage << "Unknown Identifier '" << identifier.value << "'";
        throw SemanticAnalyzerException(errorMessage.str(), identifier.start_token_meta);
    }
    return literal_data.data_type;
}

DataType SemanticAnalyzer::analyze_expression_int_literal(ASTIntLiteral& ignored) {
    (void)ignored;  // suppress unused
    return DataType::int_64;
}

DataType SemanticAnalyzer::analyze_expression_atomic(const std::shared_ptr<ASTAtomicExpression>& atomic) {
    return std::visit(SemanticAnalyzer::ExpressionVisitor{this}, atomic.get()->value);
}

DataType SemanticAnalyzer::analyze_expression_binary(const std::shared_ptr<ASTBinExpression>& binExpr) {
    // IN THE FUTURE: when there are more types, check for type compatibility
    auto& lhs = *binExpr.get()->lhs.get();
    auto& rhs = *binExpr.get()->rhs.get();
    DataType lhs_data_type = analyze_expression(lhs);
    DataType rhs_data_type = analyze_expression(rhs);
    // if rhs or lhs are int literals, no need for data narrowing warnings
    if ((lhs_data_type != rhs_data_type) && (!is_int_literal(lhs) && !is_int_literal(rhs))) {
        auto& meta = binExpr.get()->start_token_meta;
        semantic_warning("Binary operation of different data types. Data will be narrowed.", meta);
    }

    // type narrowing
    rhs.data_type = lhs_data_type;
    lhs.data_type = lhs_data_type;
    return lhs_data_type;
}

DataType SemanticAnalyzer::analyze_expression_parenthesis(const ASTParenthesisExpression& paren_expr) {
    auto& inner_expression = *paren_expr.expression.get();
    return analyze_expression(inner_expression);
}

DataType SemanticAnalyzer::analyze_expression_function_call(ASTFunctionCall& function_call_expr) {
    auto& func_name = function_call_expr.function_name;
    auto& start_token_meta = function_call_expr.start_token_meta;
    if (m_function_table.count(func_name) == 0) {
        std::stringstream error;
        error << "Unknown function " << func_name << ".";
        throw SemanticAnalyzerException(error.str(), start_token_meta);
    }
    auto& function_header_data = m_function_table.at(function_call_expr.function_name);
    auto& function_expected_params = function_header_data.parameters;
    std::vector<ASTExpression>& provided_params = function_call_expr.parameters;
    if (provided_params.size() != function_expected_params.size()) {
        std::stringstream error;
        error << "Function " << func_name << " expected " << function_expected_params.size()
              << " parameters, instead got " << provided_params.size() << ".";
        throw SemanticAnalyzerException(error.str(), start_token_meta);
    }
    for (size_t i = 0; i < provided_params.size(); ++i) {
        provided_params[i].data_type = analyze_expression(provided_params[i]);
        if (provided_params[i].data_type == function_expected_params[i].data_type) {
            // no type issues
            continue;
        }

        if (!is_int_literal(provided_params[i])) {
            auto& meta = provided_params[i].start_token_meta;
            semantic_warning("Provided parameter of different data type. Data will be narrowed/widened.", meta);
        }
        provided_params[i].data_type = function_expected_params[i].data_type;
    }
    function_call_expr.return_data_type = function_header_data.data_type;
    return function_header_data.data_type;
}
