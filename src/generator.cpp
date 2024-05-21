#include "../header/generator.hpp"

#include "../header/generator_visitor.hpp"

std::map<DataType, size_t> data_type_size_bytes = {
    {DataType::NONE, 0},
    {DataType::int_16, 2},
    // {DataType::int_32, 4},
    {DataType::int_64, 8},
};

std::map<size_t, std::string> size_bytes_to_size_keyword = {
    {2, "WORD"},
    {8, "QWORD"},
};

std::map<size_t, std::string> size_bytes_to_register = {
    {2, "ax"},
    // {4, "eax"},
    {8, "rax"},
};

std::string Generator::generate_program() {
    m_generated << "global main" << std::endl << "main:" << std::endl;
    m_generated << "\tmov rbp, rsp" << std::endl << std::endl;

    m_stack.enterScope();
    // generate all statements
    // TODO: store initial rsp, to allow access to global variables(outside functions)
    for (size_t i = 0; i < m_prog.statements.size(); ++i) {
        auto current = m_prog.statements[i].get();
        generate_statement(*current);
    }

    // default exit statement
    m_generated << std::endl << "; default program end" << std::endl;
    m_generated << "\tmov rax, 60" << std::endl;
    m_generated << "\tmov rdi, 0; status code 0- OK" << std::endl;
    m_generated << "\tsyscall";

    // generate all functions at the end of the file
    for (size_t i = 0; i < m_prog.functions.size(); ++i) {
        auto current = m_prog.functions[i].get();
        generate_statement_function(*current);
    }
    m_stack.exitScope();

    return m_generated.str();
}

void Generator::push_stack_literal(const std::string& value, size_t size) {
    std::string size_keyword = size_bytes_to_size_keyword.at(size);
    m_generated << "\tpush " << size_keyword << " " << value << std::endl;
    m_stack_size += size;
}

void Generator::push_stack_register(const std::string& reg, size_t size) {
    m_generated << "\tpush " << reg << std::endl;
    m_stack_size += size;
}
void Generator::pop_stack_register(const std::string& reg, size_t size) {
    if (size == 8) {
        m_generated << "\tpop " << reg << std::endl;
    } else {
        std::string size_keyword = size_bytes_to_size_keyword.at(size);
        // popping non-qword from the stack. so we read from the stack(0-filled)
        // and then update the stack pointer, effectively manually popping from the stack
        m_generated << ";\tManual POP BEGIN" << std::endl;
        m_generated << "\tmovsx " << reg << ", " << size_keyword << " " << "[rsp]" << std::endl;
        m_generated << "\tadd rsp, " << size << std::endl;
        m_generated << ";\tManual POP END" << std::endl;
    }
    m_stack_size -= size;
}

// --------- expression generation

void Generator::generate_expression(const ASTExpression& expression) {
    bool type_provided = expression.data_type != DataType::NONE && data_type_size_bytes.count(expression.data_type) > 0;
    assert(type_provided && "Expression found with no data type. expression index- " + expression.expression.index());

    size_t size_bytes = data_type_size_bytes.at(expression.data_type);
    std::visit(Generator::ExpressionVisitor{.generator = *this, .size = size_bytes}, expression.expression);
}

void Generator::generate_expression_identifier(const ASTIdentifier& identifier, size_t requested_size_bytes) {
    auto& variable_name = identifier.value;
    auto variable_data = assert_get_variable_data(variable_name);

    m_generated << ";\tEvaluate Variable " << variable_name << std::endl;

    // NOTE: assumes that if theres type-changing, it is narrowing
    // to handle type-widening, we need to first clear the entire a register before writing to it,
    // and then zero/sign filling it with the data we read from the stack.
    std::string original_size_keyword = size_bytes_to_size_keyword.at(variable_data.size_bytes);
    std::string original_data_reg = size_bytes_to_register.at(variable_data.size_bytes);
    std::string requested_data_reg = size_bytes_to_register.at(requested_size_bytes);

    m_generated << "\tmov " << original_data_reg << ", " << original_size_keyword << " "
                << get_variable_memory_position(variable_name) << std::endl;

    // NOTE: if reading a singular byte, we need to byteswap the read data(little endian shenanigans)
    // probably just don't support 8-bit variables, lol
    m_generated << "\tpush " << requested_data_reg << std::endl;
    m_stack_size += requested_size_bytes;
}

void Generator::generate_expression_int_literal(const ASTIntLiteral& literal, size_t size_bytes) {
    push_stack_literal(literal.value, size_bytes);
}

void Generator::generate_expression_binary(const std::shared_ptr<ASTBinExpression>& binary, size_t size_bytes) {
    static_assert((int)BinOperation::operationCount - 1 == 5,
                  "Binary Operations enum changed without changing generator");
    std::string operation;
    switch (binary.get()->operation) {
        case BinOperation::add:
            operation = "Addition";
            break;
        case BinOperation::subtract:
            operation = "Subtraction";
            break;
        case BinOperation::multiply:
            operation = "Multiplication";
            break;
        case BinOperation::divide:
            operation = "Division";
            break;
        case BinOperation::modulo:
            operation = "Modulo";
            break;
        default:
            // should never reach here. this is to remove warnings
            std::cerr << "Generation: unknown binary operation" << std::endl;
            exit(EXIT_FAILURE);
    }
    m_generated << ";\t" << operation << " Evaluation BEGIN" << std::endl;
    auto& lhsExp = *binary.get()->lhs.get();
    auto& rhsExp = *binary.get()->rhs.get();
    size_t rhs_size_bytes = data_type_size_bytes.at(rhsExp.data_type);
    size_t lhs_size_bytes = data_type_size_bytes.at(lhsExp.data_type);
    generate_expression(lhsExp);
    generate_expression(rhsExp);
    pop_stack_register("rbx", rhs_size_bytes);  // rbx = rhs
    pop_stack_register("rax", lhs_size_bytes);  // rax = lhs
    switch (binary.get()->operation) {
        case BinOperation::add:
            m_generated << "\tadd rax, rbx; rax += rbx" << std::endl;  // rax = rax + rbx
            break;
        case BinOperation::subtract:
            m_generated << "\tsub rax, rbx; rax -= rbx" << std::endl;  // rax = rax - rbx
            break;
        case BinOperation::multiply:
            m_generated << "\tmul rbx; rax *= rbx" << std::endl;  // rax = rax * rbx
            break;
        case BinOperation::divide:
            m_generated << "\txor rdx, rdx; clear rdx" << std::endl;       // clear rdx(division is rdx:rax / rbx)
            m_generated << "\tdiv rbx; rax = rdx:rax / rbx" << std::endl;  // rax = rax / rbx
            break;
        case BinOperation::modulo:
            m_generated << "\txor rdx, rdx; clear rdx" << std::endl;       // clear rdx(division is rdx:rax / rbx)
            m_generated << "\tdiv rbx; rdx = rdx:rax % rbx" << std::endl;  // rdx stores the remainder
            m_generated << "\tmov rax, rdx; rdx stores the remainder" << std::endl;
            break;
        default:
            // should never reach here. this is to remove warnings
            std::cerr << "Generation: unknown binary operation" << std::endl;
            exit(EXIT_FAILURE);
    }
    auto& reg = size_bytes_to_register.at(size_bytes);
    push_stack_register(reg, size_bytes);
    m_generated << ";\t" << operation << " Evaluation END" << std::endl << std::endl;
}

// --------- statement generation

void Generator::generate_statement(const ASTStatement& statement) {
    std::visit(Generator::StatementVisitor{*this}, statement.statement);
}
void Generator::generate_statement_exit(const ASTStatementExit& exit_statement) {
    generate_expression(exit_statement.status_code);
    m_generated << ";\tExit Statement" << std::endl;
    m_generated << "\tmov rax, 60" << std::endl;

    size_t size = data_type_size_bytes.at(exit_statement.status_code.data_type);
    pop_stack_register("rdi", size);
    m_generated << "\tsyscall" << std::endl;
}

void Generator::generate_statement_var_assignment(const ASTStatementAssign& var_assign_statement) {
    Generator::Variable variableData = assert_get_variable_data(var_assign_statement.name);

    m_generated << ";\tVariable Assigment " << var_assign_statement.name << " BEGIN" << std::endl;

    generate_expression(var_assign_statement.value);
    auto& expression_size_bytes = data_type_size_bytes.at(var_assign_statement.value.data_type);

    // NOTE: assumes 'size_bytes_to_register' holds registers rax, eax, ax, al
    // popping to the largest register to allow data widening if necessary, up to 8 bytes
    pop_stack_register("rax", expression_size_bytes);
    std::string& temp_register = size_bytes_to_register.at(variableData.size_bytes);
    m_generated << "\tmov " << get_variable_memory_position(var_assign_statement.name) << ", " << temp_register
                << std::endl;
    m_generated << ";\tVariable Assigment " << var_assign_statement.name << " END" << std::endl << std::endl;
}

void Generator::generate_statement_var_declare(const ASTStatementVar& var_statement) {
    // no need to check for duplicate variable names, since it was checked in semantic analysis
    size_t size_bytes = data_type_size_bytes.at(var_statement.data_type);

    Generator::Variable var = {
        .stack_location_bytes = m_stack_size,
        .size_bytes = size_bytes,
    };
    m_generated << ";\tVariable Declaration " << var_statement.name << " BEGIN" << std::endl;
    generate_expression(var_statement.value.value());

    m_stack.insert(var_statement.name, var);
    m_generated << ";\tVariable Declaration " << var_statement.name << " END" << std::endl << std::endl;
}

void Generator::enter_scope() { m_stack.enterScope(); }
void Generator::exit_scope() {
    std::optional<std::map<std::string, Generator::Variable>> scope = m_stack.exitScope();
    if (!scope.has_value()) {
        // should never happen
        std::cerr << "exited a non-existing scope" << std::endl;
        exit(EXIT_FAILURE);
    }
    int totalStackSpace = 0;
    for (auto& pair : scope.value()) {
        totalStackSpace += pair.second.size_bytes;
    }
    m_stack_size -= totalStackSpace;
    m_generated << "\tadd rsp, " << totalStackSpace << "; END OF SCOPE" << std::endl;
}

void Generator::generate_statement_scope(const ASTStatementScope& scope_statement) {
    enter_scope();
    for (auto& statement : scope_statement.statements) {
        this->generate_statement(*statement.get());
    }
    exit_scope();
}

void Generator::generate_statement_if(const ASTStatementIf& if_statement) {
    std::stringstream after_if_label;
    after_if_label << ".after_if_statement_" << m_condition_counter;
    std::stringstream after_else_label;
    after_else_label << ".after_else_statement_" << m_condition_counter;
    ++m_condition_counter;

    auto& expression = if_statement.expression;
    auto& success_statement = if_statement.success_statement;
    auto& fail_statement = if_statement.fail_statement;
    size_t size_bytes = data_type_size_bytes.at(expression.data_type);
    generate_expression(expression);
    pop_stack_register("rax", size_bytes);  // rax = lhs
    m_generated << "\ttest rax, rax" << std::endl
                << "\tjz " << after_if_label.str() << "; if the expression is false-y, skip the 'if' block's statements"
                << std::endl;
    generate_statement(*success_statement.get());
    if (!fail_statement) {
        // no 'else' statement
        m_generated << after_if_label.str() << ":" << std::endl;
        return;
    }
    m_generated << "\tjmp " << after_else_label.str()
                << "; after the truth-y block's statements, skip to after the else(false-y) block's statements"
                << std::endl;  // skip else section
    m_generated << after_if_label.str() << ":" << std::endl;
    generate_statement(*fail_statement.get());
    m_generated << after_else_label.str() << ":" << std::endl;
}

void Generator::generate_statement_while(const ASTStatementWhile& while_statement) {
    std::stringstream before_while_label;
    before_while_label << ".before_while_statement_" << m_condition_counter;
    std::stringstream after_while_label;
    after_while_label << ".after_while_statement_" << m_condition_counter;
    ++m_condition_counter;

    auto& expression = while_statement.expression;
    auto& success_statement = while_statement.success_statement;
    size_t size_bytes = data_type_size_bytes.at(expression.data_type);

    m_generated << before_while_label.str() << ":" << std::endl;
    generate_expression(expression);
    pop_stack_register("rax", size_bytes);  // rax = lhs
    m_generated << "\ttest rax, rax" << std::endl
                << "\tjz " << after_while_label.str()
                << "; if the expression is false-y, skip the 'while' block's statements" << std::endl;
    generate_statement(*success_statement.get());
    m_generated << "\tjmp " << before_while_label.str()
                << "; after the 'while' block ends, jump back to the condition checking" << std::endl;
    m_generated << after_while_label.str() << ":" << std::endl;
}

void Generator::generate_statement_function(const ASTStatementFunction& function_statement) {
    m_stack.enterScope();
    // add function parameters to scope
    for (auto& func_param : function_statement.parameters) {
        size_t size_bytes = data_type_size_bytes.at(func_param.data_type);
        Generator::Variable var{
            .stack_location_bytes = m_stack_size,
            .size_bytes = size_bytes,
        };
        m_stack.insert(func_param.name, var);
        m_stack_size += size_bytes;
    }
    m_stack_size += 8;  // return address pushed by 'call'

    m_generated << std::endl << "; BEGIN OF FUNCTION '" << function_statement.name << "'" << std::endl;
    m_generated << function_statement.name << ":" << std::endl;
    generate_statement(*function_statement.statement.get());
    m_generated << "\tret" << std::endl;
    m_generated << "; END OF FUNCTION '" << function_statement.name << "'" << std::endl << std::endl;

    m_stack_size -= 8;  // return address popped by 'ret'
    m_stack.exitScope();
}

void Generator::generate_statement_function_call(const ASTStatementFunctionCall& function_call_statement) {
    // push parameters to the stack before calling
    size_t total_function_params_size = 0;
    for (auto& func_param : function_call_statement.parameters) {
        generate_expression(func_param);
        total_function_params_size += data_type_size_bytes.at(func_param.data_type);
    };

    m_generated << "\tcall " << function_call_statement.function_name << std::endl;
    m_generated << "\tadd rsp, " << total_function_params_size << std::endl;  // clear stack params
    m_stack_size -= total_function_params_size;
}

std::string Generator::get_variable_memory_position(const std::string& variable_name) {
    auto variable_data = assert_get_variable_data(variable_name);

    bool is_global = m_stack.is_variable_global(variable_name);

    int offset = variable_data.stack_location_bytes + variable_data.size_bytes;

    std::stringstream out;
    if (is_global) {
        out << "[" << "rbp" << " - " << offset << "]";
    } else {
        offset = m_stack_size - offset;
        out << "[" << "rsp" << " + " << offset << "]";
    }
    return out.str();
}

Generator::Variable Generator::assert_get_variable_data(std::string variable_name) {
    Generator::Variable variableData;
    if (!m_stack.lookup(variable_name, variableData)) {
        std::cerr << "Variable '" << variable_name << "' does not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return variableData;
}