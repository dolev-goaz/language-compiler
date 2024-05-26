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
            m_symbol_table.insert(function_param.name, SymbolTable::Variable{
                                                           .start_token_meta = start_token_meta,
                                                           .data_type = function_param.data_type,
                                                       });
        } catch (const ScopeStackException& e) {
            throw SemanticAnalyzerException(e.what(), start_token_meta);
        }
    }
    analyze_statement(*func.statement.get());
    m_current_function_name = "";
    m_symbol_table.exitScope();
    auto& function_header = m_function_table.at(func.name);
    // TODO: handle void datatype
    // TODO: should handle all execution paths
    if (!function_header.found_return_statement) {
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
    auto& expression = return_statement.get()->expression;
    expression.data_type = analyze_expression(expression);
    if (expression.data_type != function_header.data_type) {
        if (!is_int_literal(expression)) {
            semantic_warning(
                "Return statement with different datatype of function return type. Data will be narrowed/widened.",
                meta);
        }
        expression.data_type = function_header.data_type;
    }
}
