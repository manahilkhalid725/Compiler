#include "type_checker.h"
#include <iostream>
#include <cctype>

void TypeChecker::analyze(const std::shared_ptr<ASTNode>& root) {
    if (!root) return;
    enterScope();
    analyzeNode(root, T_VOID);
    exitScope();
    std::cout << "[TypeChecker] Analysis completed successfully.\n";
}

void TypeChecker::enterScope() {
    symStack.push({});
}

void TypeChecker::exitScope() {
    if (!symStack.empty()) symStack.pop();
}

void TypeChecker::declareVar(const std::string& name, BasicType t) {
    std::cout << "[DeclareVar] '" << name << "' in scope level " << symStack.size() << "\n";
    if (symStack.empty()) enterScope();
    auto& cur = symStack.top();
    if (cur.find(name) != cur.end()) throw TypeCheckException("Variable redefinition: " + name);
    cur[name] = t;
}

BasicType TypeChecker::lookupVar(const std::string& name) {
    std::stack<std::unordered_map<std::string, BasicType>> temp = symStack;
    while (!temp.empty()) {
        auto& mp = temp.top();
        if (mp.find(name) != mp.end()) return mp[name];
        temp.pop();
    }
    return T_UNKNOWN;
}

void TypeChecker::declareFunction(const std::string& name, BasicType ret, const std::vector<BasicType>& params) {
    if (functions.find(name) != functions.end()) throw TypeCheckException("Function redefinition: " + name);
    functions[name] = {ret, params};
}

BasicType TypeChecker::parseTypeStr(const std::string& s) {
    if (s == "int") return T_INT;
    if (s == "float") return T_FLOAT;
    if (s == "bool") return T_BOOL;
    if (s == "string") return T_STRING;
    if (s == "void") return T_VOID;
    return T_UNKNOWN;
}

BasicType TypeChecker::typeOfLiteral(const std::string& lit) {
    if (lit == "true" || lit == "false") return T_BOOL;
    bool hasDot = false;
    bool hasNonDigit = false;
    for (char c : lit) {
        if (c == '.') hasDot = true;
        else if (!std::isdigit((unsigned char)c) && c != '-') hasNonDigit = true;
    }
    if (!hasNonDigit && hasDot) return T_FLOAT;
    if (!hasNonDigit) return T_INT;
    return T_STRING;
}

BasicType TypeChecker::unifyBinaryOp(const std::string& op, BasicType left, BasicType right) {
    if (op == "&&" || op == "||") {
        if (left != T_BOOL || right != T_BOOL)
            throw TypeCheckException("Attempted boolean operation on non-bools: " + op);
        return T_BOOL;
    }
    if (op == "==" || op == "!=") {
        if (left == T_UNKNOWN || right == T_UNKNOWN) throw TypeCheckException("EmptyExpression in equality");
        if (left != right) throw TypeCheckException("Attempted equality between different types");
        return T_BOOL;
    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!((left == T_INT || left == T_FLOAT) && (right == T_INT || right == T_FLOAT)) && !(left == T_STRING && right == T_STRING))
            throw TypeCheckException("Attempted relational op on non-numeric/string types: " + op);
        return T_BOOL;
    }
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (op == "+" && left == T_STRING && right == T_STRING) return T_STRING;
        if ((left == T_INT || left == T_FLOAT) && (right == T_INT || right == T_FLOAT)) {
            if (left == T_FLOAT || right == T_FLOAT) return T_FLOAT;
            return T_INT;
        }
        throw TypeCheckException("Attempted arithmetic op on non-numeric types: " + op);
    }
    throw TypeCheckException("Unknown binary operator: " + op);
}

BasicType TypeChecker::typeOfExpr(const std::shared_ptr<ASTNode>& expr) {
    if (!expr) return T_UNKNOWN;
    if (expr->kind == "Literal") {
        return typeOfLiteral(expr->val);
    }
    if (expr->kind == "Identifier") {
        BasicType t = lookupVar(expr->val);
        if (t == T_UNKNOWN) throw TypeCheckException("Undeclared variable in expression: " + expr->val);
        return t;
    }
    if (expr->kind == "PostfixOp") {
        if (expr->children.empty()) throw TypeCheckException("EmptyExpression in postfix");
        auto child = expr->children[0];
        BasicType t = typeOfExpr(child);
        if (!(t == T_INT || t == T_FLOAT)) throw TypeCheckException("Attempted increment/decrement on non-numeric");
        return t;
    }
    if (expr->kind == "BinaryOp") {
        if (expr->children.size() < 2) throw TypeCheckException("EmptyExpression in binary op");
        BasicType left = typeOfExpr(expr->children[0]);
        BasicType right = typeOfExpr(expr->children[1]);
        return unifyBinaryOp(expr->val, left, right);
    }
    if (expr->kind == "Assign") {
        if (expr->children.size() < 2) throw TypeCheckException("EmptyExpression in assign");
        auto lhs = expr->children[0];
        auto rhs = expr->children[1];
        if (lhs->kind != "Identifier") throw TypeCheckException("Left side of assignment must be identifier");
        BasicType lhsType = lookupVar(lhs->val);
        if (lhsType == T_UNKNOWN) throw TypeCheckException("Undeclared variable on assignment: " + lhs->val);
        BasicType rhsType = typeOfExpr(rhs);
        if (lhsType != rhsType && !(lhsType == T_FLOAT && rhsType == T_INT)) {
            throw TypeCheckException("Assignment type mismatch: " + lhs->val);
        }
        return lhsType;
    }
    if (expr->kind == "FunctionCall") {
        if (functions.find(expr->val) == functions.end()) throw TypeCheckException("Undefined function: " + expr->val);
        auto sig = functions[expr->val];
        if (sig.second.size() != expr->children.size()) throw TypeCheckException("FnCallParamCount for " + expr->val);
        for (size_t i = 0; i < sig.second.size(); ++i) {
            BasicType argt = typeOfExpr(expr->children[i]);
            if (argt != sig.second[i] && !(sig.second[i] == T_FLOAT && argt == T_INT))
                throw TypeCheckException("FnCallParamType mismatch for function " + expr->val);
        }
        return sig.first;
    }
    throw TypeCheckException("Unsupported expression kind: " + expr->kind);
}

void TypeChecker::analyzeNode(const std::shared_ptr<ASTNode>& node, BasicType currentFnRet) {
    if (!node) return;

    if (node->kind == "Program") {
        for (auto& c : node->children) analyzeNode(c, currentFnRet);
        return;
    }

    if (node->kind == "FunctionDecl") {
        if (node->children.size() < 3) throw TypeCheckException("Malformed function decl");
        std::string retTypeStr = node->children[0]->val;
        BasicType retType = parseTypeStr(retTypeStr);
        std::string fname = node->val;

        declareFunction(fname, retType, {});
        enterScope();

        std::vector<BasicType> paramTypes;
        if (node->children[2]->kind == "Params") {
            for (auto& p : node->children[2]->children) {
                std::string pname = "";
                std::string ptype = "";
                for (auto& pc : p->children) {
                    if (pc->kind == "Name" || pc->kind == "Identifier") pname = pc->val;
                    if (pc->kind == "Type") ptype = pc->val;
                }
                if (pname.empty()) pname = p->val;
                BasicType pt = parseTypeStr(ptype);
                declareVar(pname, pt);
                paramTypes.push_back(pt);
            }
        }

        functions[fname].second = paramTypes;

        BasicType savedRet = currentFnRet;
        currentFnRet = retType;
        analyzeNode(node->children.back(), currentFnRet);
        currentFnRet = savedRet;

        exitScope();
        return;
    }

    if (node->kind == "Block") {
        enterScope();
        for (auto& c : node->children) analyzeNode(c, currentFnRet);
        exitScope();
        return;
    }

    if (node->kind == "VarDecl") {
        if (node->children.size() < 2) throw TypeCheckException("ErroneousVarDecl");
        std::string typeName = node->children[0]->val;
        BasicType vt = parseTypeStr(typeName);
        std::string vname = "";
        for (auto& c : node->children) {
            if (c->kind == "Identifier" || c->kind == "Name") vname = c->val;
        }
        if (vname.empty()) throw TypeCheckException("VarDecl has empty identifier");

        if (lookupVar(vname) == T_UNKNOWN) declareVar(vname, vt);

        if (node->children.size() >= 3) {
            BasicType initT = typeOfExpr(node->children[2]);
            if (vt != initT && !(vt == T_FLOAT && initT == T_INT))
                throw TypeCheckException("ErroneousVarDecl initializer type mismatch for " + vname);
        }
        return;
    }

    if (node->kind == "Assign") {
        if (node->children.size() < 2) throw TypeCheckException("EmptyExpression");
        auto lhs = node->children[0];
        if (lhs->kind != "Identifier") throw TypeCheckException("Left side of assignment must be identifier");
        BasicType lhsType = lookupVar(lhs->val);
        if (lhsType == T_UNKNOWN) throw TypeCheckException("Undeclared variable on assignment: " + lhs->val);
        BasicType rhsT = typeOfExpr(node->children[1]);
        if (lhsType != rhsT && !(lhsType == T_FLOAT && rhsT == T_INT))
            throw TypeCheckException("ExpressionTypeMismatch on assignment to " + lhs->val);
        return;
    }

    if (node->kind == "PostfixOp") {
        if (node->children.empty()) throw TypeCheckException("EmptyExpression");
        BasicType t = typeOfExpr(node->children[0]);
        if (!(t == T_INT || t == T_FLOAT)) throw TypeCheckException("Attempted increment/decrement on non-numeric");
        return;
    }

    if (node->kind == "IfStmt") {
        if (node->children.empty()) throw TypeCheckException("EmptyExpression");
        BasicType condt = typeOfExpr(node->children[0]);
        if (condt != T_BOOL) throw TypeCheckException("NonBooleanCondStmt in if");
        analyzeNode(node->children[1], currentFnRet);
        if (node->children.size() > 2) analyzeNode(node->children[2], currentFnRet);
        return;
    }

    if (node->kind == "ReturnStmt") {
        if (node->children.empty()) {
            if (currentFnRet != T_VOID) throw TypeCheckException("ErroneousReturnType");
            return;
        }
        BasicType retExpr = typeOfExpr(node->children[0]);
        if (retExpr != currentFnRet && !(currentFnRet == T_FLOAT && retExpr == T_INT))
            throw TypeCheckException("ErroneousReturnType");
        return;
    }

    if (node->kind == "Identifier" || node->kind == "Literal" || node->kind == "BinaryOp" || 
        node->kind == "FunctionCall" || node->kind == "Assign" || node->kind == "PostfixOp") {
        typeOfExpr(node);
        return;
    }

    for (auto& c : node->children) analyzeNode(c, currentFnRet);
}
