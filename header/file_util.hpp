#include <fstream>
#include <iostream>
#include <vector>

std::string read_file(const std::string& path) {
    std::ifstream file = std::ifstream(path.c_str());

    if (!file.is_open()) {
        std::cerr << "Error opening file "
                  << "'" << path << "'" << std::endl;
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

void write_file(std::string filename, std::string contents) {
    std::ofstream file = std::ofstream(filename);
    file << contents;
    file.close();
}