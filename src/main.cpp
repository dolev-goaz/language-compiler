#include <string.h>

#include "../header/file_util.hpp"
#include "../header/generator.hpp"
#include "../header/lexer.hpp"
#include "../header/parser.hpp"

void handle_compile(std::string path);
void create_executable(std::string asm_code, std::string filename);

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Too few Arguments!" << std::endl
                  << "Usage: compiler <command> [...args]" << std::endl;
        exit(EXIT_FAILURE);
    }

    char* command = argv[1];

    if (strcmp(command, "compile") == 0) {
        std::string path = argv[2];
        handle_compile(path);
        exit(EXIT_SUCCESS);
    }
    return EXIT_FAILURE;
}

void handle_compile(std::string path) {
    std::string file_contents = read_file(path);

    Lexer lexer = Lexer(file_contents);
    std::vector<Token> tokens = lexer.tokenize();

    // for (auto&& token : tokens) {
    //     std::string value = token.value.has_value() ? token.value.value() :
    //     ""; std::cout << (int)token.type << " " << value << std::endl;
    // }

    Parser parser = Parser(tokens);
    ASTProgram program = parser.parse_program();

    Generator generator(program);
    std::string res = generator.generate_program();

    create_executable(res, "output");
}

void create_executable(std::string asm_code, std::string filename) {
    write_file("out.asm", asm_code);
    system("nasm -f elf64 out.asm");

    std::string command = "gcc -o " + filename + " out.o -e main";
    std::cout << command << std::endl;
    system(command.c_str());

    system("rm out.asm out.o");
}