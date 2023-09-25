#pragma once
#include "generator.hpp"

struct Generator::StatementVisitor {
    Generator& generator;

    void operator()(const ASTStatementExit& exit) const { generator.generate_exit(exit); }
    void operator()(const ASTStatementVar& var_declare) const {
        // check variable name already exists
        if (generator.m_variables.count(var_declare.name) > 0) {
            std::cerr << "Variable " << var_declare.name << "already exists!" << std::endl;
            exit(EXIT_FAILURE);
        }

        Generator::Variable var = {
            .stack_location_bytes = generator.m_stack_size,
            .size_bytes = 8,  // hard coded for now
        };
        generator.m_variables.insert({var_declare.name, var});
        // TODO: need to refer to size_bytes when initializing(push only the required size)
        if (var_declare.value.has_value()) {
            generator.generate_expression(var_declare.value.value());
        } else {
            // TODO: initialize with 0
        }

        generator.m_stack_size += var.size_bytes;
    }
};

struct Generator::ExpressionVisitor {
    Generator& generator;

    void operator()(const ASTIdentifier& identifier) const {
        if (generator.m_variables.count(identifier.value) == 0) {
            std::cerr << "Variable " << identifier.value << "does not exist!" << std::endl;
            exit(EXIT_FAILURE);
        }

        // get variable metadata
        Generator::Variable variable = generator.m_variables.at(identifier.value);

        // get variable position in the stack
        int offset = generator.m_stack_size - variable.stack_location_bytes;
        // rsp was on the next FREE address, offset it back to the variable's position.
        offset -= variable.size_bytes;

        // TODO: take variable.size_bytes into account(only push the bytes needed)
        // push variable value into stack
        generator.m_generated << "\tpush QWORD [rsp + " << offset << "]" << std::endl;
        generator.m_stack_size += variable.size_bytes;
    }

    void operator()(const ASTIntLiteral& literal) const {
        generator.m_generated << "\tpush QWORD " << literal.value << std::endl;
    }
};