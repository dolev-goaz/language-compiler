#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

std::map<std::string, DataType> datatype_mapping = {
    {"void", DataType::_void},  {"int_8", DataType::int_8},   {"int_16", DataType::int_16},
    {"char", DataType::int_16}, {"int_32", DataType::int_32}, {"int_64", DataType::int_64},
};

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