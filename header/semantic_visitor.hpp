#pragma once
#include "semantic_analyzer.hpp"
struct SemanticAnalyzer::ExpressionAnalysisResult {
    std::shared_ptr<DataType> data_type;
    bool is_literal;
};

struct SemanticAnalyzer::ExpressionVisitor {
    SemanticAnalyzer* analyzer;
    std::shared_ptr<DataType> lhs_datatype;  // used for array initializing for now
    SemanticAnalyzer::ExpressionAnalysisResult operator()(ASTIdentifier& identifier) const {
        return analyzer->analyze_expression_identifier(identifier);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(ASTIntLiteral& int_literal) const {
        return analyzer->analyze_expression_int_literal(int_literal);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(ASTCharLiteral& char_literal) const {
        return analyzer->analyze_expression_char_literal(char_literal);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(ASTArrayInitializer& initializer) const {
        if (lhs_datatype) {
            std::cout << lhs_datatype->toString() << std::endl;
        }
        return analyzer->analyze_expression_array_initializer(initializer, lhs_datatype);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(const std::shared_ptr<ASTAtomicExpression>& atomic) const {
        return analyzer->analyze_expression_atomic(atomic, lhs_datatype);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(const std::shared_ptr<ASTBinExpression>& binExpr) const {
        return analyzer->analyze_expression_binary(binExpr);
    }
    SemanticAnalyzer::ExpressionAnalysisResult operator()(const std::shared_ptr<ASTUnaryExpression>& unary) const {
        return analyzer->analyze_expression_unary(unary);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(const ASTParenthesisExpression& paren_expr) const {
        return analyzer->analyze_expression_parenthesis(paren_expr);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(ASTFunctionCall& function_call_expr) const {
        return analyzer->analyze_function_call(function_call_expr);
    }

    SemanticAnalyzer::ExpressionAnalysisResult operator()(std::shared_ptr<ASTArrayIndexExpression>& arr_index) const {
        return analyzer->analyze_expression_array_indexing(arr_index);
    }
};

struct SemanticAnalyzer::StatementVisitor {
    SemanticAnalyzer* analyzer;
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
    void operator()(std::shared_ptr<ASTFunctionCall>& function_call_statement) const {
        auto& statement = *function_call_statement.get();
        analyzer->analyze_function_call(statement);
    }
};