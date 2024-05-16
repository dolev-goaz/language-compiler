#pragma once
#include <exception>
#include <sstream>
#include <string>

#include "globals.hpp"

class ParserException : public std::exception {
   public:
    ParserException(const std::string& message, const TokenMeta& meta)
        : m_message(message), m_line(meta.line_num), m_col(meta.line_pos) {}

    ParserException(const std::string& message) : m_message(message), m_line(0), m_col(0) {}

    const char* what() const noexcept override {
        std::stringstream stream;
        stream << "PARSER EXCEPTION at " << Globals::getInstance().getCurrentFilePath();
        if (m_line && m_col) {
            stream << ":" << m_line << ":" << m_col << ":";
        } else {
            stream << ": Reached end of file unexpectedly.";
        }
        stream << std::endl << m_message;

        m_formatted_message = stream.str();

        return m_formatted_message.c_str();
    }

   private:
    std::string m_message;
    const size_t m_line;
    const size_t m_col;

    mutable std::string m_formatted_message;
};
