#pragma once
#include <exception>
#include <sstream>
#include <string>

class SemanticAnalyzerException : public std::exception {
   public:
    SemanticAnalyzerException(const std::string& message) : m_message(message) {}

    const char* what() const noexcept override {
        std::stringstream stream;
        stream << "Semantic Exception:" << std::endl << m_message;

        m_formatted_message = stream.str();

        return m_formatted_message.c_str();
    }

   private:
    std::string m_message;

    mutable std::string m_formatted_message;
};
