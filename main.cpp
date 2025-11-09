#include "scope_analyzer.h"
#include "parser.h"
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
            if (sum > 10 || flag) {
                ratio--;
                return sum;
            } else {
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
    }
    catch (const ScopeException& e) {
        std::cerr << "Scope Error: " << e.what() << std::endl;
    }
    catch (const ParseError& e) {
        std::cerr << "Parse Error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
