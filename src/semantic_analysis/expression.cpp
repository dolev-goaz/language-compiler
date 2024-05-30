#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression(ASTExpression& expression) {
    return std::visit(SemanticAnalyzer::ExpressionVisitor{this}, expression.expression);
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_identifier(ASTIdentifier& identifier) {
    SymbolTable::Variable* literal_data = nullptr;
    if (!m_symbol_table.lookup(identifier.value, &literal_data)) {
        std::stringstream errorMessage;
        errorMessage << "Unknown Identifier '" << identifier.value << "'";
        throw SemanticAnalyzerException(errorMessage.str(), identifier.start_token_meta);
    }
    if (!literal_data->is_initialized) {
        std::stringstream errorMessage;
        errorMessage << "Access to uninitialized variable '" << identifier.value << "'";
        throw SemanticAnalyzerException(errorMessage.str(), identifier.start_token_meta);
    }
    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = literal_data->data_type,
        .is_literal = false,
    };
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_int_literal(ASTIntLiteral& ignored) {
    (void)ignored;  // suppress unused
    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = BasicType::makeBasicType(BasicDataType::INT64),
        .is_literal = true,
    };
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_char_literal(ASTCharLiteral& ignored) {
    (void)ignored;  // suppress unused
    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = BasicType::makeBasicType(BasicDataType::INT16),
        .is_literal = true,
    };
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_atomic(
    const std::shared_ptr<ASTAtomicExpression>& atomic) {
    return std::visit(SemanticAnalyzer::ExpressionVisitor{this}, atomic.get()->value);
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_unary(
    const std::shared_ptr<ASTUnaryExpression>& unary) {
    (void)unary;
    assert(false && "Didn't implement analysis for unary expressions");
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_binary(
    const std::shared_ptr<ASTBinExpression>& binExpr) {
    auto& lhs = *binExpr.get()->lhs.get();
    auto& rhs = *binExpr.get()->rhs.get();
    auto lhs_analysis = analyze_expression(lhs);
    auto rhs_analysis = analyze_expression(rhs);
    lhs.data_type = lhs_analysis.data_type;
    rhs.data_type = rhs_analysis.data_type;

    if (rhs.data_type != lhs.data_type) {
        bool show_warnings = !lhs_analysis.is_literal && !rhs_analysis.is_literal;
        assert_cast_expression(rhs, lhs.data_type, show_warnings);
    }

    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = lhs.data_type,
        .is_literal = (lhs_analysis.is_literal && rhs_analysis.is_literal),
    };
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_parenthesis(
    const ASTParenthesisExpression& paren_expr) {
    auto& inner_expression = *paren_expr.expression.get();
    return analyze_expression(inner_expression);
}