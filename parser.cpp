#include "parser.h"
#include <iostream>

int main() {
    std::string program = R"(
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
            }
            else 
            {
                return 0;
            }
        }
    )";
    Scanner scan(program);
    Parser parser(scan);

    try {
        auto ast = parser.parseProgram();
        std::cout << "AST:" << std::endl;
        ast->print();
    } catch (const ParseError& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
