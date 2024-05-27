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

DataType SemanticAnalyzer::analyze_expression_char_literal(ASTCharLiteral& ignored) {
    (void)ignored; // suppress unused
    return DataType::int_16; // TODO: new datatype for char
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