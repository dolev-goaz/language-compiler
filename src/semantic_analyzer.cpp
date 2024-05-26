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
        analyze_statement(*statement.get());
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
    return analyze_expression(inner_expression);
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

// ---------

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
    var_declare.get()->data_type = data_type;
    try {
        m_symbol_table.insert(var_declare.get()->name,
                              SymbolTable::Variable{.start_token_meta = start_token_meta, .data_type = data_type});
    } catch (const ScopeStackException& e) {
        throw SemanticAnalyzerException(e.what(), start_token_meta);
    }

    // initial 0 value if left uninitialized
    if (!var_declare.get()->value.has_value()) {
        ASTAtomicExpression zero_literal = ASTAtomicExpression{
            .start_token_meta = {0, 0},
            .value = ASTIntLiteral{.start_token_meta = {0, 0}, .value = "0"},
        };
        var_declare.get()->value = ASTExpression{.start_token_meta = {0, 0},
                                                 .data_type = DataType::int_64,
                                                 .expression = std::make_shared<ASTAtomicExpression>(zero_literal)};
    }

    auto& expression = var_declare.get()->value.value();
    DataType rhs_data_type = analyze_expression(expression);
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
    SymbolTable::Variable variableData;
    if (m_symbol_table.lookup(name, variableData)) {
        std::stringstream error;
        error << "Assignment of variable '" << name << "' which does not exist in current scope";
        throw SemanticAnalyzerException(error.str(), meta);
    }
    expression.data_type = analyze_expression(expression);
    if (expression.data_type != variableData.data_type) {
        if (!is_int_literal(expression)) {
            semantic_warning("Assignment operation of different data types. Data will be narrowed/widened.", meta);
        }
        expression.data_type = variableData.data_type;
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
