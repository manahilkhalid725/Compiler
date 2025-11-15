#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>
#include <stdexcept>
#include <sstream>

class TypeCheckException : public std::runtime_error {
public:
    TypeCheckException(const std::string& msg) : std::runtime_error(msg) {}
};

enum BasicType {
    T_INT,
    T_FLOAT,
    T_BOOL,
    T_STRING,
    T_VOID,
    T_UNKNOWN
};

inline std::string basicTypeToStr(BasicType t) {
    switch (t) {
        case T_INT: return "int";
        case T_FLOAT: return "float";
        case T_BOOL: return "bool";
        case T_STRING: return "string";
        case T_VOID: return "void";
        default: return "unknown";
    }
}

class TypeChecker {
public:
    void analyze(const std::shared_ptr<ASTNode>& root);

private:
    std::stack<std::unordered_map<std::string, BasicType>> symStack;
    std::unordered_map<std::string, std::pair<BasicType, std::vector<BasicType>>> functions;

    void enterScope();
    void exitScope();
    void declareVar(const std::string& name, BasicType t);
    BasicType lookupVar(const std::string& name);
    void declareFunction(const std::string& name, BasicType ret, const std::vector<BasicType>& params);
    void analyzeNode(const std::shared_ptr<ASTNode>& node, BasicType currentFnRet = T_VOID);
    BasicType typeOfLiteral(const std::string& lit);
    BasicType unifyBinaryOp(const std::string& op, BasicType left, BasicType right);
    BasicType typeOfExpr(const std::shared_ptr<ASTNode>& expr);
    BasicType parseTypeStr(const std::string& s);
};
