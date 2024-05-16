#include "../header/semantic_analyzer.hpp"

std::map<std::string, DataType> datatype_mapping = {
    {"int_8", DataType::int_8},
    {"int_16", DataType::int_16},
    {"int_32", DataType::int_32},
    {"int_64", DataType::int_64},
};

struct SemanticAnalyzer::ExpressionVisitor {
    SymbolTable& symbol_table;
    DataType operator()(ASTIdentifier& identifier) const {
        SymbolTable::Variable literal_data;
        if (!symbol_table.lookup(identifier.value, literal_data)) {
            std::cerr << "Unknown identifier " << identifier.value << std::endl;
            exit(EXIT_FAILURE);
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
        auto& lhs = *binExpr.get()->lhs.get();
        auto& rhs = *binExpr.get()->rhs.get();
        lhs.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table}, lhs.expression);
        rhs.data_type = std::visit(SemanticAnalyzer::ExpressionVisitor{symbol_table}, rhs.expression);
        // type widening
        return std::max(lhs.data_type, rhs.data_type);
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
        if (datatype_mapping.count(var_declare.get()->data_type_str) == 0) {
            std::cerr << "Unknown data type " << var_declare.get()->data_type_str << std::endl;
            exit(EXIT_FAILURE);
        }
        DataType data_type = datatype_mapping.at(var_declare.get()->data_type_str);
        var_declare.get()->data_type = data_type;
        analyzer->m_symbol_table.insert(var_declare.get()->name, SymbolTable::Variable{.data_type = data_type});

        if (var_declare.get()->value.has_value()) {
            auto& expression = var_declare.get()->value.value();

            DataType rhs_data_type =
                std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table}, expression.expression);
            // IN THE FUTURE: when there are more types, check for type compatibility
            if (rhs_data_type > data_type) {
                // type narrowing
                rhs_data_type = data_type;
            }
            expression.data_type = rhs_data_type;
        } else {
            ASTAtomicExpression zero_literal = ASTAtomicExpression{
                .start_token_meta = {0, 0},
                .value = ASTIntLiteral{.start_token_meta = {0, 0}, .value = "0"},
            };
            var_declare.get()->value =
                std::make_optional(ASTExpression{.start_token_meta = {0, 0},
                                                 .data_type = DataType::int_64,
                                                 .expression = std::make_shared<ASTAtomicExpression>(zero_literal)});
        }
    }
};
void SemanticAnalyzer::analyze() {
    this->m_symbol_table.enterScope();
    for (auto& statement : m_prog.statements) {
        std::visit(SemanticAnalyzer::StatementVisitor{this}, statement.get()->statement);
    }
    this->m_symbol_table.exitScope();
}

void SymbolTable::enterScope() { scope_stack.push(std::map<std::string, SymbolTable::Variable>()); }

void SymbolTable::exitScope() {
    // throw error if no existing scope
    if (!scope_stack.empty()) {
        scope_stack.pop();
    }
}

void SymbolTable::insert(const std::string& identifier, Variable value) {
    if (scope_stack.empty()) {
        std::stringstream err_stream;
        err_stream << "Variable '" << identifier << "' cannot be declared outside of a scope.";
        throw SemanticAnalyzerException(err_stream.str());
    }

    SymbolTable::scope current_scope = scope_stack.top();
    if (current_scope.count(identifier) > 0) {
        std::stringstream err_stream;
        err_stream << "Variable '" << identifier << "' already exists in the current scope.";
        throw SemanticAnalyzerException(err_stream.str());
    }
    scope_stack.top()[identifier] = value;
}

bool SymbolTable::lookup(const std::string& identifier, Variable& value) const {
    // throw error if no existing scope
    for (auto it = scope_stack.top().rbegin(); it != scope_stack.top().rend(); ++it) {
        if (it->first == identifier) {
            value = it->second;
            return true;
        }
    }
    return false;
}
