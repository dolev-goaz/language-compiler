#include <iostream>
#include <fstream>
#include <vector>

std::string read_file(const std::string& path) {
    std::ifstream file = std::ifstream(path.c_str());
    
    if (!file.is_open()) {
        std::cerr << "Error opening file " << "'" << path <<  "'" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<char> buffer;
    char ch;
    while (file.get(ch)) {
        buffer.push_back(ch);
    }

    file.close();

    std::string res = buffer.data();

    return res;
}