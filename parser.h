#pragma once
#include "lexer.cpp"
#include "ast.h"
#include <memory>
#include <stdexcept>
#include <string>

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
    Scanner& scan;
    LexItem current;

    void next() { current = scan.nextTok(); }

    void expect(const std::string& kind) {
        if (current.kind != kind)
            throw ParseError("Expected " + kind + ", got " + current.kind);
        next();
    }

public:
    Parser(Scanner& s) : scan(s) { next(); }

    std::shared_ptr<ASTNode> parseProgram() {
        auto root = std::make_shared<ASTNode>("Program");
        while (current.kind != "T_EOF") {
            root->addChild(parseFunction());
        }
        return root;
    }

private:
    std::shared_ptr<ASTNode> parseFunction() {
        auto fnNode = std::make_shared<ASTNode>("FunctionDecl");
        expect("T_FUNCTION");

        if (current.kind != "T_INT" && current.kind != "T_FLOAT" &&
            current.kind != "T_BOOL" && current.kind != "T_STRING")
            throw ParseError("ExpectedTypeToken");

        fnNode->addChild(std::make_shared<ASTNode>("Type", current.val));
        next();

        if (current.kind != "T_IDENTIFIER")
            throw ParseError("ExpectedIdentifier");

        fnNode->addChild(std::make_shared<ASTNode>("Name", current.val));
        fnNode->val = current.val;  // FIX: store function name in val
        next();

        expect("T_PARENL");
        fnNode->addChild(parseParams());
        expect("T_PARENR");

        fnNode->addChild(parseBlock());
        return fnNode;
    }

    std::shared_ptr<ASTNode> parseParams() {
        auto params = std::make_shared<ASTNode>("Params");
        while (current.kind != "T_PARENR") {
            if (current.kind != "T_INT" && current.kind != "T_FLOAT" &&
                current.kind != "T_BOOL" && current.kind != "T_STRING")
                throw ParseError("ExpectedTypeToken");

            std::string type = current.val;
            next();

            if (current.kind != "T_IDENTIFIER")
                throw ParseError("ExpectedIdentifier");

            std::string name = current.val;
            next();

            auto paramNode = std::make_shared<ASTNode>("Param", name);
            paramNode->addChild(std::make_shared<ASTNode>("Type", type));
            params->addChild(paramNode);

            if (current.kind == "T_COMMA") next();
            else break;
        }
        return params;
    }

    std::shared_ptr<ASTNode> parseBlock() {
        expect("T_BRACEL");
        auto block = std::make_shared<ASTNode>("Block");
        while (current.kind != "T_BRACER") {
            block->addChild(parseStatement());
        }
        expect("T_BRACER");
        return block;
    }

    std::shared_ptr<ASTNode> parseStatement() {
        if (current.kind == "T_IF") return parseIf();
        if (current.kind == "T_RETURN") return parseReturn();
        if (current.kind == "T_IDENTIFIER") return parseAssignmentOrExpr();
        if (current.kind == "T_INT" || current.kind == "T_FLOAT" ||
            current.kind == "T_BOOL" || current.kind == "T_STRING")
            return parseVarDecl();

        throw ParseError("Expected expression or statement");
    }

    std::shared_ptr<ASTNode> parseVarDecl() {
        std::string varName;
        auto typeNode = std::make_shared<ASTNode>("Type", current.val);
        next();

        if (current.kind != "T_IDENTIFIER")
            throw ParseError("ExpectedIdentifier");

        varName = current.val;
        auto idNode = std::make_shared<ASTNode>("Identifier", current.val);
        next();

        auto declNode = std::make_shared<ASTNode>("VarDecl", varName);
        declNode->addChild(typeNode);
        declNode->addChild(idNode);

        if (current.kind == "T_ASSIGNOP") {
            next();
            declNode->addChild(parseExpr());
        }

        expect("T_SEMICOLON");
        return declNode;
    }

    std::shared_ptr<ASTNode> parseIf() {
        auto ifNode = std::make_shared<ASTNode>("IfStmt");
        next();
        expect("T_PARENL");
        ifNode->addChild(parseExpr());
        expect("T_PARENR");
        ifNode->addChild(parseBlock());
        if (current.kind == "T_ELSE") {
            next();
            ifNode->addChild(parseBlock());
        }
        return ifNode;
    }

    std::shared_ptr<ASTNode> parseReturn() {
        auto retNode = std::make_shared<ASTNode>("ReturnStmt");
        next();
        retNode->addChild(parseExpr());
        expect("T_SEMICOLON");
        return retNode;
    }

    std::shared_ptr<ASTNode> parseAssignmentOrExpr() {
        auto idNode = std::make_shared<ASTNode>("Identifier", current.val);
        next();

        if (current.kind == "T_INCREMENT" || current.kind == "T_DECREMENT") {
            std::string op = current.val;
            next();
            auto postfixNode = std::make_shared<ASTNode>("PostfixOp", op);
            postfixNode->addChild(idNode);
            expect("T_SEMICOLON");
            return postfixNode;
        }

        if (current.kind == "T_ASSIGNOP") {
            next();
            auto assignNode = std::make_shared<ASTNode>("Assign");
            assignNode->addChild(idNode);
            assignNode->addChild(parseExpr());
            expect("T_SEMICOLON");
            return assignNode;
        }

        throw ParseError("Expected assignment operator or postfix operator");
    }

    std::shared_ptr<ASTNode> parseExprTail(std::shared_ptr<ASTNode> left) {
        while (current.kind == "T_PLUS" || current.kind == "T_MINUS" ||
               current.kind == "T_MUL" || current.kind == "T_DIV" ||
               current.kind == "T_EQUALSOP" || current.kind == "T_NOTEQOP" ||
               current.kind == "T_LESSOP" || current.kind == "T_GREATOP" ||
               current.kind == "T_LEQOP" || current.kind == "T_GEQOP" ||
               current.kind == "T_AND" || current.kind == "T_OR") 
        {
            std::string op = current.val;
            next();
            auto right = parsePrimary();
            auto opNode = std::make_shared<ASTNode>("BinaryOp", op);
            opNode->addChild(left);
            opNode->addChild(right);
            left = opNode;
        }
        return left;
    }

    std::shared_ptr<ASTNode> parseExpr() {
        auto left = parsePrimary();
        return parseExprTail(left);
    }

    std::shared_ptr<ASTNode> parsePrimary() {
        std::shared_ptr<ASTNode> node;

        if (current.kind == "T_IDENTIFIER") {
            node = std::make_shared<ASTNode>("Identifier", current.val);
            next();
        } else if (current.kind == "T_INTLIT" || current.kind == "T_FLOATLIT" ||
                   current.kind == "T_STRINGLIT" || current.kind == "T_BOOLLIT") {
            node = std::make_shared<ASTNode>("Literal", current.val);
            next();
        } else if (current.kind == "T_PARENL") {
            next();
            node = parseExpr();
            expect("T_PARENR");
        } else {
            throw ParseError("ExpectedExpr");
        }

        while (current.kind == "T_INCREMENT" || current.kind == "T_DECREMENT") {
            std::string op = current.val;
            next();
            auto opNode = std::make_shared<ASTNode>("PostfixOp", op);
            opNode->addChild(node);
            node = opNode;
        }

        return node;
    }
};
