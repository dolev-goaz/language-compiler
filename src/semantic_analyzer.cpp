#include "../header/semantic_analyzer.hpp"

#include "semantic_visitor.hpp"

std::map<std::string, DataType> datatype_mapping = {
    {"int_8", DataType::int_8},
    {"int_16", DataType::int_16},
    {"int_32", DataType::int_32},
    {"int_64", DataType::int_64},
};

// TODO: ensure type widening/narrowing works properly

bool SemanticAnalyzer::is_int_literal(ASTExpression& expression) {
    if (!std::holds_alternative<std::shared_ptr<ASTAtomicExpression>>(expression.expression)) {
        return false;
    }
    auto& atomic = *std::get<std::shared_ptr<ASTAtomicExpression>>(expression.expression).get();

    return std::holds_alternative<ASTIntLiteral>(atomic.value);
}

void SemanticAnalyzer::semantic_warning(const std::string& message, const TokenMeta& position) {
    std::cout << "SEMANTIC WARNING AT "
              << Globals::getInstance().getCurrentFilePosition(position.line_num, position.line_pos) << ": " << message
              << std::endl;
}

void SemanticAnalyzer::analyze() {
    this->m_symbol_table.enterScope();
    auto& statements = m_prog.statements;
    auto& functions = m_prog.functions;
    // first passage through functions- function header
    for (auto& function : functions) {
        analyze_function_header(*function.get());
    }
    // pass through global statements
    for (auto& statement : statements) {
        std::visit(SemanticAnalyzer::StatementVisitor{.analyzer = this}, statement.get()->statement);
    }
    // second passage through functions- function body
    for (auto& function : functions) {
        analyze_function_body(*function.get());
    }
    this->m_symbol_table.exitScope();
}

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
    return std::visit(SemanticAnalyzer::ExpressionVisitor{this}, inner_expression.expression);
}

DataType SemanticAnalyzer::analyze_expression_function_call(ASTFunctionCallExpression& function_call_expr) {
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
        provided_params[i].data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{this}, provided_params[i].expression);
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

void SemanticAnalyzer::analyze_scope(const std::vector<std::shared_ptr<ASTStatement>>& statements,
                                     std::string function_name) {
    this->m_symbol_table.enterScope();
    for (auto& statement : statements) {
        std::visit(SemanticAnalyzer::StatementVisitor{.analyzer = this, .function_name = function_name},
                   statement.get()->statement);
    }
    this->m_symbol_table.exitScope();
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
    std::visit(SemanticAnalyzer::StatementVisitor{.analyzer = this, .function_name = func.name},
               func.statement.get()->statement);
    m_symbol_table.exitScope();
    auto& function_header = m_function_table.at(func.name);
    // TODO: handle void datatype
    // TODO: should handle all execution paths
    if (!function_header.found_return_statement) {
        throw SemanticAnalyzerException("Return statement not found", function_header.start_token_meta);
    }
}
