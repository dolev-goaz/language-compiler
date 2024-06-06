#include "debug_utils.hpp"

std::string debug_utils::print_indentation(int level) {
    std::string out;
    for (int i = 0; i < level; ++i) {
        out.append("  ");
    }
    return out;
}

std::string debug_utils::visualize_function_call(const ASTFunctionCall& funcCall) {
    std::stringstream out;
    out << funcCall.function_name;
    std::stringstream parameters;
    for (const auto& param : funcCall.parameters) {
        parameters << visualize_expression(std::make_shared<ASTExpression>(param)) << ",";
    }
    std::string params_str = parameters.str();
    params_str = params_str.substr(0, params_str.size() - 1);

    out << "(" << params_str << ");";
    return out.str();
    ;
}

std::string debug_utils::visualize_atomic_expression(const ASTAtomicExpression& atomicExpr) {
    std::stringstream out;
    std::visit(
        [&](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, ASTIntLiteral>) {
                out << value.value;
            } else if constexpr (std::is_same_v<T, ASTIdentifier>) {
                out << value.value;
            } else if constexpr (std::is_same_v<T, ASTCharLiteral>) {
                out << "'" << value.value << "'";
            } else if constexpr (std::is_same_v<T, ASTParenthesisExpression>) {
                out << "(" << visualize_expression(value.expression) << ")";
            } else if constexpr (std::is_same_v<T, ASTFunctionCall>) {
                out << visualize_function_call(value);
            }
        },
        atomicExpr.value);
    return out.str();
}

std::string debug_utils::visualize_expression(const std::shared_ptr<ASTExpression>& expr) {
    std::stringstream out;
    std::visit(
        [&](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::shared_ptr<ASTAtomicExpression>>) {
                out << visualize_atomic_expression(*value);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTBinExpression>>) {
                out << visualize_expression(value->lhs);
                out << " " << (int)value->operation << " ";
                out << visualize_expression(value->rhs);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTUnaryExpression>>) {
                out << visualize_expression(value->expression);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTArrayIndexExpression>>) {
                out << visualize_expression(value->expression);
                out << "[" << visualize_expression(value->index) << "]";
            }
        },
        expr->expression);
    return out.str();
}

std::string debug_utils::visualize_statement_exit(const ASTStatementExit& stmt) {
    std::stringstream out;
    out << "exit(" << visualize_expression(std::make_shared<ASTExpression>(stmt.status_code)) << ");";
    return out.str();
}

std::string debug_utils::visualize_statement_var(const ASTStatementVar& stmt) {
    std::stringstream out;
    out << stmt.data_type->toString() << " " << stmt.name;
    if (stmt.value.has_value()) {
        out << " = " << visualize_expression(std::make_shared<ASTExpression>(stmt.value.value()));
    }
    out << ";";
    return out.str();
}

std::string debug_utils::visualize_statement_assign(const ASTStatementAssign& stmt) {
    std::stringstream out;
    out << stmt.name << " = " << visualize_expression(std::make_shared<ASTExpression>(stmt.value)) << ";";
    return out.str();
}

std::string debug_utils::visualize_statement_scope(const ASTStatementScope& stmt, int level) {
    std::stringstream out;
    // no need to print actual {}
    for (const auto& statement : stmt.statements) {
        out << visualize_statement(statement, level + 1) << std::endl;
    }
    return out.str();
}

std::string debug_utils::visualize_statement_if(const ASTStatementIf& stmt, int level) {
    std::stringstream out;
    out << "if (" << visualize_expression(std::make_shared<ASTExpression>(stmt.expression)) << ")" << std::endl
        << visualize_statement(stmt.success_statement, level + 1);
    if (stmt.fail_statement) {
        out << std::endl << print_indentation(level) << "else " << visualize_statement(stmt.fail_statement, level);
    }
    return out.str();
}

std::string debug_utils::visualize_statement_while(const ASTStatementWhile& stmt, int level) {
    std::stringstream out;
    out << "while (" << visualize_expression(std::make_shared<ASTExpression>(stmt.expression)) << ")" << std::endl
        << visualize_statement(stmt.success_statement, level + 1);
    return out.str();
}

std::string debug_utils::visualize_statement_function(const ASTStatementFunction& stmt, int level) {
    std::stringstream out;
    out << "func " << stmt.return_data_type->toString() << " " << stmt.name;
    std::stringstream parameters;
    for (const auto& param : stmt.parameters) {
        parameters << param.data_type->toString() << " " << param.name << ",";
    }
    std::string parameters_str = parameters.str();
    parameters_str = parameters_str.substr(0, parameters_str.size() - 1);
    out << "(" << parameters_str << ")" << visualize_statement(stmt.statement, level + 1);
    return out.str();
}

std::string debug_utils::visualize_statement_return(const ASTStatementReturn& stmt) {
    std::stringstream out;
    out << "return " << visualize_expression(std::make_shared<ASTExpression>(*stmt.expression)) << ";";
    return out.str();
}

std::string debug_utils::visualize_statement(const std::shared_ptr<ASTStatement>& stmt, int level) {
    return print_indentation(level) +
           std::visit(
               [&](auto&& value) {
                   using T = std::decay_t<decltype(value)>;
                   if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementExit>>) {
                       return visualize_statement_exit(*value);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementVar>>) {
                       return visualize_statement_var(*value);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementAssign>>) {
                       return visualize_statement_assign(*value);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementScope>>) {
                       return visualize_statement_scope(*value, level);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementIf>>) {
                       return visualize_statement_if(*value, level);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementWhile>>) {
                       return visualize_statement_while(*value, level);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementFunction>>) {
                       return visualize_statement_function(*value, level);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTStatementReturn>>) {
                       return visualize_statement_return(*value);
                   } else if constexpr (std::is_same_v<T, std::shared_ptr<ASTFunctionCall>>) {
                       return visualize_function_call(*value);
                   }
               },
               stmt->statement);
}

std::string debug_utils::visualize_ast(const std::shared_ptr<ASTProgram>& program) {
    std::stringstream out;
    for (const auto& function : program->functions) {
        out << visualize_statement_function(*function, 0) << std::endl;
    }
    for (const auto& statement : program->statements) {
        out << visualize_statement(statement, 0) << std::endl;
    }

    return out.str();
}