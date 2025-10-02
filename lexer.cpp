#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <map>
using namespace std;

struct LexItem 
{
    string kind;
    string val;
};

class Scanner 
{
    string text;
    int idx;
    map<string,string> tokMap;

public:
    Scanner(const string& src) 
    {
        text = src;
        idx = 0;
        tokMap = {
            {"fn", "T_FUNCTION"}, {"int", "T_INT"}, {"float", "T_FLOAT"}, {"bool", "T_BOOL"}, {"string", "T_STRING"},
            {"if", "T_IF"}, {"else", "T_ELSE"}, {"while", "T_WHILE"}, {"for", "T_FOR"}, {"return", "T_RETURN"},
            {"true", "T_BOOLLIT"}, {"false", "T_BOOLLIT"},
        };
    }

    bool endOfFile() { return idx >= text.size(); }
    void eatSpaces() { while (!endOfFile() && isspace(text[idx])) idx++; }
    char peekChar() { return endOfFile() ? '\0' : text[idx]; }
    char takeChar() { return endOfFile() ? '\0' : text[idx++]; }

    LexItem readIdentOrKey() 
    {
        int start = idx;
        while (!endOfFile() && (isalnum(peekChar()) || peekChar() == '_')) takeChar();
        string word = text.substr(start, idx - start);
        if (tokMap.count(word)) return {tokMap[word], word};
        return {"T_IDENTIFIER", word};
    }

    LexItem readNumber()
     {
        int start = idx;
        while (!endOfFile() && isdigit(peekChar())) takeChar();

        bool isFloat = false;
        if (!endOfFile() && peekChar() == '.') 
        {
            isFloat = true;
            takeChar();
            while (!endOfFile() && isdigit(peekChar())) takeChar();
        }

        if (!endOfFile() && (isalpha(peekChar()) || peekChar() == '_')) 
        {
            int badStart = start;
            while (!endOfFile() && (isalnum(peekChar()) || peekChar() == '_')) takeChar();
            string inval = text.substr(badStart, idx - badStart);
            throw runtime_error("Invalid identifier: '" + inval + "'");
        }

        string num = text.substr(start, idx - start);
        return {isFloat ? "T_FLOATLIT" : "T_INTLIT", num};
    }

    LexItem readString() 
    {
        takeChar(); 
        int start = idx;
        while (!endOfFile() && peekChar() != '"')
        {
            if (peekChar() == '\\') takeChar();
            takeChar();
        }
        if (endOfFile()) throw runtime_error("Unterminated string literal");
        string content = text.substr(start, idx - start);
        takeChar();
        return {"T_STRINGLIT", content};
    }

    LexItem readComment()
    {
        takeChar();
        if (peekChar() == '/') 
        {
            while (!endOfFile() && peekChar() != '\n') takeChar();
            return {"T_COMMENT", ""};
        } else if (peekChar() == '*') 
        {
            takeChar();
            while (!endOfFile()) {
                if (peekChar() == '*' && idx + 1 < text.size() && text[idx + 1] == '/') 
                {
                    idx += 2;
                    return {"T_COMMENT", ""};
                }
                takeChar();
            }
            throw runtime_error("Unterminated block comment");
        }
        return {"T_DIV", "/"};
    }

    LexItem nextTok() 
    {
        eatSpaces();
        if (endOfFile()) return {"T_EOF", ""};
        char c = peekChar();

        if (isalpha(c) || c == '_') return readIdentOrKey();
        if (isdigit(c)) return readNumber();
        if (c == '"') return readString();
        if (c == '/') {
            if (idx + 1 < text.size() && text[idx + 1] == '=') 
            {
                idx += 2; return {"T_DIV_ASSIGN", "/="};
            }
            return readComment();
        }

        if (c == '=' && idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_EQUALSOP", "=="}; }
        if (c == '!' && idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_NOTEQOP", "!="}; }
        if (c == '<' && idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_LEQOP", "<="}; }
        if (c == '>' && idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_GEQOP", ">="}; }
        if (c == '&' && idx + 1 < text.size() && text[idx + 1] == '&') { idx += 2; return {"T_AND", "&&"}; }
        if (c == '|' && idx + 1 < text.size() && text[idx + 1] == '|') { idx += 2; return {"T_OR", "||"}; }

        if (c == '+') 
        {
            if (idx + 1 < text.size() && text[idx + 1] == '+') { idx += 2; return {"T_INCREMENT", "++"}; }
            if (idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_PLUS_ASSIGN", "+="}; }
            takeChar(); return {"T_PLUS", "+"};
        }
        if (c == '-') 
        {
            if (idx + 1 < text.size() && text[idx + 1] == '-') { idx += 2; return {"T_DECREMENT", "--"}; }
            if (idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_MINUS_ASSIGN", "-="}; }
            takeChar(); return {"T_MINUS", "-"};
        }
        if (c == '*') 
        {
            if (idx + 1 < text.size() && text[idx + 1] == '=') { idx += 2; return {"T_MUL_ASSIGN", "*="}; }
            takeChar(); return {"T_MUL", "*"};
        }

        if (c == '=') { takeChar(); return {"T_ASSIGNOP", "="}; }
        else if (c == '<') { takeChar(); return {"T_LESSOP", "<"}; }
        else if (c == '>') { takeChar(); return {"T_GREATOP", ">"}; }
        else if (c == '(') { takeChar(); return {"T_PARENL", "("}; }
        else if (c == ')') { takeChar(); return {"T_PARENR", ")"}; }
        else if (c == '{') { takeChar(); return {"T_BRACEL", "{"}; }
        else if (c == '}') { takeChar(); return {"T_BRACER", "}"}; }
        else if (c == '[') { takeChar(); return {"T_BRACKL", "["}; }
        else if (c == ']') { takeChar(); return {"T_BRACKR", "]"}; }
        else if (c == ',') { takeChar(); return {"T_COMMA", ","}; }
        else if (c == ';') { takeChar(); return {"T_SEMICOLON", ";"}; }
        else if (c == '"') { takeChar(); return {"T_QUOTES", "\""}; }

        throw runtime_error("Unknown token at: " + string(1, c));
    }
};

inline string tokToStr(LexItem t) 
{
    if (t.kind == "T_IDENTIFIER") return "T_IDENTIFIER(\"" + t.val + "\")";
    if (t.kind == "T_INTLIT") return "T_INTLIT(" + t.val + ")";
    if (t.kind == "T_FLOATLIT") return "T_FLOATLIT(" + t.val + ")";
    if (t.kind == "T_STRINGLIT") return "T_STRINGLIT(" + t.val + ")";
    if (t.kind == "T_BOOLLIT") return "T_BOOLLIT(" + t.val + ")";
    return t.kind;
}