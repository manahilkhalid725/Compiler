#include "scope_analyzer.h"

void ScopeAnalyzer::analyze(const std::shared_ptr<ASTNode>& root) 
{
    if (!root) return;
    std::cout << "\n[ScopeAnalyzer] Starting scope analysis\n";
    enterScope();
    analyzeNode(root);
    exitScope();
    std::cout << "[ScopeAnalyzer] Scope analysis finished successfully.\n";
}

void ScopeAnalyzer::enterScope() 
{
    scopeStack.push({});
}

void ScopeAnalyzer::exitScope() 
{
    if (!scopeStack.empty())
        scopeStack.pop();
}

void ScopeAnalyzer::declareSymbol(const Symbol& sym) 
{
    auto& current = scopeStack.top();
    if (current.find(sym.name) != current.end()) 
    {
        if (sym.isFunction)
            throw ScopeException("Function redefinition: " + sym.name);
        else
            throw ScopeException("Variable redefinition: " + sym.name);
    }
    current[sym.name] = sym;
}

const Symbol* ScopeAnalyzer::lookupSymbol(const std::string& name) 
{
    std::stack<std::unordered_map<std::string, Symbol>> temp = scopeStack;
    while (!temp.empty()) 
    {
        auto& scope = temp.top();
        if (scope.find(name) != scope.end())
            return &scope[name];
        temp.pop();
    }
    return nullptr;
}

void ScopeAnalyzer::analyzeNode(const std::shared_ptr<ASTNode>& node) 
{
    if (!node) return;
    if (node->kind == "FunctionDecl") 
    {
        declareSymbol(Symbol(node->val, "function", true));
        enterScope();
        for (auto& child : node->children) 
        {
            if (child->kind == "Params") 
            {
                for (auto& param : child->children) 
                {
                    declareSymbol(Symbol(param->val, "variable"));
                }
            }
        }
        for (auto& child : node->children)
            analyzeNode(child);

        exitScope();
        return; 
    }

    else if (node->kind == "Block") 
    {
        enterScope();
        for (auto& child : node->children)
            analyzeNode(child);
        exitScope();
        return;
    }

    else if (node->kind == "VarDecl") 
    {
        declareSymbol(Symbol(node->val, "variable"));
        for (auto& child : node->children)
            analyzeNode(child);

        return;
    }

    else if (node->kind == "Identifier") 
    {
        const Symbol* sym = lookupSymbol(node->val);
        if (!sym)
            throw ScopeException("Undeclared variable accessed: " + node->val);
    }

    else if (node->kind == "FunctionCall") 
    {
        const Symbol* sym = lookupSymbol(node->val);
        if (!sym || !sym->isFunction)
            throw ScopeException("Undefined function called: " + node->val);
    }

    for (auto& child : node->children)
        analyzeNode(child);
}
