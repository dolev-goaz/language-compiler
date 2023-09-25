#pragma once
#include "generator.hpp"

struct Generator::StatementVisitor {
    Generator& generator;

    void operator()(const ASTStatementExit& exit) const { generator.generate_exit(exit); }
    void operator()(const ASTStatementVar& var_declare) const {
        // check variable name already exists
        if (generator.m_variables.count(var_declare.name) > 0) {
            std::cerr << "Identifier " << var_declare.name << "already exists!" << std::endl;
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
        std::cerr << "Not Implemented Yet!" << std::endl;
        exit(EXIT_FAILURE);
    }

    void operator()(const ASTIntLiteral& literal) const {
        generator.m_generated << "\tpush QWORD " << literal.value << std::endl;
    }
};