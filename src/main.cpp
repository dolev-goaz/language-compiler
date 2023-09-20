#include "../header/file_util.hpp"
#include <iostream>

int main() {
    std::string file_contents =  read_file("./code_test.dlv");
    std::cout << file_contents << std::endl;
    return 0;
}
