#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

void SemanticAnalyzer::semantic_warning(const std::string& message, const TokenMeta& position) {
    std::cout << "SEMANTIC WARNING AT "
              << Globals::getInstance().getCurrentFilePosition(position.line_num, position.line_pos) << ": " << message
              << std::endl;
}

void SemanticAnalyzer::assert_cast_expression(ASTExpression& expression, std::shared_ptr<DataType> data_type,
                                              bool show_warning) {
    expression.data_type = data_type;
    auto compatibility = expression.data_type->isCompatible(*data_type);
    switch (compatibility) {
        case CompatibilityStatus::Compatible:
            return;
        case CompatibilityStatus::CompatibleWithWarning:
            semantic_warning("Implicit casting. Data will be narrowed/widened.", expression.start_token_meta);
            return;
        case CompatibilityStatus::NotCompatible:
            throw SemanticAnalyzerException("Implicit casting of non-compatible datatypes",
                                            expression.start_token_meta);
        default:
            assert(false && "Shouldn't reach here");
    }
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