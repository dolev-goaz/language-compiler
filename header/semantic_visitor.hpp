#pragma once
#include "semantic_analyzer.hpp"
struct SemanticAnalyzer::ExpressionVisitor {
    SemanticAnalyzer* analyzer;
    DataType operator()(ASTIdentifier& identifier) const {
        SymbolTable::Variable literal_data;
        if (!analyzer->m_symbol_table.lookup(identifier.value, literal_data)) {
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
        return std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, atomic.get()->value);
    }

    DataType operator()(const std::shared_ptr<ASTBinExpression>& binExpr) const {
        // IN THE FUTURE: when there are more types, check for type compatibility
        auto& lhs = *binExpr.get()->lhs.get();
        auto& rhs = *binExpr.get()->rhs.get();
        DataType lhs_data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, lhs.expression);
        DataType rhs_data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, rhs.expression);
        // if rhs or lhs are int literals, no need for data narrowing warnings
        if ((lhs_data_type != rhs_data_type) && (!analyzer->is_int_literal(lhs) && !analyzer->is_int_literal(rhs))) {
            auto& meta = binExpr.get()->start_token_meta;
            semantic_warning("Binary operation of different data types. Data will be narrowed.", meta);
        }

        // type narrowing
        rhs.data_type = lhs_data_type;
        lhs.data_type = lhs_data_type;
        return lhs_data_type;
    }

    DataType operator()(const ASTParenthesisExpression& paren_expr) const {
        auto& inner_expression = *paren_expr.expression.get();
        return std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, inner_expression.expression);
    }

    DataType operator()(ASTFunctionCallExpression& function_call_expr) const {
        auto& func_name = function_call_expr.function_name;
        auto& start_token_meta = function_call_expr.start_token_meta;
        if (analyzer->m_function_table.count(func_name) == 0) {
            std::stringstream error;
            error << "Unknown function " << func_name << ".";
            throw SemanticAnalyzerException(error.str(), start_token_meta);
        }
        auto& function_header_data = analyzer->m_function_table.at(function_call_expr.function_name);
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
                std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, provided_params[i].expression);
            if (provided_params[i].data_type == function_expected_params[i].data_type) {
                // no type issues
                continue;
            }

            if (!analyzer->is_int_literal(provided_params[i])) {
                auto& meta = provided_params[i].start_token_meta;
                semantic_warning("Provided parameter of different data type. Data will be narrowed/widened.", meta);
            }
            provided_params[i].data_type = function_expected_params[i].data_type;
        }
        function_call_expr.return_data_type = function_header_data.data_type;
        return function_header_data.data_type;
    }
};

struct SemanticAnalyzer::StatementVisitor {
    SemanticAnalyzer* analyzer;
    std::string function_name = "";
    void operator()(const std::shared_ptr<ASTStatementExit>& exit) const {
        auto& expression = exit.get()->status_code;
        expression.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, expression.expression);
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
        DataType rhs_data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, expression.expression);
        // IN THE FUTURE: when there are more types, check for type compatibility
        if (rhs_data_type != data_type) {
            // type narrowing/widening
            if (!analyzer->is_int_literal(expression)) {
                semantic_warning("Assignment operation of different data types. Data will be narrowed/widened.",
                                 start_token_meta);
            }
            // if rhs is int literal we can just cast it, no need to warn about it
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
        expression.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, expression.expression);
        if (expression.data_type != variableData.data_type) {
            if (!analyzer->is_int_literal(expression)) {
                semantic_warning("Assignment operation of different data types. Data will be narrowed/widened.", meta);
            }
            expression.data_type = variableData.data_type;
        }
    }
    void operator()(const std::shared_ptr<ASTStatementScope>& scope) const {
        analyzer->analyze_scope(scope.get()->statements, function_name);
    }
    void operator()(const std::shared_ptr<ASTStatementIf>& _if) const {
        auto& expression = _if.get()->expression;
        auto& success_statement = _if.get()->success_statement;
        expression.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, expression.expression);
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
        expression.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, expression.expression);
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
        function_header.found_return_statement = true;
        auto& expression = return_statement.get()->expression;
        expression.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer}, expression.expression);
        if (expression.data_type != function_header.data_type) {
            if (!analyzer->is_int_literal(expression)) {
                semantic_warning(
                    "Return statement with different datatype of function return type. Data will be narrowed/widened.",
                    meta);
            }
            expression.data_type = function_header.data_type;
        }
    }
};