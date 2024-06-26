#pragma once
#include <exception>
#include <sstream>
#include <string>

#include "lexer.hpp"

class SemanticAnalyzerException : public std::exception {
   public:
    SemanticAnalyzerException(const std::string& message, const TokenMeta& meta)
        : m_message(message), m_line(meta.line_num), m_col(meta.line_pos) {}

    const char* what() const noexcept override {
        std::stringstream stream;
        stream << "SEMANTIC EXCEPTION at " << Globals::getInstance().getCurrentFilePosition(m_line, m_col) << ":"
               << std::endl
               << m_message;

        m_formatted_message = stream.str();

        return m_formatted_message.c_str();
    }

   private:
    std::string m_message;
    const size_t m_line;
    const size_t m_col;

    mutable std::string m_formatted_message;
};
