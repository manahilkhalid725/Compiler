#include "scope_analyzer.h"
#include "ir_generator.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream file("program.txt"); 
    if (!file) {
        std::cerr << "Failed to open program.txt\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string program = buffer.str();

    Scanner scan(program);
    Parser parser(scan);

    try {
        std::cout << "=== PARSING ===" << std::endl;
        auto ast = parser.parseProgram();
        std::cout << "\nParsing completed successfully.\n";
        
        std::cout << "\n=== AST STRUCTURE ===" << std::endl;
        ast->print();

        std::cout << "\n=== SCOPE ANALYSIS ===" << std::endl;
        ScopeAnalyzer sa;
        sa.analyze(ast);
        std::cout << "No scope errors detected.\n";
        
        std::cout << "\n=== IR GENERATION ===" << std::endl;
        IRGenerator irGen;
        irGen.generate(ast);
        irGen.printIR();
        
        std::cout << "\nCompilation completed successfully!\n";
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}