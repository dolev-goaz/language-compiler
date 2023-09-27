#pragma once
#include <exception>
#include <sstream>
#include <string>

class ParserException : public std::exception {
   public:
    ParserException(const std::string& message, size_t line, size_t col)
        : m_message(message), m_line(line), m_col(col) {}
    ParserException(const std::string& message) : m_message(message), m_line(0), m_col(0) {}

    const char* what() const noexcept override {
        std::stringstream stream;
        stream << "Parser Exception:" << std::endl << m_message << std::endl;

        if (m_line && m_col) {
            stream << "Line: " << m_line << ", Column: " << m_col;
        }

        m_formatted_message = stream.str();

        return m_formatted_message.c_str();
    }

   private:
    std::string m_message;
    size_t m_line;
    size_t m_col;

    mutable std::string m_formatted_message;
};
