#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>

struct ASTNode {
    std::string kind;
    std::string val;
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(const std::string& k, const std::string& v = "") : kind(k), val(v) {}

    void addChild(std::shared_ptr<ASTNode> child) {
        children.push_back(child);
    }

    void print(int indent = 0) {
        for (int i = 0; i < indent; ++i) std::cout << "  ";
        std::cout << kind;
        if (!val.empty()) std::cout << "(" << val << ")";
        std::cout << std::endl;
        for (auto& c : children) c->print(indent + 1);
    }
};
