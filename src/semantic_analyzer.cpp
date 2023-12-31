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
};

struct SemanticAnalyzer::StatementVisitor {
    SemanticAnalyzer* analyzer;
    ASTStatement operator()(const ASTStatementExit& exit) const {
        auto copy_exit = exit;
        ASTExpression expression = copy_exit.status_code;
        expression.data_type =
            std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table}, expression.expression);
        copy_exit.status_code = expression;

        return ASTStatement{.statement = copy_exit};
    }
    ASTStatement operator()(const ASTStatementVar& var_declare) const {
        auto copy_var_declare = var_declare;
        if (datatype_mapping.count(copy_var_declare.data_type_str) == 0) {
            std::cerr << "Unknown data type " << copy_var_declare.data_type_str << std::endl;
            exit(EXIT_FAILURE);
        }
        DataType data_type = datatype_mapping.at(copy_var_declare.data_type_str);
        analyzer->m_symbol_table.insert(copy_var_declare.name, SymbolTable::Variable{.data_type = data_type});
        copy_var_declare.data_type = data_type;

        ASTExpression expression;
        if (copy_var_declare.value.has_value()) {
            expression = copy_var_declare.value.value();

            DataType rhs_data_type =
                std::visit(SemanticAnalyzer::ExpressionVisitor{analyzer->m_symbol_table}, expression.expression);
            // IN THE FUTURE: when there are more types, check for type compatibility
            if (rhs_data_type > data_type) {
                // type narrowing
                rhs_data_type = data_type;
            }
            expression.data_type = rhs_data_type;
        } else {
            ASTIntLiteral zero_literal = {.value = "0"};
            expression = ASTExpression{
                .data_type = data_type,
                .expression = zero_literal,
            };
        }
        copy_var_declare.value = std::make_optional(expression);
        return ASTStatement{.statement = copy_var_declare};
    }
};
ASTProgram SemanticAnalyzer::analyze() {
    std::vector<ASTStatement> modified_statements;
    this->m_symbol_table.enterScope();
    for (auto& statement : m_prog.statements) {
        auto updated = std::visit(SemanticAnalyzer::StatementVisitor{this}, statement.statement);
        modified_statements.push_back(updated);
    }
    m_prog.statements = modified_statements;
    this->m_symbol_table.exitScope();

    return m_prog;
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
