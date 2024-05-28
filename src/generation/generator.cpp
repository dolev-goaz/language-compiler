#include "../header/generator.hpp"

#include "../header/generator_visitor.hpp"

std::map<DataType, size_t> data_type_size_bytes = {
    {DataType::NONE, 0},
    {DataType::_void, 0},
    {DataType::int_16, 2},
    // {DataType::int_32, 4},
    {DataType::int_64, 8},
};

std::map<size_t, std::string> size_bytes_to_size_keyword = {
    {2, "WORD"},
    {8, "QWORD"},
};

std::map<size_t, std::string> size_bytes_to_register = {
    {1, "al"},
    {2, "ax"},
    // {4, "eax"},
    {8, "rax"},
};

std::map<BinOperation, std::string> comparison_operation = {
    {BinOperation::eq, "sete"},
    {BinOperation::lt, "setl"},
    {BinOperation::le, "setle"},
    {BinOperation::gt, "setg"},
    {BinOperation::ge, "setge"},
};

std::string Generator::generate_program() {
    m_generated << "global main" << std::endl << "main:" << std::endl;
    m_generated << "\tmov rbp, rsp" << std::endl << std::endl;

    m_stack.enterScope();
    // generate all statements
    for (size_t i = 0; i < m_prog.statements.size(); ++i) {
        auto current = m_prog.statements[i].get();
        generate_statement(*current);
    }

    // default exit statement
    m_generated << std::endl << "; default program end" << std::endl;
    m_generated << "\tmov rax, 60" << std::endl;
    m_generated << "\tmov rdi, 0; status code 0- OK" << std::endl;
    m_generated << "\tsyscall" << std::endl;

    // generate all functions at the end of the file
    for (size_t i = 0; i < m_prog.functions.size(); ++i) {
        auto current = m_prog.functions[i].get();
        generate_statement_function(*current);
    }
    m_stack.exitScope();

    return m_generated.str();
}