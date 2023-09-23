#include <string.h>

#include "../header/file_util.hpp"
#include "../header/generator.hpp"
#include "../header/parser.hpp"
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

    Tokenizer tokenizer = Tokenizer(file_contents);
    std::vector<Token> tokens = tokenizer.tokenize();

    // for (auto&& token : tokens) {
    //     std::string value = token.value.has_value() ? token.value.value() :
    //     ""; std::cout << (int)token.type << " " << value << std::endl;
    // }

    Parser parser = Parser(tokens);
    ASTProgram program = parser.parse_program();

    Generator generator(program);
    std::string res = generator.generate_program();
    std::cout << res << std::endl;
}