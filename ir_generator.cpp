#include "ir_generator.h"

std::string IRGenerator::newTemp() {
    return "t" + std::to_string(tempCounter++);
}

std::string IRGenerator::newLabel() {
    return "L" + std::to_string(labelCounter++);
}

void IRGenerator::emit(const TACInstruction& instr) {
    instructions.push_back(instr);
}

void IRGenerator::emit(const std::string& op, const std::string& result,
                       const std::string& arg1, const std::string& arg2) {
    instructions.emplace_back(op, result, arg1, arg2);
}

void IRGenerator::generate(const std::shared_ptr<ASTNode>& root) {
    if (!root) {
        throw IRException("Cannot generate IR from null AST");
    }
    
    std::cout << "\n[IRGenerator] Starting IR generation\n";
    generateNode(root);
    std::cout << "[IRGenerator] IR generation completed successfully.\n";
}

void IRGenerator::printIR() const {
    std::cout << "\n=== Three-Address Code (TAC) ===" << std::endl;
    for (const auto& instr : instructions) {
        std::cout << instr.toString() << std::endl;
    }
    std::cout << "================================\n" << std::endl;
}

void IRGenerator::generateNode(const std::shared_ptr<ASTNode>& node) {
    if (!node) return;
    
    if (node->kind == "Program") {
        for (auto& child : node->children) {
            generateNode(child);
        }
    }
    else if (node->kind == "FunctionDecl") {
        generateFunction(node);
    }
    else if (node->kind == "Block" || node->kind == "CompoundStmt") {
        generateBlock(node);
    }
    else if (node->kind == "VarDecl") {
        generateVarDecl(node);
    }
    else if (node->kind == "Assign") {
        generateAssignment(node);
    }
    else if (node->kind == "IfStmt" || node->kind == "If") {
        generateIf(node);
    }
    else if (node->kind == "WhileStmt" || node->kind == "While") {
        generateWhile(node);
    }
    else if (node->kind == "ForStmt" || node->kind == "For") {
        generateFor(node);
    }
    else if (node->kind == "ReturnStmt" || node->kind == "Return") {
        generateReturn(node);
    }
    else if (node->kind == "PostfixOp") {
        generatePostfixOp(node);
    }
    else if (node->kind == "PrefixOp" || node->kind == "UnaryOp") {
        generatePrefixOp(node);
    }
    else if (node->kind == "FunctionCall" || node->kind == "CallExpr") {
        generateFunctionCall(node);
    }
    else if (node->kind == "ExprStmt") {
        if (!node->children.empty()) {
            generateExpr(node->children[0]);
        }
    }
    else {
        for (auto& child : node->children) {
            generateNode(child);
        }
    }
}

void IRGenerator::generateFunction(const std::shared_ptr<ASTNode>& node) {
    if (node->children.size() < 3) {
        throw IRException("Invalid function declaration structure");
    }
    
    std::string funcName;
    if (!node->val.empty()) {
        funcName = node->val;
    } else if (node->children.size() > 1 && node->children[1]->kind == "Name") {
        funcName = node->children[1]->val;
    } else {
        throw IRException("Function declaration missing name");
    }
    
    currentFunction = funcName;
    
    emit("label", "func_" + funcName);
    
    for (auto& child : node->children) {
        if (child && child->kind == "Params") {
            for (auto& param : child->children) {
                if (param && param->kind == "Param") {
                    std::string paramName = param->val;
                    emit("param", paramName);
                    
                    if (!param->children.empty() && param->children[0]->kind == "Type") {
                        varTypes[paramName] = param->children[0]->val;
                    }
                }
            }
        }
    }
    
    for (auto& child : node->children) {
        if (child && (child->kind == "Block" || child->kind == "CompoundStmt")) {
            generateBlock(child);
        }
    }
    
    emit("return");
    emit("label", "end_" + funcName);
    
    currentFunction = "";
}

void IRGenerator::generateBlock(const std::shared_ptr<ASTNode>& node) {
    for (auto& stmt : node->children) {
        generateNode(stmt);
    }
}

void IRGenerator::generateVarDecl(const std::shared_ptr<ASTNode>& node) {
    std::string varName = node->val;
    
    for (auto& child : node->children) {
        if (child && child->kind == "Type") {
            varTypes[varName] = child->val;
        }
    }
    
    for (auto& child : node->children) {
        if (child && child->kind != "Type" && child->kind != "Identifier") {
            std::string initValue = generateExpr(child);
            emit("=", varName, initValue);
        }
    }
}

void IRGenerator::generateAssignment(const std::shared_ptr<ASTNode>& node) {
    if (node->children.size() < 2) {
        throw IRException("Assignment node must have at least 2 children");
    }
    
    auto lhs = node->children[0];
    auto rhs = node->children[1];
    
    if (!lhs || !rhs) {
        throw IRException("Assignment has null operands");
    }
    
    if (lhs->kind == "ArrayAccess" || lhs->kind == "Subscript") {
        std::string arrayName = lhs->children[0]->val;
        std::string indexTemp = generateExpr(lhs->children[1]);
        std::string valueTemp = generateExpr(rhs);
        emit("[]=", arrayName, indexTemp, valueTemp);
    }
    else if (lhs->kind == "Identifier") {
        std::string rhsTemp = generateExpr(rhs);
        emit("=", lhs->val, rhsTemp);
    }
    else {
        throw IRException("Invalid left-hand side of assignment");
    }
}

void IRGenerator::generateIf(const std::shared_ptr<ASTNode>& node) {
    if (node->children.empty()) {
        throw IRException("If statement missing condition");
    }
    
    std::string condTemp = generateExpr(node->children[0]);
    
    std::string labelElse = newLabel();
    std::string labelEnd = newLabel();
    
    emit("ifFalse", labelElse, condTemp);
    
    if (node->children.size() > 1) {
        generateNode(node->children[1]);
    }
    
    emit("goto", labelEnd);
    
    emit("label", labelElse);
    
    if (node->children.size() > 2) {
        generateNode(node->children[2]);
    }
    
    emit("label", labelEnd);
}

void IRGenerator::generateWhile(const std::shared_ptr<ASTNode>& node) {
    if (node->children.empty()) {
        throw IRException("While statement missing condition");
    }
    
    std::string labelStart = newLabel();
    std::string labelEnd = newLabel();
    
    emit("label", labelStart);
    
    std::string condTemp = generateExpr(node->children[0]);
    
    emit("ifFalse", labelEnd, condTemp);
    
    if (node->children.size() > 1) {
        generateNode(node->children[1]);
    }
    
    emit("goto", labelStart);
    
    emit("label", labelEnd);
}

void IRGenerator::generateFor(const std::shared_ptr<ASTNode>& node) {
    if (node->children.size() < 4) {
        throw IRException("For statement has insufficient children");
    }
    
    if (node->children[0]) {
        generateNode(node->children[0]);
    }
    
    std::string labelStart = newLabel();
    std::string labelUpdate = newLabel();
    std::string labelEnd = newLabel();
    
    emit("label", labelStart);
    
    if (node->children[1]) {
        std::string condTemp = generateExpr(node->children[1]);
        emit("ifFalse", labelEnd, condTemp);
    }
    
    if (node->children[3]) {
        generateNode(node->children[3]);
    }
    
    emit("label", labelUpdate);
    
    if (node->children[2]) {
        generateNode(node->children[2]);
    }
    
    emit("goto", labelStart);
    
    emit("label", labelEnd);
}

void IRGenerator::generateReturn(const std::shared_ptr<ASTNode>& node) {
    if (node->children.empty()) {
        emit("return");
    } else {
        std::string retTemp = generateExpr(node->children[0]);
        emit("return", retTemp);
    }
}

std::string IRGenerator::generateExpr(const std::shared_ptr<ASTNode>& node) {
    if (!node) {
        throw IRException("Cannot generate expression from null node");
    }
    
    if (node->kind == "Literal" || node->kind == "IntLiteral" || 
        node->kind == "FloatLiteral" || node->kind == "StringLiteral" ||
        node->kind == "BoolLiteral") {
        return node->val;
    }
    else if (node->kind == "Identifier") {
        return node->val;
    }
    else if (node->kind == "BinaryOp" || node->kind == "BinaryExpr") {
        return generateBinaryOp(node);
    }
    else if (node->kind == "UnaryOp") {
        return generateUnaryOp(node);
    }
    else if (node->kind == "PostfixOp") {
        return generatePostfixOp(node);
    }
    else if (node->kind == "PrefixOp") {
        return generatePrefixOp(node);
    }
    else if (node->kind == "FunctionCall" || node->kind == "CallExpr") {
        return generateFunctionCall(node);
    }
    else if (node->kind == "ArrayAccess" || node->kind == "Subscript") {
        if (node->children.size() < 2) {
            throw IRException("Array access requires array and index");
        }
        std::string arrayName = node->children[0]->val;
        std::string indexTemp = generateExpr(node->children[1]);
        std::string resultTemp = newTemp();
        emit("[]", resultTemp, arrayName, indexTemp);
        return resultTemp;
    }
    else {
        throw IRException("Unknown expression node kind: " + node->kind);
    }
}

std::string IRGenerator::generateBinaryOp(const std::shared_ptr<ASTNode>& node) {
    if (node->children.size() < 2) {
        throw IRException("Binary operation requires two operands");
    }
    
    std::string left = generateExpr(node->children[0]);
    std::string right = generateExpr(node->children[1]);
    std::string resultTemp = newTemp();
    
    std::string op = node->val;
    
    if (op == "+") emit("+", resultTemp, left, right);
    else if (op == "-") emit("-", resultTemp, left, right);
    else if (op == "*") emit("*", resultTemp, left, right);
    else if (op == "/") emit("/", resultTemp, left, right);
    else if (op == "%") emit("%", resultTemp, left, right);
    else if (op == "==") emit("==", resultTemp, left, right);
    else if (op == "!=") emit("!=", resultTemp, left, right);
    else if (op == "<") emit("<", resultTemp, left, right);
    else if (op == ">") emit(">", resultTemp, left, right);
    else if (op == "<=") emit("<=", resultTemp, left, right);
    else if (op == ">=") emit(">=", resultTemp, left, right);
    else if (op == "&&") emit("&&", resultTemp, left, right);
    else if (op == "||") emit("||", resultTemp, left, right);
    else {
        throw IRException("Unknown binary operator: " + op);
    }
    
    return resultTemp;
}

std::string IRGenerator::generateUnaryOp(const std::shared_ptr<ASTNode>& node) {
    if (node->children.empty()) {
        throw IRException("Unary operation requires one operand");
    }
    
    std::string operand = generateExpr(node->children[0]);
    std::string resultTemp = newTemp();
    
    std::string op = node->val;
    
    if (op == "!") {
        emit("!", resultTemp, operand);
    }
    else if (op == "-") {
        emit("-_unary", resultTemp, operand);
    }
    else if (op == "+") {
        emit("+_unary", resultTemp, operand);
    }
    else {
        throw IRException("Unknown unary operator: " + op);
    }
    
    return resultTemp;
}

std::string IRGenerator::generatePostfixOp(const std::shared_ptr<ASTNode>& node) {
    if (node->children.empty()) {
        throw IRException("Postfix operation requires operand");
    }
    
    auto operand = node->children[0];
    if (!operand || operand->kind != "Identifier") {
        throw IRException("Postfix operation requires identifier");
    }
    
    std::string varName = operand->val;
    std::string resultTemp = newTemp();
    std::string op = node->val;
    
    if (op == "++") {
        emit("=", resultTemp, varName);
        std::string oneTemp = newTemp();
        emit("=", oneTemp, "1");
        emit("+", varName, varName, oneTemp);
    }
    else if (op == "--") {
        emit("=", resultTemp, varName);
        std::string oneTemp = newTemp();
        emit("=", oneTemp, "1");
        emit("-", varName, varName, oneTemp);
    }
    else {
        throw IRException("Unknown postfix operator: " + op);
    }
    
    return resultTemp;
}

std::string IRGenerator::generatePrefixOp(const std::shared_ptr<ASTNode>& node) {
    if (node->children.empty()) {
        throw IRException("Prefix operation requires operand");
    }
    
    auto operand = node->children[0];
    if (!operand || operand->kind != "Identifier") {
        throw IRException("Prefix operation requires identifier");
    }
    
    std::string varName = operand->val;
    std::string op = node->val;
    
    if (op == "++") {
        std::string oneTemp = newTemp();
        emit("=", oneTemp, "1");
        emit("+", varName, varName, oneTemp);
        return varName;
    }
    else if (op == "--") {
        std::string oneTemp = newTemp();
        emit("=", oneTemp, "1");
        emit("-", varName, varName, oneTemp);
        return varName;
    }
    else {
        throw IRException("Unknown prefix operator: " + op);
    }
}

std::string IRGenerator::generateFunctionCall(const std::shared_ptr<ASTNode>& node) {
    std::string funcName;
    int argStartIndex = 0;
    
    if (!node->val.empty()) {
        funcName = node->val;
    } else if (!node->children.empty() && node->children[0]->kind == "Identifier") {
        funcName = node->children[0]->val;
        argStartIndex = 1;
    } else {
        throw IRException("Function call missing function name");
    }
    
    std::vector<std::string> argTemps;
    for (size_t i = argStartIndex; i < node->children.size(); ++i) {
        if (node->children[i]->kind == "ArgumentList" || node->children[i]->kind == "Args") {
            for (auto& arg : node->children[i]->children) {
                std::string argTemp = generateExpr(arg);
                argTemps.push_back(argTemp);
            }
        } else {
            std::string argTemp = generateExpr(node->children[i]);
            argTemps.push_back(argTemp);
        }
    }
    
    for (const auto& arg : argTemps) {
        emit("param", arg);
    }
    
    std::string resultTemp = newTemp();
    emit("call", resultTemp, funcName, std::to_string(argTemps.size()));
    
    return resultTemp;
}