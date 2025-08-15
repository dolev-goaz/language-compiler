#include "generator.hpp"

void Generator::generate_statement_function(const std::shared_ptr<ASTStatementFunction>& function_statement) {
    m_stack.enterScope();
    // add function parameters to scope
    for (auto& func_param : function_statement->parameters) {
        size_t size_bytes = func_param.data_type->get_size_bytes();
        Generator::Variable var{
            .stack_location_bytes = m_stack_size,
            .size_bytes = size_bytes,
            .data_type = func_param.data_type,
        };
        m_stack.insert(func_param.name, var);
        m_stack_size += size_bytes;
    }
    m_stack_size += 8;  // return address pushed by 'call'

    m_generated << std::endl << "; BEGIN OF FUNCTION '" << function_statement->name << "'" << std::endl;
    m_generated << function_statement->name << ":" << std::endl;
    push_stack_register("rbp", 8);                 // store the previous stack frame
    m_generated << "\tmov rbp, rsp" << std::endl;  // this is the current stack frame
    generate_statement(*function_statement->statement);
    m_generated << ".return:" << std::endl;
    m_generated << "\tmov rsp, rbp" << std::endl;  // return the stack to its previous state
    pop_stack_register("rbp", 8, 8);               // restore the previous stack frame
    m_generated << "\tret" << std::endl;
    m_generated << "; END OF FUNCTION '" << function_statement->name << "'" << std::endl << std::endl;

    m_stack_size -= 8;  // return address popped by 'ret'
    m_stack.exitScope();
}

void Generator::generate_statement_return(const std::shared_ptr<ASTStatementReturn>& return_statement) {
    m_generated << "; BEGIN RETURN STATEMENT" << std::endl;
    if (return_statement->expression.has_value()) {
        auto& expression = return_statement->expression.value();
        generate_expression(expression);
        size_t return_size_bytes = expression.data_type->get_size_bytes();
        if (return_size_bytes > 0) {
            auto& reg = size_bytes_to_register.at(return_size_bytes);

            // NOTE: this only works for primitives for now
            pop_stack_register(reg, return_size_bytes, return_size_bytes);
            m_generated << "\tmov [rdi], " << reg << std::endl;
        }
    }
    m_generated << "\tjmp .return" << std::endl;
    m_generated << "; END RETURN STATEMENT" << std::endl;
}

void Generator::generate_expression_function_call(const ASTFunctionCall& function_call_expr, size_t return_size_bytes) {
    size_t return_type_size = function_call_expr.return_data_type->get_size_bytes();
    if (return_type_size) {
        m_generated << "; BEGIN PREPARE RETURN LOCATION INTO RDI" << std::endl;
        push_stack_register("rdi", 8);
        m_generated << "\tsub rsp, " << return_type_size << std::endl;  // allocate stack for return param
        m_generated << "\tlea rdi, [rsp]" << std::endl;                 // return data location
        m_stack_size += return_type_size;
        m_generated << "; END PREPARE RETURN LOCATION INTO RDI" << std::endl;
    }
    // push parameters to the stack before calling
    m_generated << "; BEGIN OF FUNCTION PARAMATERS FOR " << function_call_expr.function_name << std::endl;
    size_t total_function_params_size = 0;
    for (auto& func_param : function_call_expr.parameters) {
        generate_expression(func_param);
        total_function_params_size += func_param.data_type->get_size_bytes();
    };
    m_generated << "; END OF FUNCTION PARAMATERS FOR " << function_call_expr.function_name << std::endl;

    m_generated << "\tcall " << function_call_expr.function_name << std::endl;
    m_generated << "\tadd rsp, " << total_function_params_size << "; CLEAR FUNCTION PARAMATERS FOR "
                << function_call_expr.function_name << std::endl;  // clear stack params
    if (return_type_size) {
        pop_stack_register("rax", 8, return_type_size);
        pop_stack_register("rdi", 8, 8);
        if (return_size_bytes) {
            // if should store return value on the stack
            std::string& reg = size_bytes_to_register.at(return_size_bytes);
            push_stack_register(reg, return_size_bytes);
        }
    } else {
        pop_stack_register("rdi", 8, 8);
    }
    m_stack_size -= total_function_params_size;
}
