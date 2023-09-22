#include <string>
#include <vector>

#include "./token.hpp"

class ASTNode {};

class ASTExpression : public ASTNode {};  // used for inheritence

class ASTIdentifier : public ASTExpression {
   public:
    Token value;
};

class ASTIntLiteral : public ASTExpression {
   public:
    Token value;
};

class ASTStatement : public ASTNode {};

class ASTStatementExit : public ASTStatement {
   public:
    ASTExpression code;
};

class ASTProgram : public ASTNode {
   public:
    std::vector<ASTStatement> statements;
};