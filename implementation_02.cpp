#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <sstream>

using namespace std;

// -------------------------------------------------------------
// 1. TOKEN DEFINITIONS & LEXER (Preserved from Review 1)
// -------------------------------------------------------------

enum class TokenType {
    IF, ELSE, ID, NUM, ASSIGN,
    PLUS, MINUS, MUL, DIV,
    RELOP, AND, OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END
};

struct Token {
    TokenType type;
    string text;
    int line;
};

class Lexer {
    string src;
    size_t idx = 0;
    int line = 1;

public:
    Lexer(string code) : src(move(code)) {}

    vector<Token> scanTokens() {
        vector<Token> tokens;
        while (idx < src.size()) {
            char ch = src[idx];
            if (ch == '\n') { line++; idx++; continue; }
            if (isspace(ch)) { idx++; continue; }

            if (isalpha(ch) || ch == '_') {
                size_t start = idx;
                while (idx < src.size() && (isalnum(src[idx]) || src[idx] == '_')) idx++;
                string word = src.substr(start, idx - start);
                if (word == "if")        tokens.push_back({TokenType::IF, word, line});
                else if (word == "else") tokens.push_back({TokenType::ELSE, word, line});
                else                     tokens.push_back({TokenType::ID, word, line});
                continue;
            }

            if (isdigit(ch)) {
                size_t start = idx;
                while (idx < src.size() && isdigit(src[idx])) idx++;
                tokens.push_back({TokenType::NUM, src.substr(start, idx - start), line});
                continue;
            }

            if (idx + 1 < src.size()) {
                string op2 = src.substr(idx, 2);
                if (op2 == "<=" || op2 == ">=" || op2 == "==" || op2 == "!=") {
                    tokens.push_back({TokenType::RELOP, op2, line});
                    idx += 2; continue;
                }
                if (op2 == "&&") { tokens.push_back({TokenType::AND, op2, line}); idx += 2; continue; }
                if (op2 == "||") { tokens.push_back({TokenType::OR, op2, line}); idx += 2; continue; }
            }

            if (ch == '<' || ch == '>') { tokens.push_back({TokenType::RELOP, string(1, ch), line}); idx++; continue; }
            if (ch == '=') { tokens.push_back({TokenType::ASSIGN, "=", line}); idx++; continue; }
            if (ch == '+') { tokens.push_back({TokenType::PLUS, "+", line}); idx++; continue; }
            if (ch == '-') { tokens.push_back({TokenType::MINUS, "-", line}); idx++; continue; }
            if (ch == '*') { tokens.push_back({TokenType::MUL, "*", line}); idx++; continue; }
            if (ch == '/') { tokens.push_back({TokenType::DIV, "/", line}); idx++; continue; }
            if (ch == '!') { tokens.push_back({TokenType::NOT, "!", line}); idx++; continue; }
            if (ch == '(') { tokens.push_back({TokenType::LPAREN, "(", line}); idx++; continue; }
            if (ch == ')') { tokens.push_back({TokenType::RPAREN, ")", line}); idx++; continue; }
            if (ch == '{') { tokens.push_back({TokenType::LBRACE, "{", line}); idx++; continue; }
            if (ch == '}') { tokens.push_back({TokenType::RBRACE, "}", line}); idx++; continue; }
            if (ch == ';') { tokens.push_back({TokenType::SEMI, ";", line}); idx++; continue; }

            cerr << "Lexer error: unexpected character '" << ch << "' at line " << line << "\n";
            idx++;
        }
        tokens.push_back({TokenType::END, "", line});
        return tokens;
    }
};

// -------------------------------------------------------------
// 2. TAC CODE GENERATION & BACKPATCHING ENGINE
// -------------------------------------------------------------

struct Quad {
    int index;
    string op;
    string arg1;
    string arg2;
    string target; // Jump target or assignment destination
};

class CodeGenerator {
public:
    vector<Quad> code;
    int tempCounter = 1;

    int nextQuad() const {
        return static_cast<int>(code.size());
    }

    string newTemp() {
        return "t" + to_string(tempCounter++);
    }

    void emit(const string& op, const string& arg1, const string& arg2, const string& target) {
        code.push_back({nextQuad(), op, arg1, arg2, target});
    }

    vector<int> makelist(int quadIndex) {
        return {quadIndex};
    }

    vector<int> merge(const vector<int>& l1, const vector<int>& l2) {
        vector<int> res = l1;
        res.insert(res.end(), l2.begin(), l2.end());
        return res;
    }

    void backpatch(const vector<int>& list, int targetQuad) {
        for (int idx : list) {
            if (idx >= 0 && idx < static_cast<int>(code.size())) {
                code[idx].target = to_string(targetQuad);
            }
        }
    }

    void printTAC() const {
        cout << "\n================ THREE-ADDRESS CODE (TAC) ================\n";
        for (const auto& q : code) {
            cout << q.index << ": ";
            if (q.op == "=") {
                cout << q.target << " = " << q.arg1;
            } else if (q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/") {
                cout << q.target << " = " << q.arg1 << " " << q.op << " " << q.arg2;
            } else if (q.op == "if") {
                cout << "if " << q.arg1 << " " << q.arg2 << " goto " << q.target;
            } else if (q.op == "goto") {
                cout << "goto " << q.target;
            }
            cout << "\n";
        }
        cout << "==========================================================\n";
    }
};

// -------------------------------------------------------------
// 3. SYNTAX-DIRECTED RECURSIVE DESCENT TRANSLATOR
// -------------------------------------------------------------

struct ExprAttr {
    string place;
};

struct BoolAttr {
    vector<int> truelist;
    vector<int> falselist;
};

struct StmtAttr {
    vector<int> nextlist;
};

class SDTParser {
    vector<Token> tokens;
    size_t pos = 0;
    CodeGenerator& cg;

    Token peek() { return tokens[pos]; }

    Token expect(TokenType type) {
        Token t = peek();
        if (t.type != type) {
            cerr << "Parser error: unexpected token '" << t.text << "' at line " << t.line << "\n";
            exit(1);
        }
        pos++;
        return t;
    }

public:
    SDTParser(vector<Token> t, CodeGenerator& g) : tokens(move(t)), cg(g) {}

    void parseProgram() {
        StmtAttr prog = parseStmtList();
        cg.backpatch(prog.nextlist, cg.nextQuad());
        cout << ">> Translation completed. TAC emitted and backpatched.\n";
    }

    StmtAttr parseStmtList() {
        StmtAttr sList;
        while (peek().type == TokenType::ID || peek().type == TokenType::IF || peek().type == TokenType::LBRACE) {
            int marker = cg.nextQuad();
            StmtAttr s = parseStatement();
            cg.backpatch(sList.nextlist, marker);
            sList.nextlist = s.nextlist;
        }
        return sList;
    }

    StmtAttr parseStatement() {
        if (peek().type == TokenType::ID) {
            return parseAssignment();
        } else if (peek().type == TokenType::IF) {
            return parseIf();
        } else if (peek().type == TokenType::LBRACE) {
            return parseBlock();
        }
        cerr << "Parser error: invalid statement head '" << peek().text << "'\n";
        exit(1);
    }

    StmtAttr parseAssignment() {
        string name = expect(TokenType::ID).text;
        expect(TokenType::ASSIGN);
        ExprAttr e = parseExpr();
        expect(TokenType::SEMI);
        cg.emit("=", e.place, "", name);
        return StmtAttr{};
    }

    StmtAttr parseIf() {
        expect(TokenType::IF);
        expect(TokenType::LPAREN);
        BoolAttr b = parseBoolExpr();
        expect(TokenType::RPAREN);

        int m1 = cg.nextQuad();
        StmtAttr s1 = parseStatement();

        if (peek().type == TokenType::ELSE) {
            int n = cg.nextQuad();
            cg.emit("goto", "", "", "-1"); // Placeholder jump to skip else body
            expect(TokenType::ELSE);
            int m2 = cg.nextQuad();
            StmtAttr s2 = parseStatement();

            cg.backpatch(b.truelist, m1);
            cg.backpatch(b.falselist, m2);

            StmtAttr res;
            res.nextlist = cg.merge(s1.nextlist, cg.makelist(n));
            res.nextlist = cg.merge(res.nextlist, s2.nextlist);
            return res;
        } else {
            cg.backpatch(b.truelist, m1);
            StmtAttr res;
            res.nextlist = cg.merge(b.falselist, s1.nextlist);
            return res;
        }
    }

    StmtAttr parseBlock() {
        expect(TokenType::LBRACE);
        StmtAttr s = parseStmtList();
        expect(TokenType::RBRACE);
        return s;
    }

    // BoolExpr -> BoolTerm (|| M BoolTerm)*
    BoolAttr parseBoolExpr() {
        BoolAttr b1 = parseBoolTerm();
        while (peek().type == TokenType::OR) {
            expect(TokenType::OR);
            int m = cg.nextQuad();
            BoolAttr b2 = parseBoolTerm();

            cg.backpatch(b1.falselist, m);
            b1.truelist = cg.merge(b1.truelist, b2.truelist);
            b1.falselist = b2.falselist;
        }
        return b1;
    }

    // BoolTerm -> BoolFactor (&& M BoolFactor)*
    BoolAttr parseBoolTerm() {
        BoolAttr b1 = parseBoolFactor();
        while (peek().type == TokenType::AND) {
            expect(TokenType::AND);
            int m = cg.nextQuad();
            BoolAttr b2 = parseBoolFactor();

            cg.backpatch(b1.truelist, m);
            b1.falselist = cg.merge(b1.falselist, b2.falselist);
            b1.truelist = b2.truelist;
        }
        return b1;
    }

    // BoolFactor -> ! BoolFactor | ( BoolExpr ) | Expr RelOp Expr
    BoolAttr parseBoolFactor() {
        if (peek().type == TokenType::NOT) {
            expect(TokenType::NOT);
            BoolAttr b = parseBoolFactor();
            return {b.falselist, b.truelist};
        }
        if (peek().type == TokenType::LPAREN) {
            expect(TokenType::LPAREN);
            BoolAttr b = parseBoolExpr();
            expect(TokenType::RPAREN);
            return b;
        }

        ExprAttr e1 = parseExpr();
        Token rel = expect(TokenType::RELOP);
        ExprAttr e2 = parseExpr();

        BoolAttr b;
        b.truelist = cg.makelist(cg.nextQuad());
        b.falselist = cg.makelist(cg.nextQuad() + 1);

        cg.emit("if", e1.place, rel.text + " " + e2.place, "-1");
        cg.emit("goto", "", "", "-1");
        return b;
    }

    // Expr -> Term ((+ | -) Term)*
    ExprAttr parseExpr() {
        ExprAttr e1 = parseTerm();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            string op = expect(peek().type).text;
            ExprAttr e2 = parseTerm();
            string temp = cg.newTemp();
            cg.emit(op, e1.place, e2.place, temp);
            e1.place = temp;
        }
        return e1;
    }

    // Term -> Factor ((* | /) Factor)*
    ExprAttr parseTerm() {
        ExprAttr f1 = parseFactor();
        while (peek().type == TokenType::MUL || peek().type == TokenType::DIV) {
            string op = expect(peek().type).text;
            ExprAttr f2 = parseFactor();
            string temp = cg.newTemp();
            cg.emit(op, f1.place, f2.place, temp);
            f1.place = temp;
        }
        return f1;
    }

    // Factor -> ID | NUM | ( Expr )
    ExprAttr parseFactor() {
        if (peek().type == TokenType::ID) {
            return {expect(TokenType::ID).text};
        } else if (peek().type == TokenType::NUM) {
            return {expect(TokenType::NUM).text};
        } else if (peek().type == TokenType::LPAREN) {
            expect(TokenType::LPAREN);
            ExprAttr e = parseExpr();
            expect(TokenType::RPAREN);
            return e;
        }
        cerr << "Parser error: expected factor, got '" << peek().text << "'\n";
        exit(1);
    }
};

// -------------------------------------------------------------
// 4. MAIN DRIVER (REVIEW 2 DEMO)
// -------------------------------------------------------------

int main() {
    string sourceCode = R"(
        if (a < b || c == 10) {
            x = a + b * 2;
        } else {
            x = 0;
        }
    )";

    cout << "Compiling source:\n" << sourceCode << "\n";

    Lexer lexer(sourceCode);
    vector<Token> tokens = lexer.scanTokens();

    CodeGenerator cg;
    SDTParser parser(tokens, cg);
    parser.parseProgram();

    cg.printTAC();

    return 0;
}
