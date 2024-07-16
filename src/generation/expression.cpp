#include "generator.hpp"
#include "generator_visitor.hpp"

// --------- expression generation

void Generator::generate_expression(const ASTExpression& expression) {
    bool type_provided = (expression.data_type != nullptr);
    assert(type_provided && "Expression found with no data type. expression index- " + expression.expression.index());

    size_t size_bytes = expression.data_type->get_size_bytes();
    std::visit(Generator::ExpressionVisitor{.generator = *this, .size = size_bytes}, expression.expression);
}

void Generator::generate_expression_identifier(const ASTIdentifier& identifier, size_t requested_size_bytes) {
    auto& variable_name = identifier.value;
    auto variable_data = assert_get_variable_data(variable_name);

    m_generated << ";\tEvaluate Variable " << variable_name << std::endl;

    bool is_pointer_type = (bool)dynamic_cast<PointerType*>(variable_data.data_type.get());
    bool is_base_type = (bool)dynamic_cast<BasicType*>(variable_data.data_type.get());
    bool is_complex_type = (!is_base_type && !is_pointer_type);  // complex types are having their pointers copied

    load_memory_address_var(identifier);
    pop_stack_register("rdx", 8, 8);  // address is always 8 bytes

    size_t data_size = is_complex_type ? 8 : variable_data.size_bytes;  // if complex type, we only assign the address

    // NOTE: assumes that if theres type-changing, it is narrowing
    // to handle type-widening, we need to first clear the entire a register before writing to it,
    // and then zero/sign filling it with the data we read from the stack.
    std::string original_size_keyword = size_bytes_to_size_keyword.at(data_size);
    std::string original_data_reg = size_bytes_to_register.at(data_size);
    std::string requested_data_reg = size_bytes_to_register.at(requested_size_bytes);

    if (is_complex_type) {
        // copy reference- pointer decay
        m_generated << "\t; RESOLVE ADDRESS OF VARIABLE" << std::endl;
        m_generated << "\tmov " << original_data_reg << ", " << original_size_keyword << " rdx" << std::endl;
    } else {
        m_generated << "\t; RESOLVE VALUE OF VARIABLE" << std::endl;
        m_generated << "\tmov " << original_data_reg << ", " << original_size_keyword << " [rdx]" << std::endl;
    }

    // NOTE: if reading a singular byte, we need to byteswap the read data(little endian shenanigans)
    // probably just don't support 8-bit variables, lol
    push_stack_register(requested_data_reg, requested_size_bytes);
}

void Generator::generate_expression_int_literal(const ASTIntLiteral& literal, size_t size_bytes) {
    push_stack_literal(literal.value, size_bytes);
}

void Generator::generate_expression_char_literal(const ASTCharLiteral& literal, size_t size_bytes) {
    auto ascii_value = std::to_string(literal.value);
    push_stack_literal(ascii_value, size_bytes);
}

void Generator::generate_expression_array_initializer(const ASTArrayInitializer& array_initializer) {
    // push them in reverse(stack reverses order)
    for (auto cur = array_initializer.initialize_values.rbegin(); cur != array_initializer.initialize_values.rend();
         ++cur) {
        generate_expression(*cur);
    }
}

void Generator::generate_expression_binary(const std::shared_ptr<ASTBinExpression>& binary, size_t size_bytes) {
    static_assert((int)BinOperation::operationCount - 1 == 10,
                  "Binary Operations enum changed without changing generator");
    std::string operation;
    switch (binary->operation) {
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
    auto& lhsExp = binary->lhs;
    auto& rhsExp = binary->rhs;
    size_t rhs_size_bytes = rhsExp->data_type->get_size_bytes();
    size_t lhs_size_bytes = lhsExp->data_type->get_size_bytes();
    generate_expression(*lhsExp);
    generate_expression(*rhsExp);
    pop_stack_register("rbx", 8, rhs_size_bytes);  // rbx = rhs
    pop_stack_register("rax", 8, lhs_size_bytes);  // rax = lhs

    auto bin_operation = binary->operation;
    switch (bin_operation) {
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
        case BinOperation::lt:
        case BinOperation::le:
        case BinOperation::gt:
        case BinOperation::ge:
            m_generated << "\tcmp rax, rbx" << std::endl;
            m_generated << "\t" << comparison_operation.at(bin_operation) << " bl; temporary in bl" << std::endl;
            m_generated << "\txor rax, rax" << std::endl;
            m_generated << "\tmov al, bl" << std::endl;  // final result is stored in rax by convention
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

void Generator::generate_expression_unary(const std::shared_ptr<ASTUnaryExpression>& unary, size_t size_bytes) {
    static_assert((int)UnaryOperation::operationCount - 1 == 2,
                  "Implemented unary operations without updating generator");

    auto& operand = *unary->expression;
    auto operation = unary->operation;

    switch (operation) {
        case UnaryOperation::negate: {
            size_t operand_size_bytes = operand.data_type->get_size_bytes();
            generate_expression(operand);
            pop_stack_register("rax", 8, operand_size_bytes);
            m_generated << "\tneg rax; Negate expression" << std::endl;
            break;
        }
        case UnaryOperation::dereference: {
            load_memory_address_expr(operand);
            pop_stack_register("rax", 8, 8);  // pointers are always qword
            break;
        }
        default:
            assert(false && "Unknown unary operation");
    }
    auto& reg = size_bytes_to_register.at(size_bytes);
    push_stack_register(reg, size_bytes);
}

void Generator::generate_expression_array_index(const std::shared_ptr<ASTArrayIndexExpression>& array_index,
                                                size_t requested_size_bytes) {
    auto array_type = dynamic_cast<ArrayType*>(array_index->expression->data_type.get());
    auto pointer_type = dynamic_cast<PointerType*>(array_index->expression->data_type.get());
    if (!pointer_type && !array_type) {
        std::cerr << "Generation: unexpected expression to index" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto& inner_element = array_type ? array_type->elementType : pointer_type->baseType;
    auto inner_type_size_bytes = inner_element->get_size_bytes();

    // NOTE: very similar to variable evaluation
    std::string original_size_keyword = size_bytes_to_size_keyword.at(inner_type_size_bytes);
    std::string original_data_reg = size_bytes_to_register.at(inner_type_size_bytes);
    std::string requested_data_reg = size_bytes_to_register.at(requested_size_bytes);

    size_t index_size_bytes = array_index->index->data_type->get_size_bytes();

    if (array_type) {
        load_memory_address_expr(*array_index->expression);
    } else {
        generate_expression(*array_index->expression);
    }

    generate_expression(*array_index->index);

    pop_stack_register("rax", 8, index_size_bytes);  // index is in rax
    pop_stack_register("rcx", 8, 8);                 // offset is in rcx

    m_generated << "\tmov rbx, " << inner_type_size_bytes << std::endl << "\tmul rbx" << std::endl;
    m_generated << "\tadd rcx, rax ; Indexing offset" << std::endl;  // indexing

    m_generated << "\tmov " << original_data_reg << ", " << original_size_keyword << " [rcx]" << std::endl;

    push_stack_register(requested_data_reg, requested_size_bytes);
}

void Generator::load_memory_address_expr(const ASTExpression& expression) {
    if (std::holds_alternative<std::shared_ptr<ASTArrayIndexExpression>>(expression.expression)) {
        m_generated << "\t; Evaluate array index memory address BEGIN" << std::endl;
        auto array_index = std::get<std::shared_ptr<ASTArrayIndexExpression>>(expression.expression);
        auto array_type = dynamic_cast<ArrayType*>(array_index->expression->data_type.get());
        auto inner_type_size_bytes = array_type->elementType->get_size_bytes();

        load_memory_address_expr(*array_index->expression);
        pop_stack_register("rcx", 8, 8);

        // indexing
        m_generated << "\t; Begin Array Index generation" << std::endl;
        generate_expression(*array_index->index);
        pop_stack_register("rax", 8, 8);
        m_generated << "\tmov rbx, " << inner_type_size_bytes << std::endl << "\tmul rbx" << std::endl;
        m_generated << "\t; End Array Index generation" << std::endl;

        m_generated << "\tadd rcx, rax ; Indexing offset" << std::endl;  // indexing
        push_stack_register("rcx", 8);
        m_generated << "\t; Evaluate array index memory address END" << std::endl;
        return;
    }

    // must be atomic expression
    if (!std::holds_alternative<std::shared_ptr<ASTAtomicExpression>>(expression.expression)) {
        std::cerr << "Generation: unexpected expression to calculate address of" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto atomic = std::get<std::shared_ptr<ASTAtomicExpression>>(expression.expression);
    if (std::holds_alternative<ASTIdentifier>(atomic->value)) {
        auto identifier = std::get<ASTIdentifier>(atomic->value);
        load_memory_address_var(identifier);
        return;
    }
}