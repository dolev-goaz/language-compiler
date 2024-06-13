#include "semantic_analyzer.hpp"
#include "semantic_visitor.hpp"

void SemanticAnalyzer::semantic_warning(const std::string& message, const TokenMeta& position) {
    std::cout << "SEMANTIC WARNING AT "
              << Globals::getInstance().getCurrentFilePosition(position.line_num, position.line_pos) << ": " << message
              << std::endl;
}

void SemanticAnalyzer::assert_cast_expression(ASTExpression& expression, std::shared_ptr<DataType> data_type,
                                              bool show_warning) {
    auto compatibility = expression.data_type->is_compatible(*data_type);
    std::stringstream casting_msg;
    casting_msg << "Casting '" << expression.data_type->toString() << "' to '" << data_type->toString() << "'.";
    expression.data_type = data_type;
    switch (compatibility) {
        case CompatibilityStatus::Compatible:
            return;
        case CompatibilityStatus::CompatibleWithWarning:
            if (show_warning) {
                semantic_warning("Implicit casting. Data will be narrowed/widened. " + casting_msg.str(),
                                 expression.start_token_meta);
            }
            return;
        case CompatibilityStatus::NotCompatible:
            throw SemanticAnalyzerException("Implicit casting of non-compatible datatypes. " + casting_msg.str(),
                                            expression.start_token_meta);
        default:
            assert(false && "Shouldn't reach here");
    }
}

std::shared_ptr<DataType> SemanticAnalyzer::create_data_type(const std::vector<Token> data_type_tokens) {
    Token base_type_token = data_type_tokens.at(0);
    std::string& base_type_str = base_type_token.value.value();
    std::shared_ptr<DataType> type;
    try {
        type = BasicType::makeBasicType(base_type_str);
    } catch (const std::exception& e) {
        throw SemanticAnalyzerException(e.what(), base_type_token.meta);
    }

    size_t token_index = 1;
    auto token_count = data_type_tokens.size();

    Token array_size_token;
    size_t array_size;

    std::stack<size_t> array_sizes;

    while (token_index < token_count) {
        auto current = data_type_tokens.at(token_index);
        switch (current.type) {
            case TokenType::star:
                type = std::make_shared<PointerType>(type);
                break;
            case TokenType::open_square:
                array_size_token = data_type_tokens.at(token_index + 1);
                token_index += 2;  // read count and closing square token
                array_size = std::stoi(array_size_token.value.value());
                array_sizes.push(array_size);
                break;
            default:
                assert(false && "Shouldn't reach here");
        }
        token_index += 1;
    }

    // we invert the order of declaration, like c does(for some reason)
    while (!array_sizes.empty()) {
        array_size = array_sizes.top();
        array_sizes.pop();
        type = std::make_shared<ArrayType>(type, array_size);
    }

    return type;
}

void SemanticAnalyzer::analyze() {
    this->m_symbol_table.enterScope();
    auto& statements = m_prog.statements;
    auto& functions = m_prog.functions;
    // first passage through functions- function header
    for (auto& function : functions) {
        analyze_function_header(*function);
    }
    // pass through global statements
    for (auto& statement : statements) {
        analyze_statement(*statement);
    }
    // second passage through functions- function body
    for (auto& function : functions) {
        analyze_function_body(*function);
    }
    this->m_symbol_table.exitScope();
}