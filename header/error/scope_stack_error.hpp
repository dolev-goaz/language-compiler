#pragma once
#include <exception>
#include <string>

class ScopeStackException : public std::exception {
   public:
    ScopeStackException(const std::string& message) : m_message(message) {}

    const char* what() const noexcept override { return m_message.c_str(); }

   private:
    std::string m_message;
};
