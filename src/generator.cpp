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

// --------- expression generation

void Generator::generate_expression(const ASTExpression& expression) {
    std::visit(Generator::ExpressionVisitor{*this}, expression.expression);
}

void Generator::generate_expression_identifier(const ASTIdentifier& identifier) {
    if (m_variables.count(identifier.value) == 0) {
        std::cerr << "Variable '" << identifier.value << "' does not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // get variable metadata
    Generator::Variable variable = m_variables.at(identifier.value);

    // get variable position in the stack
    int offset = m_stack_size - variable.stack_location_bytes;
    // rsp was on the next FREE address, offset it back to the variable's position.
    offset -= variable.size_bytes;

    // push variable value into stack
    push_stack_offset(offset, variable.size_bytes);
}

void Generator::generate_expression_int_literal(const ASTIntLiteral& literal) {
    // TODO: are literals always 8 bytes?
    push_stack_literal(literal.value, 8);
}

// --------- statement generation

void Generator::generate_statement(const ASTStatement& statement) {
    std::visit(Generator::StatementVisitor{*this}, statement.statement);
}
void Generator::generate_statement_exit(const ASTStatementExit& exit_statement) {
    generate_expression(exit_statement.status_code);
    m_generated << "\tmov rax, 60" << std::endl;

    pop_stack_register("rdi", 8);
    m_generated << "\tsyscall" << std::endl;
}

void Generator::generate_statement_var_declare(const ASTStatementVar& var_statement) {
    // check variable name already exists
    if (m_variables.count(var_statement.name) > 0) {
        std::cerr << "Variable '" << var_statement.name << "' already exists!" << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator::Variable var = {
        .stack_location_bytes = m_stack_size,
        .size_bytes = 8,  // hard coded for now
    };
    // TODO: need to refer to size_bytes when initializing(push only the required size)
    generate_expression(var_statement.value.value());

    m_variables.insert({var_statement.name, var});  // variable is now set
}