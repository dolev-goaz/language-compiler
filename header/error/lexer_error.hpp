#pragma once
#include <exception>
#include <sstream>
#include <string>

class LexerException : public std::exception {
   public:
    LexerException(const std::string& message, const std::string& file_path, size_t line, size_t col)
        : m_message(message), m_file_path(file_path), m_line(line), m_col(col) {}

    const char* what() const noexcept override {
        std::stringstream stream;
        stream << "TOKEN EXCEPTION at " << m_file_path << ":" << m_line << ":" << m_col << ":" << std::endl
               << m_message;

        m_formatted_message = stream.str();

        return m_formatted_message.c_str();
    }

   private:
    const std::string m_message;
    const std::string& m_file_path;
    const size_t m_line;
    const size_t m_col;

    mutable std::string m_formatted_message;
};
