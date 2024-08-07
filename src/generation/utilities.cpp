#include "generator.hpp"

void Generator::load_memory_address_var(const ASTIdentifier& identifier) {
    auto& variable_name = identifier.value;
    auto variable_data = assert_get_variable_data(variable_name);

    bool is_global = m_stack.is_variable_global(variable_name);

    int offset = variable_data.stack_location_bytes + variable_data.size_bytes;

    m_generated << "\t; Load Memory Address Of " << variable_name << std::endl;
    if (is_global) {
        m_generated << "\tmov r11, " << "[global_variables_base]" << std::endl;
        m_generated << "\tsub r11, " << offset << std::endl;
        ;
    } else {
        offset = m_stack_size - offset;
        m_generated << "\tmov r11, " << "rsp" << std::endl;
        m_generated << "\tadd r11, " << offset << std::endl;
    }
    push_stack_register("r11", 8);
    m_generated << "\t; End Load Memory Address Of " << variable_name << std::endl;
}

Generator::Variable Generator::assert_get_variable_data(std::string variable_name) {
    Generator::Variable* variableData = nullptr;
    if (!m_stack.lookup(variable_name, &variableData)) {
        std::cerr << "Variable '" << variable_name << "' does not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return *variableData;
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

void Generator::push_stack_literal(const std::string& value, size_t size) {
    if (size == 1) {
        m_generated << "\tsub rsp, 1" << std::endl << "\tmov byte [rsp], " << value << std::endl;
    } else {
        std::string size_keyword = size_bytes_to_size_keyword.at(size);
        m_generated << "\tpush " << size_keyword << " " << value << std::endl;
    }
    m_stack_size += size;
}

void Generator::push_stack_register(const std::string& reg, size_t size) {
    if (size == 1) {
        m_generated << "\tsub rsp, 1" << std::endl << "\tmov byte [rsp], " << reg << std::endl;
    } else {
        m_generated << "\tpush " << reg << std::endl;
    }
    m_stack_size += size;
}

void Generator::pop_stack_register(const std::string& reg, size_t register_size, size_t requested_size) {
    if (requested_size == register_size) {
        m_generated << "\tpop " << reg << std::endl;
    } else {
        std::string size_keyword = size_bytes_to_size_keyword.at(requested_size);
        // popping non-qword from the stack. so we read from the stack(0-filled)
        // and then update the stack pointer, effectively manually popping from the stack
        m_generated << ";\tManual POP BEGIN" << std::endl;
        m_generated << "\tmovzx " << reg << ", " << size_keyword << " " << "[rsp]" << std::endl;
        m_generated << "\tadd rsp, " << requested_size << std::endl;
        m_generated << ";\tManual POP END" << std::endl;
    }
    m_stack_size -= requested_size;
}
