#include "../header/generator.hpp"

#include "../header/generator_visitor.hpp"
std::string Generator::generate_program() {
    m_generated << "global main" << std::endl << "main:" << std::endl;

    for (auto statement : m_prog.statements) {
        generate_statement(statement);
    }

    // default exit statement
    m_generated << std::endl << "; default program end" << std::endl;
    m_generated << "\tmov rax, 60" << std::endl;
    m_generated << "\tmov rdi, 0" << std::endl;
    m_generated << "\tsyscall" << std::endl;
    return m_generated.str();
}

void Generator::generate_statement(const ASTStatement& statement) {
    std::visit(Generator::StatementVisitor{*this}, statement.statement);
}
void Generator::generate_exit(const ASTStatementExit& exit_statement) {
    generate_expression(exit_statement.status_code);
    m_generated << "\tmov rax, 60" << std::endl;
    m_generated << "\tpop rdi" << std::endl;
    m_generated << "\tsyscall" << std::endl;
}

void Generator::generate_expression(const ASTExpression& expression) {
    std::visit(Generator::ExpressionVisitor{*this}, expression.expression);
}