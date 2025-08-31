#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

using namespace std;

struct TokenObj 
{
    string kind;
    string text;
};

class Tokenizer 
{
    string program;
    int index;

    vector<pair<string, regex>> patterns = 
    {
        {"FUNCTION", regex("^fn\\b")},
        {"INT", regex("^int\\b")},
        {"FLOAT", regex("^float\\b")},
        {"BOOL", regex("^bool\\b")},
        {"STRING", regex("^string\\b")},
        {"IF", regex("^if\\b")},
        {"ELSE", regex("^else\\b")},
        {"WHILE", regex("^while\\b")},
        {"FOR", regex("^for\\b")},
        {"RETURN", regex("^return\\b")},
        {"BOOLLIT", regex("^(true|false)\\b")},
        {"FLOATLIT", regex("^[0-9]+\\.[0-9]+")},
        {"INVALID", regex("^[0-9][a-zA-Z_][a-zA-Z0-9_]*")},
        {"INTLIT", regex("^[0-9]+")},
        {"STRINGLIT", regex("^\"([^\"\\\\]|\\\\.)*\"")},
        {"UNTERMINATED_STRING", regex("^\"([^\"\\\\]|\\\\.)*$")},
        {"IDENTIFIER", regex("^[a-zA-Z_][a-zA-Z0-9_]*")},
        {"EQUALSOP", regex("^==")},
        {"NOTEQOP", regex("^!=")},
        {"LEQOP", regex("^<=")},
        {"GEQOP", regex("^>=")},
        {"AND", regex("^&&")},
        {"OR", regex("^\\|\\|")},
        {"PLUS_ASSIGN", regex("^\\+=")},
        {"MINUS_ASSIGN", regex("^-=")},
        {"MUL_ASSIGN", regex("^\\*=")},
        {"DIV_ASSIGN", regex("^/=")},
        {"INCREMENT", regex("^\\+\\+")},
        {"DECREMENT", regex("^--")},
        {"ASSIGNOP", regex("^=")},
        {"LESSOP", regex("^<")},
        {"GREATOP", regex("^>")},
        {"PLUS", regex("^\\+")},
        {"MINUS", regex("^-")},
        {"MUL", regex("^\\*")},
        {"DIV", regex("^/")},
        {"PARENL", regex("^\\(")},
        {"PARENR", regex("^\\)")},
        {"BRACEL", regex("^\\{")},
        {"BRACER", regex("^\\}")},
        {"BRACKL", regex("^\\[")},
        {"BRACKR", regex("^\\]")},
        {"COMMA", regex("^,")},
        {"SEMICOLON", regex("^;")},
        {"QUOTES", regex("^\"")},
        {"COMMENT", regex("^(//.*|/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/)")},
    };

public:
    Tokenizer(const string& src) 
    {
        program = src;
        index = 0;
    }

    bool reachedEnd() 
    {
        return index >= program.size();
    }

    void ignoreSpaces() 
    {
        while (!reachedEnd() && isspace(program[index])) index++;
    }

    TokenObj nextToken() 
    {
        ignoreSpaces();
        if (reachedEnd()) return {"EOF", ""};
        string rest = program.substr(index);
        smatch found;
        string bestType = "INVALID";
        size_t bestLen = 0;
        smatch bestMatch;

        for (auto& [kind, pattern] : patterns) 
        {
            smatch trial;
            if (regex_search(rest, trial, pattern)) 
            {
                string value = trial.str();
                if (value.size() > bestLen) {
                    bestLen = value.size();
                    bestMatch = trial;
                    bestType = kind;
                }
            }
        }

        if (bestLen > 0) 
        {
            index += bestLen;
            if (bestType == "COMMENT") return nextToken();
            if (bestType == "UNTERMINATED_STRING")
                throw runtime_error("Unterminated string literal starting at: " + rest.substr(0, 10));
            if (bestType == "INVALID")
                throw runtime_error("Invalid identifier: " + rest.substr(0, 10));
            return {bestType, bestMatch.str()};
        }

        throw runtime_error("Unknown token starting at: " + rest.substr(0, 10));
    }
};

string describeToken(const string& kind, const string& val = "") 
{
    if (kind == "FUNCTION") return "T_FUNCTION";
    else if (kind == "INT") return "T_INT";
    else if (kind == "FLOAT") return "T_FLOAT";
    else if (kind == "BOOL") return "T_BOOL";
    else if (kind == "STRING") return "T_STRING";
    else if (kind == "IF") return "T_IF";
    else if (kind == "ELSE") return "T_ELSE";
    else if (kind == "WHILE") return "T_WHILE";
    else if (kind == "FOR") return "T_FOR";
    else if (kind == "RETURN") return "T_RETURN";
    else if (kind == "IDENTIFIER") return "T_IDENTIFIER(\"" + val + "\")";
    else if (kind == "INTLIT") return "T_INTLIT(" + val + ")";
    else if (kind == "FLOATLIT") return "T_FLOATLIT(" + val + ")";
    else if (kind == "STRINGLIT") return "T_STRINGLIT(" + val + ")";
    else if (kind == "BOOLLIT") return "T_BOOLLIT(" + val + ")";
    else if (kind == "ASSIGNOP") return "T_ASSIGNOP";
    else if (kind == "EQUALSOP") return "T_EQUALSOP";
    else if (kind == "NOTEQOP") return "T_NOTEQOP";
    else if (kind == "LESSOP") return "T_LESSOP";
    else if (kind == "GREATOP") return "T_GREATOP";
    else if (kind == "LEQOP") return "T_LEQOP";
    else if (kind == "GEQOP") return "T_GEQOP";
    else if (kind == "AND") return "T_AND";
    else if (kind == "OR") return "T_OR";
    else if (kind == "PLUS") return "T_PLUS";
    else if (kind == "MINUS") return "T_MINUS";
    else if (kind == "MUL") return "T_MUL";
    else if (kind == "DIV") return "T_DIV";
    else if (kind == "PARENL") return "T_PARENL";
    else if (kind == "PARENR") return "T_PARENR";
    else if (kind == "BRACEL") return "T_BRACEL";
    else if (kind == "BRACER") return "T_BRACER";
    else if (kind == "BRACKL") return "T_BRACKL";
    else if (kind == "BRACKR") return "T_BRACKR";
    else if (kind == "COMMA") return "T_COMMA";
    else if (kind == "SEMICOLON") return "T_SEMICOLON";
    else if (kind == "QUOTES") return "T_QUOTES";
    else if (kind == "COMMENT") return "T_COMMENT";
    else if (kind == "INVALID") return "T_INVALID";
    else if (kind == "PLUS_ASSIGN") return "T_PLUS_ASSIGN";
    else if (kind == "MINUS_ASSIGN") return "T_MINUS_ASSIGN";
    else if (kind == "MUL_ASSIGN") return "T_MUL_ASSIGN";
    else if (kind == "DIV_ASSIGN") return "T_DIV_ASSIGN";
    else if (kind == "INCREMENT") return "T_INCREMENT";
    else if (kind == "DECREMENT") return "T_DECREMENT";
    else if (kind == "UNTERMINATED_STRING") return "T_UNTERMINATED_STRING";
    else if (kind == "EOF") return "T_EOF";
    return "UNKNOWN";
}

int main() 
{
    string snippet = R"(
        fn int my_fn(int x, float y) 
        {
            string my_str = "hmm\n";
            bool my_bool = x == 40;
            if (x != 0 && y >= 2.5) 
            {
                y+=20;
                return x;
            }
        }
    )";

    Tokenizer scanner(snippet);
    try {
        while (true) 
        {
            TokenObj tk = scanner.nextToken();
            if (tk.kind == "EOF") break;
            cout << describeToken(tk.kind, tk.text) << endl;
        }
    } catch (exception& e) {
        cerr << "Lexer error: " << e.what() << endl;
    }
    return 0;
}
