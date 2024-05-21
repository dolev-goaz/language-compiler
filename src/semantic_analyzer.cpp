#include "../header/semantic_analyzer.hpp"

std::map<std::string, DataType> datatype_mapping = {
    {"int_8", DataType::int_8},
    {"int_16", DataType::int_16},
    {"int_32", DataType::int_32},
    {"int_64", DataType::int_64},
};

// TODO: ensure type widening/narrowing works properly

struct SemanticAnalyzer::ExpressionVisitor {
    SymbolTable::SemanticScopeStack& symbol_table;
    SymbolTable::SemanticFunctionTable& function_table;
    DataType operator()(ASTIdentifier& identifier) const {
        SymbolTable::Variable literal_data;
        if (!symbol_table.lookup(identifier.value, literal_data)) {
            std::stringstream errorMessage;
            errorMessage << "Unknown Identifier '" << identifier.value << "'";
            throw SemanticAnalyzerException(errorMessage.str(), identifier.start_token_meta);
        }
        return literal_data.data_type;
    }

    DataType operator()(ASTIntLiteral& ignored) const {
        (void)ignored;  // suppress unused
        return DataType::int_64;
    }

    DataType operator()(const std::shared_ptr<ASTAtomicExpression>& atomic) const {
        return std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table, function_table}, atomic.get()->value);
    }

    DataType operator()(const std::shared_ptr<ASTBinExpression>& binExpr) const {
        // IN THE FUTURE: when there are more types, check for type compatibility
        auto& lhs = *binExpr.get()->lhs.get();
        auto& rhs = *binExpr.get()->rhs.get();
        DataType lhs_data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table, function_table}, lhs.expression);
        DataType rhs_data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table, function_table}, rhs.expression);
        if (lhs_data_type != rhs_data_type) {
            auto& meta = binExpr.get()->start_token_meta;
            std::cout << "SEMANTIC WARNING AT "
                      << Globals::getInstance().getCurrentFilePosition(meta.line_num, meta.line_pos)
                      << ": Binary operation of different data types. Data will be narrowed." << std::endl;
        }
        // type narrowing
        DataType data_type = std::min(lhs_data_type, rhs_data_type);
        lhs.data_type = data_type;
        rhs.data_type = data_type;
        return data_type;
    }

    DataType operator()(const ASTParenthesisExpression& paren_expr) const {
        auto& inner_expression = *paren_expr.expression.get();
        return std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table, function_table},
                          inner_expression.expression);
    }

    DataType operator()(ASTFunctionCallExpression& function_call_expr) const {
        auto& func_name = function_call_expr.function_name;
        auto& start_token_meta = function_call_expr.start_token_meta;
        if (function_table.count(func_name) == 0) {
            std::stringstream error;
            error << "Unknown function " << func_name << ".";
            throw SemanticAnalyzerException(error.str(), start_token_meta);
        }
        auto& function_header_data = function_table.at(function_call_expr.function_name);
        auto& function_expected_params = function_header_data.parameters;
        std::vector<ASTExpression>& provided_params = function_call_expr.parameters;
        if (provided_params.size() != function_expected_params.size()) {
            std::stringstream error;
            error << "Function " << func_name << " expected " << function_expected_params.size()
                  << " parameters, instead got " << provided_params.size() << ".";
            throw SemanticAnalyzerException(error.str(), start_token_meta);
        }
        for (size_t i = 0; i < provided_params.size(); ++i) {
            provided_params[i].data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table, function_table},
                                                      provided_params[i].expression);
            if (provided_params[i].data_type == function_expected_params[i].data_type) {
                // no type issues
                continue;
            }
            auto& meta = provided_params[i].start_token_meta;
            std::cout << "SEMANTIC WARNING AT "
                      << Globals::getInstance().getCurrentFilePosition(meta.line_num, meta.line_pos)
                      << ": Provided parameter of different data type. Data will be narrowed/widened." << std::endl;
            provided_params[i].data_type = function_expected_params[i].data_type;
        }

        return function_header_data.data_type;
    }
};

struct SemanticAnalyzer::StatementVisitor {
    SemanticAnalyzer* analyzer;
    std::string function_name = "";
    void operator()(const std::shared_ptr<ASTStatementExit>& exit) const {
        auto& expression = exit.get()->status_code;
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table, analyzer->m_function_table},
                       expression.expression);
    }
    void operator()(const std::shared_ptr<ASTStatementVar>& var_declare) const {
        auto& start_token_meta = var_declare.get()->start_token_meta;
        if (datatype_mapping.count(var_declare.get()->data_type_str) == 0) {
            std::stringstream errorMessage;
            errorMessage << "Unknown data type '" << var_declare.get()->data_type_str << "'";
            throw SemanticAnalyzerException(errorMessage.str(), start_token_meta);
        }
        DataType data_type = datatype_mapping.at(var_declare.get()->data_type_str);
        var_declare.get()->data_type = data_type;
        try {
            analyzer->m_symbol_table.insert(
                var_declare.get()->name,
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
        DataType rhs_data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table, analyzer->m_function_table},
                       expression.expression);
        // IN THE FUTURE: when there are more types, check for type compatibility
        if (rhs_data_type > data_type) {
            // type narrowing
            rhs_data_type = data_type;
        }
        expression.data_type = rhs_data_type;
    }
    void operator()(const std::shared_ptr<ASTStatementAssign>& var_assign) const {
        auto& meta = var_assign.get()->start_token_meta;
        auto& name = var_assign.get()->name;
        auto& expression = var_assign.get()->value;
        SymbolTable::Variable variableData;
        if (!analyzer->m_symbol_table.lookup(name, variableData)) {
            std::stringstream error;
            error << "Assignment of variable '" << name << "' which does not exist in current scope";
            throw SemanticAnalyzerException(error.str(), meta);
        }
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table, analyzer->m_function_table},
                       expression.expression);
        if (expression.data_type != variableData.data_type) {
            std::cout << "SEMANTIC WARNING AT "
                      << Globals::getInstance().getCurrentFilePosition(meta.line_num, meta.line_pos)
                      << ": Assignment operation of different data types. Data will be narrowed/widened." << std::endl;
            expression.data_type = variableData.data_type;
        }
    }
    void operator()(const std::shared_ptr<ASTStatementScope>& scope) const {
        analyzer->analyze_scope(scope.get()->statements, function_name);
    }
    void operator()(const std::shared_ptr<ASTStatementIf>& _if) const {
        auto& expression = _if.get()->expression;
        auto& success_statement = _if.get()->success_statement;
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table, analyzer->m_function_table},
                       expression.expression);
        std::visit(SemanticAnalyzer::StatementVisitor{.analyzer = analyzer, .function_name = function_name},
                   success_statement.get()->statement);
        if (_if.get()->fail_statement != nullptr) {
            std::visit(SemanticAnalyzer::StatementVisitor{.analyzer = analyzer, .function_name = function_name},
                       _if.get()->fail_statement.get()->statement);
        }
    }
    void operator()(const std::shared_ptr<ASTStatementWhile>& while_statement) const {
        // NOTE: identical to analysis of if statements
        auto& expression = while_statement.get()->expression;
        auto& success_statement = while_statement.get()->success_statement;
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table, analyzer->m_function_table},
                       expression.expression);
        std::visit(SemanticAnalyzer::StatementVisitor{.analyzer = analyzer, .function_name = function_name},
                   success_statement.get()->statement);
    }
    void operator()(const std::shared_ptr<ASTStatementFunction>& function_statement) const {
        (void)function_statement;  // ignore unused
        assert(false && "Should never reach here. Function statements are to be parsed seperately");
    }
    void operator()(const std::shared_ptr<ASTStatementReturn>& return_statement) const {
        if (function_name.empty()) {
            throw SemanticAnalyzerException("Can't use 'return' outside of a function",
                                            return_statement.get()->start_token_meta);
        }
        auto& meta = return_statement.get()->start_token_meta;
        auto& function_header = analyzer->m_function_table.at(function_name);
        auto& expression = return_statement.get()->expression;
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table, analyzer->m_function_table},
                       expression.expression);
        if (expression.data_type != function_header.data_type) {
            std::cout
                << "SEMANTIC WARNING AT " << Globals::getInstance().getCurrentFilePosition(meta.line_num, meta.line_pos)
                << ": return statement with different datatype of function return type. Data will be narrowed/widened."
                << std::endl;
            expression.data_type = function_header.data_type;
        }
    }
};

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