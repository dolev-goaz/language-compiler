#pragma once
#include <assert.h>

#include <optional>
#include <vector>

#include "./AST_node.hpp"
#include "./error/parser_error.hpp"
#include "./lexer.hpp"

extern std::map<TokenType, BinOperation> binOperationMapping;
class Parser {
   public:
    Parser(std::vector<Token> tokens) : m_tokens(tokens), m_token_index(0) {}

    ASTProgram parse_program();

   private:
    std::vector<Token> m_tokens;
    size_t m_token_index = 0;

    std::shared_ptr<ASTStatement> parse_statement();

    std::shared_ptr<ASTStatementExit> parse_statement_exit();
    std::shared_ptr<ASTStatementVar> parse_statement_var_declare();
    std::shared_ptr<ASTStatementAssign> parse_statement_var_assign();
    std::shared_ptr<ASTStatementScope> parse_statement_scope();
    std::shared_ptr<ASTStatementIf> parse_statement_if();
    std::shared_ptr<ASTStatementWhile> parse_statement_while();

    std::shared_ptr<ASTStatementFunction> parse_statement_function();
    std::vector<ASTFunctionParam> parse_function_params();
    std::shared_ptr<ASTStatementFunctionCall> parse_statement_function_call();

    // uses predence climbing, described here-
    // https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
    std::optional<ASTExpression> parse_expression(const int min_prec = 0);
    std::shared_ptr<ASTAtomicExpression> try_parse_atomic();
    std::optional<int> binary_operator_precedence(const BinOperation& operation);

    std::optional<Token> consume();
    std::optional<Token> peek(int offset = 0);
    std::optional<Token> try_consume(TokenType type);
    bool test_peek(TokenType type, int offset = 0);
    Token assert_consume(TokenType type, const std::string& msg);
};