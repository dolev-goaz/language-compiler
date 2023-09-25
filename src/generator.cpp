#include "../header/generator.hpp"

#include "../header/generator_visitor.hpp"
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
    std::visit(Generator::StatementVisitor{*this}, statement.statement);
}
void Generator::generate_exit(const ASTStatementExit& exit_statement) {
    std::string expression = generate_expression(exit_statement.status_code);
    m_generated << "    mov rax, 60" << std::endl;
    m_generated << "    mov rdi, " << expression << std::endl;
    m_generated << "    syscall" << std::endl;
}

std::string Generator::generate_expression(const ASTExpression& expression) {
    return std::visit(Generator::ExpressionVisitor{*this}, expression.expression);
}