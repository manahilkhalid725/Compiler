#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <map>
#include <memory>
#include <sstream>
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
            // optional common extras
            {"break","T_BREAK"}, {"continue","T_CONTINUE"}
        };
    }

    bool endOfFile() { return idx >= (int)text.size(); }
    void eatSpaces() { while (!endOfFile() && isspace((unsigned char)text[idx])) idx++; }
    char peekChar() { return endOfFile() ? '\0' : text[idx]; }
    char takeChar() { return endOfFile() ? '\0' : text[idx++]; }

    LexItem readIdentOrKey() 
    {
        int start = idx;
        while (!endOfFile() && (isalnum((unsigned char)peekChar()) || peekChar() == '_')) takeChar();
        string word = text.substr(start, idx - start);
        if (tokMap.count(word)) return {tokMap[word], word};
        return {"T_IDENTIFIER", word};
    }

    LexItem readNumber()
     {
        int start = idx;
        while (!endOfFile() && isdigit((unsigned char)peekChar())) takeChar();

        bool isFloat = false;
        if (!endOfFile() && peekChar() == '.') 
        {
            isFloat = true;
            takeChar();
            while (!endOfFile() && isdigit((unsigned char)peekChar())) takeChar();
        }

        if (!endOfFile() && (isalpha((unsigned char)peekChar()) || peekChar() == '_')) 
        {
            int badStart = start;
            while (!endOfFile() && (isalnum((unsigned char)peekChar()) || peekChar() == '_')) takeChar();
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
            if (peekChar() == '\\') { takeChar(); if(!endOfFile()) takeChar(); } else takeChar();
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
                if (peekChar() == '*' && idx + 1 < (int)text.size() && text[idx + 1] == '/') 
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

        if (isalpha((unsigned char)c) || c == '_') return readIdentOrKey();
        if (isdigit((unsigned char)c)) return readNumber();
        if (c == '"') return readString();
        if (c == '/') {
            if (idx + 1 < (int)text.size() && text[idx + 1] == '=') 
            {
                idx += 2; return {"T_DIV_ASSIGN", "/="};
            }
            return readComment();
        }

        if (c == '=' && idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_EQUALSOP", "=="}; }
        if (c == '!' && idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_NOTEQOP", "!="}; }
        if (c == '<' && idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_LEQOP", "<="}; }
        if (c == '>' && idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_GEQOP", ">="}; }
        if (c == '&' && idx + 1 < (int)text.size() && text[idx + 1] == '&') { idx += 2; return {"T_AND", "&&"}; }
        if (c == '|' && idx + 1 < (int)text.size() && text[idx + 1] == '|') { idx += 2; return {"T_OR", "||"}; }

        if (c == '+') 
        {
            if (idx + 1 < (int)text.size() && text[idx + 1] == '+') { idx += 2; return {"T_INCREMENT", "++"}; }
            if (idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_PLUS_ASSIGN", "+="}; }
            takeChar(); return {"T_PLUS", "+"};
        }
        if (c == '-') 
        {
            if (idx + 1 < (int)text.size() && text[idx + 1] == '-') { idx += 2; return {"T_DECREMENT", "--"}; }
            if (idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_MINUS_ASSIGN", "-="}; }
            takeChar(); return {"T_MINUS", "-"};
        }
        if (c == '*') 
        {
            if (idx + 1 < (int)text.size() && text[idx + 1] == '=') { idx += 2; return {"T_MUL_ASSIGN", "*="}; }
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
        else if (c == '.') { takeChar(); return {"T_DOT", "."}; }
        else if (c == '"') { takeChar(); return {"T_QUOTES", "\""}; }

        throw runtime_error(string("Unknown token at: ") + string(1, c));
    }
};

string tokToStr(const LexItem &t) 
{
    if (t.kind == "T_IDENTIFIER") return "T_IDENTIFIER(\"" + t.val + "\")";
    if (t.kind == "T_INTLIT") return "T_INTLIT(" + t.val + ")";
    if (t.kind == "T_FLOATLIT") return "T_FLOATLIT(" + t.val + ")";
    if (t.kind == "T_STRINGLIT") return "T_STRINGLIT(" + t.val + ")";
    if (t.kind == "T_BOOLLIT") return "T_BOOLLIT(" + t.val + ")";
    return t.kind + (t.val.empty() ? "" : string("(") + t.val + ")");
}


enum class ParseError {
    UnexpectedEOF,
    FailedToFindToken,
    ExpectedTypeToken,
    ExpectedIdentifier,
    UnexpectedToken,
    ExpectedFloatLit,
    ExpectedIntLit,
    ExpectedStringLit,
    ExpectedBoolLit,
    ExpectedExpr,
};

struct ParseException : public runtime_error {
    ParseError err;
    LexItem token;
    ParseException(ParseError e, const LexItem &t, const string &m="") : runtime_error(m), err(e), token(t) {}
};

struct Node {
    virtual ~Node() = default;
    virtual string str(int indent=0) const = 0;
};

struct Expr : Node { };

using ExprPtr = unique_ptr<Expr>;

struct LiteralExpr : Expr {
    string kind; 
    string value;
    LiteralExpr(const string& k,const string& v):kind(k),value(v){}
    string str(int) const override {
        return "Literal(" + kind + ":" + value + ")";
    }
};

struct IdentExpr : Expr {
    string name;
    IdentExpr(const string& n):name(n){}
    string str(int) const override { return "Ident(" + name + ")"; }
};

struct UnaryExpr : Expr {
    string op;
    ExprPtr rhs;
    UnaryExpr(string o, ExprPtr r):op(move(o)), rhs(move(r)){}
    string str(int) const override {
        return string("Unary(") + op + " " + rhs->str() + ")";
    }
};

struct BinaryExpr : Expr {
    string op;
    ExprPtr lhs, rhs;
    BinaryExpr(string o, ExprPtr l, ExprPtr r):op(move(o)),lhs(move(l)),rhs(move(r)){}
    string str(int) const override {
        return "(" + lhs->str() + " " + op + " " + rhs->str() + ")";
    }
};

struct AssignExpr : Expr {
    string target;
    ExprPtr value;
    AssignExpr(string t, ExprPtr v):target(move(t)), value(move(v)){}
    string str(int) const override {
        return "Assign(" + target + " = " + value->str() + ")";
    }
};

struct CallExpr : Expr {
    string ident;
    vector<ExprPtr> args;
    CallExpr(string id):ident(move(id)){}
    string str(int) const override {
        string s = "Call(" + ident + "(";
        for(size_t i=0;i<args.size();++i){
            s += args[i]->str();
            if (i+1<args.size()) s += ", ";
        }
        s += "))";
        return s;
    }
};

struct Stmt : Node { };
using StmtPtr = unique_ptr<Stmt>;

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e):expr(move(e)){}
    string str(int) const override {
        return "ExprStmt(" + (expr ? expr->str() : string("None")) + ")";
    }
};

struct VarDecl : Stmt {
    string type_tok;
    string ident;
    ExprPtr init;
    VarDecl(string t, string id, ExprPtr i):type_tok(move(t)),ident(move(id)),init(move(i)){}
    string str(int) const override {
        string s = "VarDecl(" + type_tok + " " + ident;
        if (init) s += " = " + init->str();
        s += ")";
        return s;
    }
};

struct ReturnStmt : Stmt {
    ExprPtr expr;
    ReturnStmt(ExprPtr e):expr(move(e)){}
    string str(int) const override {
        return "Return(" + (expr?expr->str():"") + ")";
    }
};

struct IfStmt : Stmt {
    ExprPtr cond;
    vector<StmtPtr> if_block;
    vector<StmtPtr> else_block;
    IfStmt(ExprPtr c):cond(move(c)){}
    string str(int) const override {
        string s = "If(" + (cond?cond->str():"") + ") {";
        for(auto &st: if_block) s += "\n  " + st->str();
        s += "\n} Else {";
        for(auto &st: else_block) s += "\n  " + st->str();
        s += "\n}";
        return s;
    }
};

struct ForStmt : Stmt {

    unique_ptr<VarDecl> initVar;  
    StmtPtr initStmt;      
    ExprPtr cond;         
    ExprPtr update;          
    vector<StmtPtr> block;

    string str(int indent) const override {
        string pad(indent, ' ');
        string s = pad + "For(\n";

        if (initVar) s += initVar->str(indent + 2);
        else if (initStmt) s += initStmt->str(indent + 2);

        s += "\n" + string(indent + 2, ' ') + "Cond: ";
        s += (cond ? cond->str(indent + 4) : "None");

        s += "\n" + string(indent + 2, ' ') + "Update: ";
        s += (update ? update->str(indent + 4) : "None");

        s += "\n" + string(indent + 2, ' ') + "Block {";
        for (auto &st : block) {
            s += "\n" + st->str(indent + 4);
        }
        s += "\n" + string(indent + 2, ' ') + "}";

        s += "\n" + pad + ")";
        return s;
    }
};


struct BreakStmt : Stmt {
    string str(int) const override { return "Break"; }
};

struct BlockStmt : Stmt {
    vector<StmtPtr> sts;
    string str(int) const override {
        string s = "Block{";
        for(auto &st: sts) s += "\n  " + st->str();
        s += "\n}";
        return s;
    }
};

struct FnDecl : Stmt {
    string type_tok; 
    string ident;
    vector<pair<string,string>> params; 
    vector<StmtPtr> block;
    FnDecl(string t, string id):type_tok(move(t)), ident(move(id)){}
    string str(int) const override {
        string s = "FnDecl(" + type_tok + " " + ident + "(";
        for(size_t i=0;i<params.size();++i){
            s += params[i].first + " " + params[i].second;
            if (i+1<params.size()) s += ", ";
        }
        s += ")) ";
        for(auto &st: block) s += "\n  " + st->str();
        return s;
    }
};


class Parser {
    Scanner scanner;
    LexItem cur, peeked;
    bool hasPeek;
public:
    Parser(const string &src):scanner(src), hasPeek(false) {
        advance();
    }

    void advance() {
        try {
            cur = scanner.nextTok();
        } catch (exception &e) {
            throw;
        }
    }

    const LexItem& curTok() const { return cur; }

    bool check(const string &k) const { return cur.kind == k; }

    bool match(const string &k) {
        if (check(k)) { advance(); return true; }
        return false;
    }

    void expect(const string &k, ParseError err = ParseError::UnexpectedToken) {
        if (!check(k)) throw ParseException(err, cur, string("Expected token ") + k + " but got " + cur.kind);
        advance();
    }

    vector<StmtPtr> parseProgram() {
        vector<StmtPtr> out;
        while (!check("T_EOF")) {

            if (check("T_COMMENT")) { advance(); continue; }
            if (check("T_FUNCTION")) {
                out.push_back(parseFunction());
                continue;
            }

            if (isTypeToken(cur.kind)) {
                out.push_back(parseVarDeclTop());
                continue;
            }

            out.push_back(parseStatement());
        }
        return out;
    }

private:
    static bool isTypeToken(const string &k) {
        return k == "T_INT" || k == "T_FLOAT" || k == "T_BOOL" || k == "T_STRING";
    }

    StmtPtr parseFunction() {

        expect("T_FUNCTION");
        if (!isTypeToken(cur.kind)) throw ParseException(ParseError::ExpectedTypeToken, cur, "Function return type expected");
        string retType = cur.kind; advance();
        if (!check("T_IDENTIFIER")) throw ParseException(ParseError::ExpectedIdentifier, cur, "Function name expected");
        string name = cur.val; advance();
        expect("T_PARENL");
        vector<pair<string,string>> params;
        if (!check("T_PARENR")) {

            while (true) {
                if (!isTypeToken(cur.kind)) throw ParseException(ParseError::ExpectedTypeToken, cur, "Param type expected");
                string ptype = cur.kind; advance();
                if (!check("T_IDENTIFIER")) throw ParseException(ParseError::ExpectedIdentifier, cur, "Param name expected");
                string pname = cur.val; advance();
                params.emplace_back(ptype, pname);
                if (check("T_COMMA")) { advance(); continue; }
                break;
            }
        }
        expect("T_PARENR");
        // block
        auto fn = make_unique<FnDecl>(retType, name);
        fn->params = move(params);
        auto block = parseBlockInternal();
        fn->block = move(block);
        return fn;
    }

    StmtPtr parseVarDeclTop() {
        auto vd = parseVarDeclInternal();
        if (check("T_SEMICOLON") || check("T_DOT")) advance();
        return make_unique<VarDecl>(move(*vd));
    }

    unique_ptr<VarDecl> parseVarDeclInternal() {
        if (!isTypeToken(cur.kind)) throw ParseException(ParseError::ExpectedTypeToken, cur, "Type expected for var decl");
        string t = cur.kind; advance();
        if (!check("T_IDENTIFIER")) throw ParseException(ParseError::ExpectedIdentifier, cur, "Identifier expected in var decl");
        string id = cur.val; advance();
        ExprPtr init = nullptr;
        if (check("T_ASSIGNOP")) {
            advance();
            init = parseExpression();
        }
        return make_unique<VarDecl>(t, id, move(init));
    }

    StmtPtr parseStatement() {
        if (isTypeToken(cur.kind)) {
            auto vd = parseVarDeclInternal();
            if (check("T_SEMICOLON") || check("T_DOT")) advance();
            return make_unique<VarDecl>(move(*vd));
        }
        if (check("T_RETURN")) {
            advance();
            ExprPtr ex = nullptr;
            if (!check("T_SEMICOLON") && !check("T_DOT")) {
                ex = parseExpression();
            }
            if (check("T_SEMICOLON") || check("T_DOT")) advance();
            return make_unique<ReturnStmt>(move(ex));
        }
        if (check("T_IF")) return parseIfStmt();
        if (check("T_FOR")) return parseForStmt();
        if (check("T_BRACEL")) {
            auto blk = parseBlock();
            return blk;
        }
        if (check("T_BREAK")) { advance(); if (check("T_SEMICOLON") || check("T_DOT")) advance(); return make_unique<BreakStmt>(); }

        ExprPtr ex = parseExpression();
        if (!ex) throw ParseException(ParseError::ExpectedExpr, cur, "Expected expression in statement");
        if (check("T_SEMICOLON") || check("T_DOT")) advance();
        return make_unique<ExprStmt>(move(ex));
    }

    vector<StmtPtr> parseBlockInternal() {
        vector<StmtPtr> contents;
        if (!check("T_BRACEL")) throw ParseException(ParseError::UnexpectedToken, cur, "Block must start with {");
        advance(); 
        while (!check("T_BRACER") && !check("T_EOF")) {
            if (check("T_COMMENT")) { advance(); continue; }
            contents.push_back(parseStatement());
        }
        expect("T_BRACER");
        return contents;
    }

    StmtPtr parseBlock() {
        if (!check("T_BRACEL")) throw ParseException(ParseError::UnexpectedToken, cur, "Expected { for block");
        vector<StmtPtr> inner = parseBlockInternal();
        auto b = make_unique<BlockStmt>();
        b->sts = move(inner);
        return b;
    }

    StmtPtr parseIfStmt() {
        expect("T_IF");
        expect("T_PARENL");
        ExprPtr cond = parseExpression();
        expect("T_PARENR");
        StmtPtr thenBlock = parseStatement();
        vector<StmtPtr> thenVec;
        if (auto pb = dynamic_cast<BlockStmt*>(thenBlock.get())) {
            thenVec = move(pb->sts);
        } else {
            thenVec.push_back(move(thenBlock));
        }
        vector<StmtPtr> elseVec;
        if (check("T_ELSE")) {
            advance();
            StmtPtr elseBlock = parseStatement();
            if (auto eb = dynamic_cast<BlockStmt*>(elseBlock.get())) {
                elseVec = move(eb->sts);
            } else {
                elseVec.push_back(move(elseBlock));
            }
        }
        auto ifs = make_unique<IfStmt>(move(cond));
        ifs->if_block = move(thenVec);
        ifs->else_block = move(elseVec);
        return ifs;
    }

    StmtPtr parseForStmt() {
        expect("T_FOR");
        expect("T_PARENL");
        unique_ptr<VarDecl> initVar = nullptr;
        StmtPtr initStmt = nullptr;
        if (!check("T_SEMICOLON")) {
            if (isTypeToken(cur.kind)) {
                initVar = parseVarDeclInternal();
            } else {
                ExprPtr ex = parseExpression();
                initStmt = make_unique<ExprStmt>(move(ex));
            }
        }
        if (check("T_SEMICOLON")) advance();
        ExprPtr cond = nullptr;
        if (!check("T_SEMICOLON")) {
            cond = parseExpression();
        }
        if (check("T_SEMICOLON")) advance();
        ExprPtr upd = nullptr;
        if (!check("T_PARENR")) upd = parseExpression();
        expect("T_PARENR");
        StmtPtr blk = parseStatement();
        auto forS = make_unique<ForStmt>();
        forS->initVar = move(initVar);
        forS->initStmt = move(initStmt);
        forS->cond = move(cond);
        forS->update = move(upd);
        if (auto b = dynamic_cast<BlockStmt*>(blk.get())) {
            forS->block = move(b->sts);
        } else {
            forS->block.push_back(move(blk));
        }
        return forS;
    }

    int getBinaryPrecedence(const string &tok) {
        if (tok == "T_OR") return 1;
        if (tok == "T_AND") return 2;
        if (tok == "T_EQUALSOP" || tok == "T_NOTEQOP") return 3;
        if (tok == "T_LESSOP" || tok == "T_GREATOP" || tok == "T_LEQOP" || tok == "T_GEQOP") return 4;
        if (tok == "T_PLUS" || tok == "T_MINUS") return 5;
        if (tok == "T_MUL" || tok == "T_DIV" || tok == "T_MUL_ASSIGN" || tok=="T_DIV_ASSIGN") return 6;
        if (tok == "T_ASSIGNOP") return 0;
        return -1;
    }

    ExprPtr parseExpression() {
        return parseAssignment();
    }

    ExprPtr parseAssignment() {
        ExprPtr left = parseBinaryOpRHS(0, parseUnary());
        if (check("T_ASSIGNOP")) {
            auto id = dynamic_cast<IdentExpr*>(left.get());
            if (!id) throw ParseException(ParseError::UnexpectedToken, cur, "Left side of assignment must be identifier");
            advance();
            ExprPtr val = parseExpression();
            return make_unique<AssignExpr>(id->name, move(val));
        }
        return left;
    }
    ExprPtr parseUnary() {
        if (check("T_MINUS") || check("T_PLUS") || check("T_NOTEQOP") ) {
            string op = cur.kind; advance();
            ExprPtr rhs = parseUnary();
            return make_unique<UnaryExpr>(op, move(rhs));
        }
        return parsePrimary();
    }

    ExprPtr parsePrimary() {
        if (check("T_INTLIT")) {
            string v = cur.val; advance();
            return make_unique<LiteralExpr>("int", v);
        }
        if (check("T_FLOATLIT")) {
            string v = cur.val; advance();
            return make_unique<LiteralExpr>("float", v);
        }
        if (check("T_STRINGLIT")) {
            string v = cur.val; advance();
            return make_unique<LiteralExpr>("string", v);
        }
        if (check("T_BOOLLIT")) {
            string v = cur.val; advance();
            return make_unique<LiteralExpr>("bool", v);
        }
        if (check("T_IDENTIFIER")) {
            string id = cur.val; advance();
            // function call?
            if (check("T_PARENL")) {
                advance();
                auto call = make_unique<CallExpr>(id);
                if (!check("T_PARENR")) {
                    while (true) {
                        ExprPtr arg = parseExpression();
                        call->args.push_back(move(arg));
                        if (check("T_COMMA")) { advance(); continue; }
                        break;
                    }
                }
                expect("T_PARENR");
                return call;
            }
            return make_unique<IdentExpr>(id);
        }
        if (check("T_PARENL")) {
            advance();
            ExprPtr inside = parseExpression();
            expect("T_PARENR");
            return inside;
        }
        throw ParseException(ParseError::ExpectedExpr, cur, "Primary expression expected");
    }

    ExprPtr parseBinaryOpRHS(int exprPrec, ExprPtr lhs) {
        while (true) {
            int tokPrec = getBinaryPrecedence(cur.kind);
            if (tokPrec < exprPrec) return lhs;

            string binOp = cur.kind;
            advance();
            ExprPtr rhs = parseUnary();
            int nextPrec = getBinaryPrecedence(cur.kind);
            if (tokPrec < nextPrec) {
                rhs = parseBinaryOpRHS(tokPrec + 1, move(rhs));
            }

            lhs = make_unique<BinaryExpr>(binOp, move(lhs), move(rhs));
        }
    }
};

void printProgram(const vector<StmtPtr> &prog) {
    cout << "[\n";
    for (auto &s: prog) {
        cout << "  " << s->str() << ",\n";
    }
    cout << "]\n";
}

int main() {
    string program = R"(
        int x = 10;
        int y = 20;
        int z = x * y + (x - y) / 2;

    )";

    Parser parser(program);
    try {
        auto prog = parser.parseProgram();
        printProgram(prog);
    } catch (ParseException &pe) {
        cerr << "Parse error: code=" << (int)pe.err << " token=" << pe.token.kind << " val=" << pe.token.val << " msg=" << pe.what() << endl;
    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
