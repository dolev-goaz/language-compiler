#pragma once
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

    std::optional<ASTExpression> parse_expression();
    std::shared_ptr<ASTAtomicExpression> try_parse_atomic();
    std::shared_ptr<ASTBinExpression> try_parse_bin_expression(const ASTAtomicExpression& lhs);

    std::optional<Token> consume();
    std::optional<Token> peek(int offset = 0);
    std::optional<Token> try_consume(TokenType type);
    bool test_peek(TokenType type, int offset = 0);
    Token assert_consume(TokenType type, const std::string& msg);
};