#include "scope_analyzer.h"
#include "parser.h"
#include "type_checker.h"
#include <iostream>
#include <string>

int main() {
    string program = R"(
        fn int calculate(int a, int b)
        {
            int sum = a + b;
            float ratio = 0.0;
            bool flag = true;
            sum++;
            ratio = sum / 2.0;
            if (sum > 10 || flag) 
            {
                ratio--;
                return sum;
            } else 
            {
                return 0;
            }
        }
    )";

    Scanner scan(program);
    Parser parser(scan);

    try {
        auto ast = parser.parseProgram();
        std::cout << "AST:\n";
        ast->print();

        std::cout << "Performing Scope Analysis \n";
        ScopeAnalyzer sa;
        sa.analyze(ast);
        std::cout << "\n No scope errors detected.\n";

        std::cout << "Performing Type Checking \n";
        TypeChecker tc;
        tc.analyze(ast);
        std::cout << "\n No type errors detected.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
