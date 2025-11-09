#ifndef SCOPE_ANALYZER_H
#define SCOPE_ANALYZER_H

#include "ast.h"
#include <string>
#include <stack>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <iostream>

class ScopeException : public std::exception 
{
    std::string message;
public:
    explicit ScopeException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

struct Symbol {
    std::string name;
    std::string type;
    bool isFunction;
    Symbol(std::string n = "", std::string t = "", bool f = false)
        : name(std::move(n)), type(std::move(t)), isFunction(f) {}
};

class ScopeAnalyzer {
public:
    void analyze(const std::shared_ptr<ASTNode>& root);

private:
    std::stack<std::unordered_map<std::string, Symbol>> scopeStack;

    void enterScope();
    void exitScope();
    void declareSymbol(const Symbol& sym);
    const Symbol* lookupSymbol(const std::string& name);
    void analyzeNode(const std::shared_ptr<ASTNode>& node);
};

#endif
