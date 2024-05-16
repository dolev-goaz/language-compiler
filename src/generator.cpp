#include "../header/generator.hpp"

#include "../header/generator_visitor.hpp"

std::map<DataType, size_t> data_type_size_bytes = {
    {DataType::int_16, 2},
    {DataType::int_64, 8},
    {DataType::NONE, 0},
};

std::map<size_t, std::string> size_bytes_to_size_keyword = {
    {2, "WORD"},
    {8, "QWORD"},
};

std::string Generator::generate_program() {
    m_generated << "global main" << std::endl << "main:" << std::endl;

    for (size_t i = 0; i < m_prog.statements.size(); ++i) {
        auto current = m_prog.statements[i].get();
        generate_statement(*current);
    }

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
void Generator::push_stack_offset(int offset, size_t size) {
    // reading a value from the an arbitrary point in the stack, then pushing that value
    // to the top of the stack.
    if (size == 8) {
        m_generated << "\tmov rax, [rsp + " << offset << "]" << std::endl;
    } else {
        std::string size_keyword = size_bytes_to_size_keyword.at(size);
        m_generated << "\tmovsx rax, " << size_keyword << " [rsp + " << offset << "]" << std::endl;
    }

    if ((m_stack_size + offset) % 2 != 0) {
        m_generated << "\tbswap rax" << std::endl;  // little endian shenanigans when reading inside a word
    }
    m_generated << "\tpush rax" << std::endl;
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
        m_generated << "\tmovsx " << reg << ", " << size_keyword << " " << "[rsp]" << std::endl;
        m_generated << "\tadd rsp, " << size << std::endl;
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
    (void)size_bytes;  // ignore unused
    if (m_variables.count(identifier.value) == 0) {
        std::cerr << "Variable '" << identifier.value << "' does not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    m_generated << ";\tEvaluate Variable " << identifier.value << std::endl;

    // get variable metadata
    Generator::Variable variable = m_variables.at(identifier.value);

    // get variable position in the stack
    int offset = m_stack_size - variable.stack_location_bytes;
    // rsp was on the next FREE address, offset it back to the variable's position.
    offset -= variable.size_bytes;

    // push variable value into stack
    push_stack_offset(offset, variable.size_bytes);
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
    // TODO: make sure size_bytes is ok as parameter for all those
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
            // TODO: doesn't work well with 16bit
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

    pop_stack_register("rdi", 8);
    m_generated << "\tsyscall" << std::endl;
}

void Generator::generate_statement_var_declare(const ASTStatementVar& var_statement) {
    // check variable name already exists
    if (m_variables.count(var_statement.name) > 0) {
        std::cerr << "Variable '" << var_statement.name << "' already exists!" << std::endl;
        exit(EXIT_FAILURE);
    }

    size_t size_bytes = data_type_size_bytes.at(var_statement.data_type);

    Generator::Variable var = {
        .stack_location_bytes = m_stack_size,
        .size_bytes = size_bytes,
    };
    m_generated << ";\tVariable Declaration " << var_statement.name << " BEGIN" << std::endl;
    generate_expression(var_statement.value.value());

    m_variables.insert({var_statement.name, var});  // variable is now set
    m_generated << ";\tVariable Declaration " << var_statement.name << " END" << std::endl << std::endl;
}

void Generator::generate_statement_scope(const ASTStatementScope& scope_statement) {
    // TODO: handle begin of scope(setup)
    for (auto& statement : scope_statement.statements) {
        this->generate_statement(*statement.get());
    }
    // TODO: handle end of scope(cleanup)
}