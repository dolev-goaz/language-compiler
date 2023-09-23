#include "../header/generator.hpp"
std::string Generator::generate_program() {
    m_generated << "global main" << std::endl << "main:" << std::endl;

    for (auto statement : m_prog.statements) {
        generate_statement(statement);
    }

    // default exit statement
    m_generated << std::endl << "; default program end" << std::endl;
    m_generated << "    mov rax, 60" << std::endl;
    m_generated << "    mov rdi, 0" << std::endl;
    m_generated << "    syscall" << std::endl;
    return m_generated.str();
}

void Generator::generate_statement(const ASTStatement& statement) {
    std::visit(
        [this](const auto& n) {
            using StatementType = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<StatementType, ASTStatementExit>) {
                generate_exit(n);
                return;
            }

            std::cerr << "Unimplemented statement!" << std::endl;
            exit(EXIT_FAILURE);
        },
        statement.statement);
}
void Generator::generate_exit(const ASTStatementExit& exit_statement) {
    std::string expression = generate_expression(exit_statement.status_code);
    m_generated << "    mov rax, 60" << std::endl;
    m_generated << "    mov rdi, " << expression << std::endl;
    m_generated << "    syscall" << std::endl;
}

std::string Generator::generate_expression(const ASTExpression& expression) {
    std::string result = std::visit(
        [this](const auto& n) {
            using ExpressionType = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<ExpressionType, ASTIntLiteral>) {
                return ((ASTIntLiteral)n).value.value.value();
            }
            std::cerr << "Unimplemented expression type!" << std::endl;
            exit(EXIT_FAILURE);

            // unreachable code
            return std::string("");
        },
        expression.expression);
    return result;
}