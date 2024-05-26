#pragma once
#include "semantic_analyzer.hpp"
struct SemanticAnalyzer::ExpressionVisitor {
    SemanticAnalyzer* analyzer;
    DataType operator()(ASTIdentifier& identifier) const { return analyzer->analyze_expression_identifier(identifier); }

    DataType operator()(ASTIntLiteral& int_literal) const {
        return analyzer->analyze_expression_int_literal(int_literal);
    }

    DataType operator()(const std::shared_ptr<ASTAtomicExpression>& atomic) const {
        return analyzer->analyze_expression_atomic(atomic);
    }

    DataType operator()(const std::shared_ptr<ASTBinExpression>& binExpr) const {
        return analyzer->analyze_expression_binary(binExpr);
    }

    DataType operator()(const ASTParenthesisExpression& paren_expr) const {
        return analyzer->analyze_expression_parenthesis(paren_expr);
    }

    DataType operator()(ASTFunctionCallExpression& function_call_expr) const {
        return analyzer->analyze_expression_function_call(function_call_expr);
    }
};

struct SemanticAnalyzer::StatementVisitor {
    SemanticAnalyzer* analyzer;
    std::string function_name = "";
    void operator()(const std::shared_ptr<ASTStatementExit>& exit) const { analyzer->analyze_statement_exit(exit); }
    void operator()(const std::shared_ptr<ASTStatementVar>& var_declare) const {
        analyzer->analyze_statement_var_declare(var_declare);
    }
    void operator()(const std::shared_ptr<ASTStatementAssign>& var_assign) const {
        analyzer->analyze_statement_var_assign(var_assign);
    }
    void operator()(const std::shared_ptr<ASTStatementScope>& scope) const { analyzer->analyze_statement_scope(scope); }
    void operator()(const std::shared_ptr<ASTStatementIf>& _if) const { analyzer->analyze_statement_if(_if); }
    void operator()(const std::shared_ptr<ASTStatementWhile>& while_statement) const {
        analyzer->analyze_statement_while(while_statement);
    }
    void operator()(const std::shared_ptr<ASTStatementFunction>& function_statement) const {
        (void)function_statement;  // ignore unused
        assert(false && "Should never reach here. Function statements are to be parsed seperately");
    }
    void operator()(const std::shared_ptr<ASTStatementReturn>& return_statement) const {
        analyzer->analyze_statement_return(return_statement);
    }
};