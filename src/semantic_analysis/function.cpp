#include "../header/semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

void SemanticAnalyzer::analyze_function_param(ASTFunctionParam& param) {
    auto& start_token_meta = param.start_token_meta;
    if (datatype_mapping.count(param.data_type_str) == 0) {
        std::stringstream errorMessage;
        errorMessage << "Unknown data type '" << param.data_type_str << "'";
        throw SemanticAnalyzerException(errorMessage.str(), start_token_meta);
    }
    DataType data_type = datatype_mapping.at(param.data_type_str);
    if (data_type == DataType::_void) {
        throw SemanticAnalyzerException("Function parameter can not be of type void", start_token_meta);
    }
    param.data_type = data_type;
}

void SemanticAnalyzer::analyze_function_header(ASTStatementFunction& func) {
    if (datatype_mapping.count(func.return_data_type_str) == 0) {
        std::stringstream errorMessage;
        errorMessage << "Unknown data type '" << func.return_data_type_str << "'";
        throw SemanticAnalyzerException(errorMessage.str(), func.start_token_meta);
    }
    for (auto& function_param : func.parameters) {
        // set params datatype
        analyze_function_param(function_param);
    }
    func.return_data_type = datatype_mapping.at(func.return_data_type_str);
    SymbolTable::FunctionHeader function_header = {
        .start_token_meta = func.start_token_meta,
        .data_type = func.return_data_type,
        .found_return_statement = false,
        .parameters = func.parameters,
    };
    m_function_table.emplace(func.name, function_header);
}

void SemanticAnalyzer::analyze_function_body(ASTStatementFunction& func) {
    m_symbol_table.enterScope();
    m_current_function_name = func.name;
    for (auto& function_param : func.parameters) {
        auto& start_token_meta = function_param.start_token_meta;
        try {
            m_symbol_table.insert(function_param.name,
                                  SymbolTable::Variable{
                                      .start_token_meta = start_token_meta,
                                      .data_type = function_param.data_type,
                                      .is_initialized = true,  // parameters are initialized from caller
                                  });
        } catch (const ScopeStackException& e) {
            throw SemanticAnalyzerException(e.what(), start_token_meta);
        }
    }
    analyze_statement(*func.statement.get());
    m_current_function_name = "";
    m_symbol_table.exitScope();
    auto& function_header = m_function_table.at(func.name);
    // TODO: should handle all execution paths
    if (!function_header.found_return_statement && function_header.data_type != DataType::_void) {
        throw SemanticAnalyzerException("Return statement not found", function_header.start_token_meta);
    }
}

void SemanticAnalyzer::analyze_statement_return(const std::shared_ptr<ASTStatementReturn>& return_statement) {
    if (m_current_function_name.empty()) {
        throw SemanticAnalyzerException("Can't use 'return' outside of a function",
                                        return_statement.get()->start_token_meta);
    }
    auto& meta = return_statement.get()->start_token_meta;
    auto& function_header = m_function_table.at(m_current_function_name);
    function_header.found_return_statement = true;
    auto& possible_expression = return_statement.get()->expression;
    if (!possible_expression.has_value()) {
        if (function_header.data_type != DataType::_void) {
            throw SemanticAnalyzerException("Return statement of function with return type must include an expression",
                                            meta);
        }
        return;
    }
    auto& expression = possible_expression.value();
    auto analysis_result = analyze_expression(expression);
    expression.data_type = analysis_result.data_type;
    if (function_header.data_type == DataType::_void && expression.data_type != DataType::_void) {
        throw SemanticAnalyzerException("Can not return non-void expressions from void methods", meta);
    }
    if (expression.data_type != function_header.data_type) {
        assert_cast_expression(expression, function_header.data_type, !analysis_result.is_literal);
    }
}

SemanticAnalyzer::ExpressionAnalysisResult SemanticAnalyzer::analyze_function_call(
    ASTFunctionCall& function_call_expr) {
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
        auto analysis_result = analyze_expression(provided_params[i]);
        provided_params[i].data_type = analysis_result.data_type;
        auto expected_data_type = function_expected_params[i].data_type;

        if (provided_params[i].data_type != expected_data_type) {
            assert_cast_expression(provided_params[i], expected_data_type, !analysis_result.is_literal);
        }
    }
    function_call_expr.return_data_type = function_header_data.data_type;
    return SemanticAnalyzer::ExpressionAnalysisResult{
        .data_type = function_header_data.data_type,
        .is_literal = false,
    };
}
