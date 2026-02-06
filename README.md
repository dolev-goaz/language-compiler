# Language Compiler

## About

A custom language compiler I've written in C++.
Supports a simple, custom language with basic features like variables, functions, and control flow.

The compiler transpiles the custom language into assembly code, which is then assembled and linked to create an executable.

The compiler is implemented using LL Top-Down Parsing via Recursive Descent.
It's using backtracking to handle ambiguities in the grammar, and it performs semantic analysis to ensure correct usage of variables, functions, and control flow constructs.

## Features

- Variable declarations and assignments
- Scope management
- Control flow statements (if, while)
- Function definitions and calls with parameters and return values
- Exit statements for program termination with status codes
- Expression evaluation, including binary and unary operations, function calls, and array indexing
- Array initializers
- Strings, implemented as arrays of characters
- Pointer types and dereferencing
- Semantic analysis to ensure correct usage of variables, functions, and control flow constructs

Grammar is defined in `current_grammer.md` and `grammar.md`.
For now the only output of the custom language is the exit code of the program, which can be set using the `exit` statement.

## Usage

1. Clone the repository:
   ```bash
   git clone https://github.com/dolev-goaz/language-compiler.git
   cd language-compiler
   ```
2. Build the compiler:
   ```bash
   make
   ```
3. Compile a source file:
   ```bash
   ./compiler compile path/to/source_file.lang
   ```
4. Run the compiled executable:
   ```bash
   ./output
   ```

## Example - code_test.dlv

```c
int_64 a = 50;

int_16* b = &a;

*b = 20;
exit(a);
```

### Compiling and running-

```bash
./compiler compile code_test.dlv
./output
echo $? # Output status code, should be 20
```

## Testing

Testing is implemented in `Python`, at `testing/tester.py`
Test programs are defined in `testing/programs`, and expected outputs are defined in `testing/test_cases.json`.

## Dependencies

- g++ (for compiling the compiler)
- NASM (for assembling the generated assembly code)

## File Structure

```
.
├── header
│   ├── AST_node.hpp
│   ├── data_type.hpp
│   ├── debug_utils.hpp
│   ├── error
│   │   ├── lexer_error.hpp
│   │   ├── parser_error.hpp
│   │   ├── scope_stack_error.hpp
│   │   └── sem_analyze_error.hpp
│   ├── file_util.hpp
│   ├── generator.hpp
│   ├── generator_visitor.hpp
│   ├── globals.hpp
│   ├── lexer.hpp
│   ├── parser.hpp
│   ├── parse_statement_stack.hpp
│   ├── scope_stack.hpp
│   ├── semantic_analyzer.hpp
│   └── semantic_visitor.hpp
├── src
│   ├── data_type.cpp
│   ├── debug_utils.cpp
│   ├── file_util.cpp
│   ├── generation
│   │   ├── expression.cpp
│   │   ├── function.cpp
│   │   ├── generator.cpp
│   │   ├── statememt.cpp
│   │   └── utilities.cpp
│   ├── lexer.cpp
│   ├── main.cpp
│   ├── parsing
│   │   ├── expression.cpp
│   │   ├── function.cpp
│   │   ├── statement.cpp
│   │   └── utilities.cpp
│   └── semantic_analysis
│       ├── expression.cpp
│       ├── function.cpp
│       ├── statement.cpp
│       └── utilities.cpp
├── testing
│   ├── programs            # Contains test programs in the custom language
│   ├── test_cases.json     # Contains expected outputs for the test programs
│   └── tester.py           # Python script to run tests and compare outputs
├── makefile
├── code_test.dlv
├── current_grammer.md
├── grammar.md
├── syntax.md
└── README.md
```
