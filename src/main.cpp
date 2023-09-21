#include <string.h>

#include "../header/file_util.hpp"
#include "../header/token.hpp"

void handle_compile(char* path);

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Too few Arguments!" << std::endl
                  << "Usage: compiler <command> [...args]" << std::endl;
        exit(EXIT_FAILURE);
    }

    char* command = argv[1];

    if (strcmp(command, "compile") == 0) {
        char* path = argv[2];
        handle_compile(path);
        exit(EXIT_SUCCESS);
    }
    return EXIT_FAILURE;
}

void handle_compile(char* path) {
    std::string file_contents = read_file(path);

    TokenParser tokenizer = TokenParser(file_contents);
    std::vector<Token> tokens = tokenizer.tokenize();
}