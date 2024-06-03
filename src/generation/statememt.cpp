#include "generator.hpp"
#include "generator_visitor.hpp"

void Generator::generate_statement(const ASTStatement& statement) {
    std::visit(Generator::StatementVisitor{*this}, statement.statement);
}

void Generator::generate_statement_exit(const std::shared_ptr<ASTStatementExit>& exit_statement) {
    generate_expression(exit_statement->status_code);
    m_generated << ";\tExit Statement" << std::endl;
    m_generated << "\tmov rax, 60" << std::endl;

    size_t size = exit_statement->status_code.data_type->get_size_bytes();
    pop_stack_register("rdi", 8, size);
    m_generated << "\tsyscall" << std::endl;
}

void Generator::generate_statement_var_assignment(const std::shared_ptr<ASTStatementAssign>& var_assign_statement) {
    Generator::Variable variableData = assert_get_variable_data(var_assign_statement->name);

    m_generated << ";\tVariable Assigment " << var_assign_statement->name << " BEGIN" << std::endl;

    generate_expression(var_assign_statement->value);
    size_t expression_size_bytes = var_assign_statement->value.data_type->get_size_bytes();

    // NOTE: assumes 'size_bytes_to_register' holds registers rax, eax, ax, al
    // popping to the largest register to allow data widening if necessary, up to 8 bytes
    pop_stack_register("rax", 8, expression_size_bytes);
    std::string& temp_register = size_bytes_to_register.at(variableData.size_bytes);

    load_memory_address("rdx", var_assign_statement->name);
    m_generated << "\tmov [rdx], " << temp_register << std::endl;
    m_generated << ";\tVariable Assigment " << var_assign_statement->name << " END" << std::endl << std::endl;
}

void Generator::generate_statement_var_declare(const std::shared_ptr<ASTStatementVar>& var_statement) {
    // no need to check for duplicate variable names, since it was checked in semantic analysis
    size_t size_bytes = var_statement->data_type->get_size_bytes();

    Generator::Variable var = {
        .stack_location_bytes = m_stack_size,
        .size_bytes = size_bytes,
    };
    m_generated << ";\tVariable Declaration " << var_statement->name << " BEGIN" << std::endl;
    if (var_statement->value.has_value()) {
        generate_expression(var_statement->value.value());
    } else {
        m_generated << "\tsub rsp, " << size_bytes << std::endl;
        // allocate stack initializing to 0
        auto is_array = (bool)(dynamic_cast<ArrayType*>(var_statement->data_type.get()));
        if (is_array) {
            // TODO: should probably only do this if the array doesnt get initialized with a value
            m_generated << "\t; Initialize Arrays To 0" << std::endl
                        << "\tmov rcx, " << size_bytes << std::endl
                        << "\tmov rdi, rsp" << std::endl
                        << "\tmov rax, 0" << std::endl
                        << "\trep stosb" << std::endl;
        }
        m_stack_size += size_bytes;
    }

    m_stack.insert(var_statement->name, var);
    m_generated << ";\tVariable Declaration " << var_statement->name << " END" << std::endl << std::endl;
}

void Generator::generate_statement_scope(const std::shared_ptr<ASTStatementScope>& scope_statement) {
    enter_scope();
    for (auto& statement : scope_statement->statements) {
        this->generate_statement(*statement);
    }
    exit_scope();
}

void Generator::generate_statement_if(const std::shared_ptr<ASTStatementIf>& if_statement) {
    std::stringstream after_if_label;
    after_if_label << ".after_if_statement_" << m_condition_counter;
    std::stringstream after_else_label;
    after_else_label << ".after_else_statement_" << m_condition_counter;
    ++m_condition_counter;

    auto& expression = if_statement->expression;
    auto& success_statement = if_statement->success_statement;
    auto& fail_statement = if_statement->fail_statement;
    size_t size_bytes = expression.data_type->get_size_bytes();
    generate_expression(expression);
    pop_stack_register("rax", 8, size_bytes);  // rax = lhs
    m_generated << "\ttest rax, rax" << std::endl
                << "\tjz " << after_if_label.str() << "; if the expression is false-y, skip the 'if' block's statements"
                << std::endl;
    generate_statement(*success_statement);
    if (!fail_statement) {
        // no 'else' statement
        m_generated << after_if_label.str() << ":" << std::endl;
        return;
    }
    m_generated << "\tjmp " << after_else_label.str()
                << "; after the truth-y block's statements, skip to after the else(false-y) block's statements"
                << std::endl;  // skip else section
    m_generated << after_if_label.str() << ":" << std::endl;
    generate_statement(*fail_statement);
    m_generated << after_else_label.str() << ":" << std::endl;
}

void Generator::generate_statement_while(const std::shared_ptr<ASTStatementWhile>& while_statement) {
    std::stringstream before_while_label;
    before_while_label << ".before_while_statement_" << m_condition_counter;
    std::stringstream after_while_label;
    after_while_label << ".after_while_statement_" << m_condition_counter;
    ++m_condition_counter;

    auto& expression = while_statement->expression;
    auto& success_statement = while_statement->success_statement;
    size_t size_bytes = expression.data_type->get_size_bytes();

    m_generated << before_while_label.str() << ":" << std::endl;
    generate_expression(expression);
    pop_stack_register("rax", 8, size_bytes);  // rax = lhs
    m_generated << "\ttest rax, rax" << std::endl
                << "\tjz " << after_while_label.str()
                << "; if the expression is false-y, skip the 'while' block's statements" << std::endl;
    generate_statement(*success_statement);
    m_generated << "\tjmp " << before_while_label.str()
                << "; after the 'while' block ends, jump back to the condition checking" << std::endl;
    m_generated << after_while_label.str() << ":" << std::endl;
}
