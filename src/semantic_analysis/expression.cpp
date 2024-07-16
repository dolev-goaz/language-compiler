#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression(
    ASTExpression& expression, const std::shared_ptr<DataType>& lhs_datatype) {
    return std::visit(SemanticAnalyzer::ExpressionVisitor{.analyzer = this, .lhs_datatype = lhs_datatype},
                      expression.expression);
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_lhs(ASTExpression& expression,
                                                                                    bool is_initializing) {
    if (std::holds_alternative<std::shared_ptr<ASTArrayIndexExpression>>(expression.expression)) {
        // TODO: idk what to do here for now

        // auto array_indexing = std::get<std::shared_ptr<ASTArrayIndexExpression>>(expression.expression);
        // auto array_type = dynamic_cast<ArrayType*>(array_indexing->expression->data_type.get());
        // auto inner_type_size_bytes = array_type->elementType->get_size_bytes();
        return analyze_expression(expression);
    }

    // must be atomic expression
    if (!std::holds_alternative<std::shared_ptr<ASTAtomicExpression>>(expression.expression)) {
        throw SemanticAnalyzerException("Unexpected lhs expression", expression.start_token_meta);
    }
    auto atomic = std::get<std::shared_ptr<ASTAtomicExpression>>(expression.expression);
    if (std::holds_alternative<ASTIdentifier>(atomic->value)) {
        auto identifier = std::get<ASTIdentifier>(atomic->value);
        auto name = identifier.value;
        SymbolTable::Variable* variableData = nullptr;
        if (!m_symbol_table.lookup(name, &variableData)) {
            std::stringstream error;
            error << "LHS variable does not exist in current scope- " << name;
            throw SemanticAnalyzerException(error.str(), expression.start_token_meta);
        }
        variableData->is_initialized = is_initializing;
        return analyze_expression(expression);
    }

    throw SemanticAnalyzerException("Didn't implement the provided LHS expression", expression.start_token_meta);
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_identifier(ASTIdentifier& identifier) {
    SymbolTable::Variable* literal_data = nullptr;
    if (!m_symbol_table.lookup(identifier.value, &literal_data)) {
        std::stringstream errorMessage;
        errorMessage << "Unknown Identifier '" << identifier.value << "'";
        throw SemanticAnalyzerException(errorMessage.str(), identifier.start_token_meta);
    }
    auto is_array = (bool)(dynamic_cast<ArrayType*>(literal_data->data_type.get()));
    if (!literal_data->is_initialized && !is_array) {
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

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_array_initializer(
    ASTArrayInitializer& initializer, const std::shared_ptr<DataType>& lhs_datatype) {
    auto& values = initializer.initialize_values;
    if (values.size() == 0) {
        throw SemanticAnalyzerException("Array initializer with no members", initializer.start_token_meta);
    }
    auto array_type = dynamic_cast<ArrayType*>(lhs_datatype.get());
    if (!array_type) {
        throw SemanticAnalyzerException("Unexpected datatype for array initializer", initializer.start_token_meta);
    }
    if (array_type->size == 0) {
        array_type->size = values.size();
    }
    if (values.size() != array_type->size) {
        std::stringstream err;
        err << "Expected initializer of size " << array_type->size << ". Instead got " << values.size() << ".";
        throw SemanticAnalyzerException(err.str(), initializer.start_token_meta);
    }
    auto expected_inner_type = array_type->elementType;

    for (auto& value : values) {
        auto analysis_result = analyze_expression(value, expected_inner_type);
        value.data_type = analysis_result.data_type;
        value.is_literal = analysis_result.is_literal;

        assert_cast_expression(value, expected_inner_type, !analysis_result.is_literal);
    }

    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = lhs_datatype,
        .is_literal = false,
    };
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_atomic(
    const std::shared_ptr<ASTAtomicExpression>& atomic, const std::shared_ptr<DataType>& lhs_datatype) {
    return std::visit(SemanticAnalyzer::ExpressionVisitor{this, lhs_datatype}, atomic->value);
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_unary(
    const std::shared_ptr<ASTUnaryExpression>& unary) {
    static_assert((int)UnaryOperation::operationCount - 1 == 3,
                  "Implemented unary operations without updating semantic analysis");
    auto& operand = unary->expression;
    auto analysis_result = analyze_expression(*operand);
    operand->data_type = analysis_result.data_type;
    operand->is_literal = analysis_result.is_literal;
    std::shared_ptr<DataType> inner_type;
    switch (unary->operation) {
        case UnaryOperation::negate:
            return SemanticAnalyzer::ExpressionAnalysisResult{
                .data_type = operand->data_type,
                .is_literal = operand->is_literal,
            };

        case UnaryOperation::dereference:
            return SemanticAnalyzer::ExpressionAnalysisResult{
                .data_type = std::make_shared<PointerType>(operand->data_type),
                .is_literal = false,
            };
        case UnaryOperation::reference:
            inner_type = DataTypeUtils::get_inner_type(operand->data_type);
            if (!inner_type) {
                std::stringstream error;
                error << "Can't reference type '" << operand->data_type->toString() << "'!";
                throw SemanticAnalyzerException(error.str(), unary->start_token_meta);
            }
            return SemanticAnalyzer::ExpressionAnalysisResult{
                .data_type = inner_type,
                .is_literal = false,
            };
        default:
            throw SemanticAnalyzerException("Unknown unary operation", unary->start_token_meta);
    }
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_binary(
    const std::shared_ptr<ASTBinExpression>& binExpr) {
    auto& lhs = binExpr->lhs;
    auto& rhs = binExpr->rhs;
    auto lhs_analysis = analyze_expression(*lhs);
    auto rhs_analysis = analyze_expression(*rhs);
    lhs->data_type = lhs_analysis.data_type;
    rhs->data_type = rhs_analysis.data_type;

    if (rhs->data_type != lhs->data_type) {
        bool show_warnings = !lhs_analysis.is_literal && !rhs_analysis.is_literal;
        assert_cast_expression(*rhs, lhs->data_type, show_warnings);
    }

    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = lhs->data_type,
        .is_literal = (lhs_analysis.is_literal && rhs_analysis.is_literal),
    };
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_parenthesis(
    const ASTParenthesisExpression& paren_expr) {
    return analyze_expression(*paren_expr.expression);
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_expression_array_indexing(
    const std::shared_ptr<ASTArrayIndexExpression>& arr_index_expr) {
    auto& operand = arr_index_expr->expression;
    auto& index = arr_index_expr->index;

    auto operand_analysis = analyze_expression(*operand);
    operand->data_type = operand_analysis.data_type;
    operand->is_literal = operand_analysis.is_literal;

    auto array_type = dynamic_cast<ArrayType*>(operand->data_type.get());
    auto pointer_type = dynamic_cast<PointerType*>(operand->data_type.get());
    if (!array_type && !pointer_type) {
        throw SemanticAnalyzerException("Array indexing on non-array type", arr_index_expr->start_token_meta);
    }
    auto index_analysis = analyze_expression(*index);
    index->data_type = index_analysis.data_type;
    index->is_literal = index_analysis.is_literal;

    auto regular_index_type = BasicType::makeBasicType(BasicDataType::INT64);
    auto compatibility = index->data_type->is_compatible(*regular_index_type);
    if (compatibility == CompatibilityStatus::NotCompatible) {
        throw SemanticAnalyzerException("Array index must be numeric", index->start_token_meta);
    }

    auto element_type = array_type ? array_type->elementType : pointer_type->baseType;

    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = element_type,
        .is_literal = false,
    };
}

bool SemanticAnalyzer::is_array_initializer(const ASTExpression& expr) {
    if (!std::holds_alternative<std::shared_ptr<ASTAtomicExpression>>(expr.expression)) {
        return false;
    }
    auto atomic = std::get<std::shared_ptr<ASTAtomicExpression>>(expr.expression);
    return std::holds_alternative<ASTArrayInitializer>(atomic->value);
}
