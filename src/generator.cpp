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

    pop_stack_register("rdi", 8);
    m_generated << "\tsyscall" << std::endl;
}

void Generator::generate_expression(const ASTExpression& expression) {
    std::visit(Generator::ExpressionVisitor{*this}, expression.expression);
}

void Generator::push_stack_literal(const std::string& value, size_t size) {
    // TODO: include size in the calculations instead of using QWORD immediately.
    m_generated << "\tpush QWORD " << value << std::endl;
    m_stack_size += size;
}
void Generator::push_stack_offset(int offset, size_t size) {
    // TODO: take size into account(only push the bytes needed) instead of using QWORD.
    m_generated << "\tpush QWORD [rsp + " << offset << "]" << std::endl;
    m_stack_size += size;
}
void Generator::pop_stack_register(const std::string& reg, size_t size) {
    m_generated << "\tpop " << reg << std::endl;
    m_stack_size -= size;
}