#pragma once
#include <exception>
#include <sstream>
#include <string>

#include "globals.hpp"

class LexerException : public std::exception {
   public:
    LexerException(const std::string& message, size_t line, size_t col)
        : m_message(message), m_line(line), m_col(col) {}

    const char* what() const noexcept override {
        std::stringstream stream;
        stream << "TOKEN EXCEPTION at " << Globals::getInstance().getCurrentFilePath() << ":" << m_line << ":" << m_col
               << ":" << std::endl
               << m_message;

        m_formatted_message = stream.str();

        return m_formatted_message.c_str();
    }

   private:
    const std::string m_message;
    const size_t m_line;
    const size_t m_col;

    mutable std::string m_formatted_message;
};
