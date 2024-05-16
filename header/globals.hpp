#pragma once
#include <string>

class Globals {
   private:
    std::string currentFilePath;

    Globals() {}

    // Delete copy constructor and copy assignment operator
    Globals(const Globals&) = delete;
    Globals& operator=(const Globals&) = delete;

   public:
    // Static member function to get the singleton instance
    static Globals& getInstance() {
        static Globals instance;  // Guaranteed to be initialized only once
        return instance;
    }

    // Getter and setter for currentFilename
    std::string getCurrentFilePath() const { return currentFilePath; }

    void setCurrentFilePath(const std::string& filePath) { currentFilePath = filePath; }
};
