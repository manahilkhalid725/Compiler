#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include "ast.h"
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <unordered_map>

class IRException : public std::exception 
{
    std::string message;
public:
    explicit IRException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

struct TACInstruction {
    std::string op; 
    std::string result;  
    std::string arg1;    
    std::string arg2;     
    
    TACInstruction(std::string operation, std::string res = "", std::string a1 = "", std::string a2 = "")
        : op(std::move(operation)), result(std::move(res)), arg1(std::move(a1)), arg2(std::move(a2)) {}
    
    std::string toString() const {
        if (op == "label") {
            return result + ":";
        } else if (op == "goto") {
            return "    goto " + result;
        } else if (op == "if") {
            return "    if " + arg1 + " goto " + result;
        } else if (op == "ifFalse") {
            return "    ifFalse " + arg1 + " goto " + result;
        } else if (op == "param") {
            return "    param " + result;
        } else if (op == "call") {
            if (!result.empty() && !arg1.empty()) {
                return "    " + result + " = call " + arg1 + ", " + arg2;
            } else {
                return "    call " + arg1 + ", " + arg2;
            }
        } else if (op == "return") {
            if (!result.empty()) {
                return "    return " + result;
            } else {
                return "    return";
            }
        } else if (op == "=") {
            return "    " + result + " = " + arg1;
        } else if (op == "[]") {
            return "    " + result + " = " + arg1 + "[" + arg2 + "]";
        } else if (op == "[]=") {
            return "    " + result + "[" + arg1 + "] = " + arg2;
        } else if (op == "++_post" || op == "--_post") {
            return "    " + result + " = " + arg1 + " " + op.substr(0, 2);
        } else if (op == "++_pre" || op == "--_pre") {
            return "    " + op.substr(0, 2) + " " + result;
        } else if (op == "!" || op == "-_unary" || op == "+_unary") {
            std::string actualOp = op;
            if (op == "-_unary") actualOp = "-";
            else if (op == "+_unary") actualOp = "+";
            return "    " + result + " = " + actualOp + arg1;
        } else if (arg2.empty()) {
            return "    " + result + " = " + op + " " + arg1;
        } else {
            return "    " + result + " = " + arg1 + " " + op + " " + arg2;
        }
    }
};

class IRGenerator {
public:
    IRGenerator() : tempCounter(0), labelCounter(0) {}
    
    void generate(const std::shared_ptr<ASTNode>& root);
    void printIR() const;
    std::vector<TACInstruction> getInstructions() const { return instructions; }
    
private:
    std::vector<TACInstruction> instructions;
    int tempCounter;
    int labelCounter;
    std::string currentFunction;
    std::unordered_map<std::string, std::string> varTypes;
    
    std::string newTemp();
    std::string newLabel();
    
    void generateNode(const std::shared_ptr<ASTNode>& node);
    std::string generateExpr(const std::shared_ptr<ASTNode>& node);
    void generateFunction(const std::shared_ptr<ASTNode>& node);
    void generateVarDecl(const std::shared_ptr<ASTNode>& node);
    void generateAssignment(const std::shared_ptr<ASTNode>& node);
    void generateIf(const std::shared_ptr<ASTNode>& node);
    void generateWhile(const std::shared_ptr<ASTNode>& node);
    void generateFor(const std::shared_ptr<ASTNode>& node);
    void generateReturn(const std::shared_ptr<ASTNode>& node);
    void generateBlock(const std::shared_ptr<ASTNode>& node);
    std::string generateBinaryOp(const std::shared_ptr<ASTNode>& node);
    std::string generateUnaryOp(const std::shared_ptr<ASTNode>& node);
    std::string generatePostfixOp(const std::shared_ptr<ASTNode>& node);
    std::string generatePrefixOp(const std::shared_ptr<ASTNode>& node);
    std::string generateFunctionCall(const std::shared_ptr<ASTNode>& node);
    
    void emit(const TACInstruction& instr);
    void emit(const std::string& op, const std::string& result = "", 
              const std::string& arg1 = "", const std::string& arg2 = "");
};

#endif