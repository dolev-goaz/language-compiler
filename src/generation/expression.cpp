#include "generator.hpp"
#include "generator_visitor.hpp"

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

void Generator::generate_expression_char_literal(const ASTCharLiteral& literal, size_t size_bytes) {
    auto ascii_value = std::to_string(literal.value);
    push_stack_literal(ascii_value, size_bytes);
}


void Generator::generate_expression_binary(const std::shared_ptr<ASTBinExpression>& binary, size_t size_bytes) {
    static_assert((int)BinOperation::operationCount - 1 == 10,
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
        case BinOperation::eq:
            operation = "Comparison(=)";
            break;
        case BinOperation::lt:
            operation = "Comparison(<)";
            break;
        case BinOperation::le:
            operation = "Comparison(<=)";
            break;
        case BinOperation::gt:
            operation = "Comparison(>)";
            break;
        case BinOperation::ge:
            operation = "Comparison(>=)";
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
    pop_stack_register("rbx", 8, rhs_size_bytes);  // rbx = rhs
    pop_stack_register("rax", 8, lhs_size_bytes);  // rax = lhs

    std::string _8bit_reg = size_bytes_to_register.at(1);
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
        case BinOperation::eq:
            m_generated << "\tcmp rax, rbx" << std::endl;
            m_generated << "\tsete " << _8bit_reg << std::endl;
            break;
        case BinOperation::lt:
            m_generated << "\tcmp rax, rbx" << std::endl;
            m_generated << "\tsetl " << _8bit_reg << std::endl;
            break;
        case BinOperation::le:
            m_generated << "\tcmp rax, rbx" << std::endl;
            m_generated << "\tsetle " << _8bit_reg << std::endl;
            break;
        case BinOperation::gt:
            m_generated << "\tcmp rax, rbx" << std::endl;
            m_generated << "\tsetg " << _8bit_reg << std::endl;
            break;
        case BinOperation::ge:
            m_generated << "\tcmp rax, rbx" << std::endl;
            m_generated << "\tsetge " << _8bit_reg << std::endl;
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
