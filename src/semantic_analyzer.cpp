#include "../header/semantic_analyzer.hpp"

std::map<std::string, DataType> datatype_mapping = {
    {"int_8", DataType::int_8},
    {"int_16", DataType::int_16},
    {"int_32", DataType::int_32},
    {"int_64", DataType::int_64},
};

struct SemanticAnalyzer::ExpressionVisitor {
    SymbolTable::SemanticScopeStack& symbol_table;
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
        return std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table}, atomic.get()->value);
    }

    DataType operator()(const std::shared_ptr<ASTBinExpression>& binExpr) const {
        // IN THE FUTURE: when there are more types, check for type compatibility
        auto& lhs = *binExpr.get()->lhs.get();
        auto& rhs = *binExpr.get()->rhs.get();
        DataType lhs_data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table}, lhs.expression);
        DataType rhs_data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table}, rhs.expression);
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
};

struct SemanticAnalyzer::StatementVisitor {
    SemanticAnalyzer* analyzer;
    void operator()(const std::shared_ptr<ASTStatementExit>& exit) const {
        auto& expression = exit.get()->status_code;
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table}, expression.expression);
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
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table}, expression.expression);
        // IN THE FUTURE: when there are more types, check for type compatibility
        if (rhs_data_type > data_type) {
            // type narrowing
            rhs_data_type = data_type;
        }
        expression.data_type = rhs_data_type;
    }
    void operator()(const std::shared_ptr<ASTStatementScope>& scope) const {
        analyzer->analyze_scope(scope.get()->statements);
    }
};

void SemanticAnalyzer::analyze_scope(const std::vector<std::shared_ptr<ASTStatement>>& statements) {
    this->m_symbol_table.enterScope();
    for (auto& statement : statements) {
        std::visit(SemanticAnalyzer::StatementVisitor{this}, statement.get()->statement);
    }
    this->m_symbol_table.exitScope();
}

void SemanticAnalyzer::analyze() {
    // basically, the entire program is wrapped in a scope, allowing declarations outside of a scope- creating the
    // global scope.
    analyze_scope(m_prog.statements);
}