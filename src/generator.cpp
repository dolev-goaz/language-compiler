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

    m_stack.enterScope();
    for (size_t i = 0; i < m_prog.statements.size(); ++i) {
        auto current = m_prog.statements[i].get();
        generate_statement(*current);
    }
    m_stack.exitScope();

    // default exit statement
    m_generated << std::endl << "; default program end" << std::endl;
    m_generated << "\tmov rax, 60" << std::endl;
    m_generated << "\tmov rdi, 0; status code 0- OK" << std::endl;
    m_generated << "\tsyscall";
    return m_generated.str();
}

void Generator::push_stack_literal(const std::string& value, size_t size) {
    std::string size_keyword = size_bytes_to_size_keyword.at(size);
    m_generated << "\tpush " << size_keyword << " " << value << std::endl;
    m_stack_size += size;
}
void Generator::push_stack_offset(int offset, size_t data_size, size_t requested_size) {
    // reading a value from the an arbitrary point in the stack, then pushing that value
    // to the top of the stack.

    // NOTE: assumes that if theres type-changing, it is narrowing
    // to handle type-widening, we need to first clear the entire a register before writing to it,
    // and then zero/sign filling it with the data we read from the stack.
    std::string original_size_keyword = size_bytes_to_size_keyword.at(data_size);
    std::string original_data_reg = size_bytes_to_register.at(data_size);
    std::string requested_data_reg = size_bytes_to_register.at(requested_size);

    m_generated << "\tmov " << original_data_reg << ", " << original_size_keyword << " [rsp + " << offset << "]"
                << std::endl;

    // NOTE: if reading a singular byte, we need to byteswap the read data(little endian shenanigans)
    // probably just don't support 8-bit variables, lol
    m_generated << "\tpush " << requested_data_reg << std::endl;
    m_stack_size += requested_size;
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

void Generator::generate_expression_identifier(const ASTIdentifier& identifier, size_t size_bytes) {
    Generator::Variable variableData;
    if (!m_stack.lookup(identifier.value, variableData)) {
        std::cerr << "Variable '" << identifier.value << "' does not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    m_generated << ";\tEvaluate Variable " << identifier.value << std::endl;

    // get variable metadata

    // get variable position in the stack
    int offset = m_stack_size - variableData.stack_location_bytes;
    // rsp was on the next FREE address, offset it back to the variable's position.
    offset -= variableData.size_bytes;

    // push variable value into stack
    push_stack_offset(offset, variableData.size_bytes, size_bytes);
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
    generate_expression(lhsExp);
    generate_expression(rhsExp);
    pop_stack_register("rbx", size_bytes);  // rbx = rhs
    pop_stack_register("rax", size_bytes);  // rax = lhs
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
    push_stack_register("rax", size_bytes);
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