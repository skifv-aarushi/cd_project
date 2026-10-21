#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <utility>
#include <cstdlib>

using namespace std;

// =============================================================
// 1. LEXICAL ANALYSIS (LEXER)
// =============================================================

enum class TokenType {
    IF, ELSE, WHILE, BREAK, CONTINUE, PRINT,
    ID, NUM, ASSIGN,
    PLUS, MINUS, MUL, DIV,
    RELOP, AND, OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END_OF_FILE
};

static string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::IF:          return "'if'";
        case TokenType::ELSE:        return "'else'";
        case TokenType::WHILE:       return "'while'";
        case TokenType::BREAK:       return "'break'";
        case TokenType::CONTINUE:    return "'continue'";
        case TokenType::PRINT:       return "'print'";
        case TokenType::ID:          return "identifier";
        case TokenType::NUM:         return "number";
        case TokenType::ASSIGN:      return "'='";
        case TokenType::PLUS:        return "'+'";
        case TokenType::MINUS:       return "'-'";
        case TokenType::MUL:         return "'*'";
        case TokenType::DIV:         return "'/'";
        case TokenType::RELOP:       return "relational operator";
        case TokenType::AND:         return "'&&'";
        case TokenType::OR:          return "'||'";
        case TokenType::NOT:         return "'!'";
        case TokenType::LPAREN:      return "'('";
        case TokenType::RPAREN:      return "')'";
        case TokenType::LBRACE:      return "'{'";
        case TokenType::RBRACE:      return "'}'";
        case TokenType::SEMI:        return "';'";
        case TokenType::END_OF_FILE: return "end of file";
    }
    return "unknown";
}

struct Token {
    TokenType type;
    string text;
    int line;
    int col;
};

static string describe(const Token& t) {
    if (t.type == TokenType::END_OF_FILE) return "end of file";
    return "'" + t.text + "'";
}

static bool isSpaceCh(char c) { return isspace(static_cast<unsigned char>(c)) != 0; }
static bool isAlphaCh(char c) { return isalpha(static_cast<unsigned char>(c)) != 0; }
static bool isDigitCh(char c) { return isdigit(static_cast<unsigned char>(c)) != 0; }
static bool isAlnumCh(char c) { return isalnum(static_cast<unsigned char>(c)) != 0; }

class Lexer {
    string src;
    size_t idx = 0;
    int line = 1;
    int col = 1;

    char peekChar() const {
        if (idx < src.size()) return src[idx];
        return '\0';
    }

    char peekNext() const {
        if (idx + 1 < src.size()) return src[idx + 1];
        return '\0';
    }

    char advance() {
        char ch = src[idx++];
        if (ch == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
        return ch;
    }

public:
    explicit Lexer(string code) : src(move(code)) {}

    vector<Token> scanTokens() {
        vector<Token> tokens;

        while (idx < src.size()) {
            char ch = peekChar();

            if (isSpaceCh(ch)) {
                advance();
                continue;
            }

            int startLine = line;
            int startCol = col;

            // Single line comments (//)
            if (ch == '/' && peekNext() == '/') {
                while (idx < src.size() && peekChar() != '\n') advance();
                continue;
            }

            // Identifiers and Keywords
            if (isAlphaCh(ch) || ch == '_') {
                string word;
                while (idx < src.size() && (isAlnumCh(peekChar()) || peekChar() == '_')) {
                    word += advance();
                }

                TokenType type = TokenType::ID;
                if (word == "if")            type = TokenType::IF;
                else if (word == "else")     type = TokenType::ELSE;
                else if (word == "while")    type = TokenType::WHILE;
                else if (word == "break")    type = TokenType::BREAK;
                else if (word == "continue") type = TokenType::CONTINUE;
                else if (word == "print")    type = TokenType::PRINT;
                tokens.push_back({type, word, startLine, startCol});
                continue;
            }

            // Numeric Literals
            if (isDigitCh(ch)) {
                string numStr;
                while (idx < src.size() && isDigitCh(peekChar())) {
                    numStr += advance();
                }
                // Reject things like "12abc" instead of silently splitting them
                if (isAlphaCh(peekChar()) || peekChar() == '_') {
                    throw runtime_error("Lexical Error at line " + to_string(startLine) + ", col " +
                                        to_string(startCol) + ": Malformed token '" + numStr + peekChar() +
                                        "...' (identifiers cannot start with a digit).");
                }
                tokens.push_back({TokenType::NUM, numStr, startLine, startCol});
                continue;
            }

            // Relational, Assignment and Not operators
            if (ch == '<' || ch == '>' || ch == '!' || ch == '=') {
                advance();
                if (peekChar() == '=') {
                    advance();
                    tokens.push_back({TokenType::RELOP, string(1, ch) + "=", startLine, startCol});
                } else if (ch == '=') {
                    tokens.push_back({TokenType::ASSIGN, "=", startLine, startCol});
                } else if (ch == '!') {
                    tokens.push_back({TokenType::NOT, "!", startLine, startCol});
                } else {
                    tokens.push_back({TokenType::RELOP, string(1, ch), startLine, startCol});
                }
                continue;
            }

            // Logical AND (&&)
            if (ch == '&') {
                advance();
                if (peekChar() == '&') {
                    advance();
                    tokens.push_back({TokenType::AND, "&&", startLine, startCol});
                } else {
                    throw runtime_error("Lexical Error at line " + to_string(startLine) + ", col " + to_string(startCol) +
                                        ": Incomplete operator '&'. Expected '&&'.");
                }
                continue;
            }

            // Logical OR (||)
            if (ch == '|') {
                advance();
                if (peekChar() == '|') {
                    advance();
                    tokens.push_back({TokenType::OR, "||", startLine, startCol});
                } else {
                    throw runtime_error("Lexical Error at line " + to_string(startLine) + ", col " + to_string(startCol) +
                                        ": Incomplete operator '|'. Expected '||'.");
                }
                continue;
            }

            // Single Character Operators and Delimiters
            TokenType single;
            bool matched = true;
            switch (ch) {
                case '+': single = TokenType::PLUS;   break;
                case '-': single = TokenType::MINUS;  break;
                case '*': single = TokenType::MUL;    break;
                case '/': single = TokenType::DIV;    break;
                case '(': single = TokenType::LPAREN; break;
                case ')': single = TokenType::RPAREN; break;
                case '{': single = TokenType::LBRACE; break;
                case '}': single = TokenType::RBRACE; break;
                case ';': single = TokenType::SEMI;   break;
                default:  single = TokenType::SEMI; matched = false; break;
            }
            if (matched) {
                advance();
                tokens.push_back({single, string(1, ch), startLine, startCol});
                continue;
            }

            throw runtime_error("Lexical Error: Unexpected character '" + string(1, ch) +
                                "' at line " + to_string(startLine) + ", col " + to_string(startCol));
        }

        tokens.push_back({TokenType::END_OF_FILE, "", line, col});
        return tokens;
    }
};

// =============================================================
// 2. TAC CODE GENERATION & BACKPATCHING ENGINE
// =============================================================

struct Quad {
    int index;
    string op;
    string arg1;
    string arg2;
    string target;
};

class CodeGenerator {
public:
    vector<Quad> code;
    int tempCounter = 1;

    // Names the programmer used (so generated temporaries never clash with them)
    unordered_set<string> userNames;
    // Names of the compiler-generated temporaries
    unordered_set<string> tempNames;

    int nextQuad() const {
        return static_cast<int>(code.size());
    }

    string newTemp() {
        string name;
        do {
            name = "t" + to_string(tempCounter++);
        } while (userNames.count(name));
        tempNames.insert(name);
        return name;
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
        for (int i : list) {
            if (i >= 0 && i < static_cast<int>(code.size())) {
                code[i].target = to_string(targetQuad);
            }
        }
    }

    // Every jump must point to a real instruction once parsing is done.
    void validateJumps() const {
        for (const auto& q : code) {
            if (q.op != "goto" && q.op != "if") continue;
            int t = -1;
            try {
                size_t used = 0;
                t = stoi(q.target, &used);
                if (used != q.target.size()) t = -1;
            } catch (...) {
                t = -1;
            }
            if (t < 0 || t >= static_cast<int>(code.size())) {
                throw runtime_error("Internal Error: unresolved jump target at quad " + to_string(q.index));
            }
        }
    }

    void printTAC() const {
        cout << "\n================ THREE-ADDRESS CODE (TAC) ================\n";
        for (const auto& q : code) {
            cout << q.index << ":\t";
            if (q.op == "=") {
                cout << q.target << " = " << q.arg1;
            } else if (q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/") {
                cout << q.target << " = " << q.arg1 << " " << q.op << " " << q.arg2;
            } else if (q.op == "if") {
                cout << "if " << q.arg1 << " " << q.arg2 << " goto " << q.target;
            } else if (q.op == "goto") {
                cout << "goto " << q.target;
            } else if (q.op == "print") {
                cout << "print " << q.arg1;
            } else if (q.op == "HALT") {
                cout << "HALT";
            }
            cout << "\n";
        }
        cout << "==========================================================\n";
    }
};

// =============================================================
// 3. SYNTAX-DIRECTED TRANSLATOR (PARSER)
// =============================================================

struct ExprAttr {
    string place;
    bool isConst = false;
    long long constVal = 0;

    ExprAttr() = default;
    ExprAttr(string p, bool c, long long v) : place(move(p)), isConst(c), constVal(v) {}
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

    vector<vector<int>> breakStack;   // pending 'break' jumps for each open loop
    vector<int> continueStack;        // loop-start quad for each open loop

    const Token& peek() const {
        if (pos < tokens.size()) return tokens[pos];
        return tokens.back();
    }

    [[noreturn]] void syntaxError(const Token& t, const string& msg) const {
        throw runtime_error("Syntax Error at line " + to_string(t.line) + ", col " + to_string(t.col) + ": " + msg);
    }

    Token expect(TokenType type) {
        Token t = peek();
        if (t.type != type) {
            syntaxError(t, "Expected " + tokenTypeName(type) + " but found " + describe(t));
        }
        pos++;
        return t;
    }

    // Decides whether the '(' at the current position opens a boolean group
    // such as (a < b && c) rather than an arithmetic group such as (a + b).
    bool isNextGroupBoolean() const {
        if (peek().type != TokenType::LPAREN) return false;
        int depth = 0;
        for (size_t i = pos; i < tokens.size(); ++i) {
            TokenType tt = tokens[i].type;
            if (tt == TokenType::LPAREN) {
                depth++;
            } else if (tt == TokenType::RPAREN) {
                depth--;
                if (depth == 0) return false;
            } else if (depth >= 1 && (tt == TokenType::AND || tt == TokenType::OR ||
                                      tt == TokenType::RELOP || tt == TokenType::NOT)) {
                return true;
            }
        }
        return false;
    }

public:
    SDTParser(vector<Token> t, CodeGenerator& g) : tokens(move(t)), cg(g) {}

    void parseProgram() {
        StmtAttr prog = parseStmtList();
        expect(TokenType::END_OF_FILE);

        int haltQuad = cg.nextQuad();
        cg.backpatch(prog.nextlist, haltQuad);
        cg.emit("HALT", "", "", "");
        cg.validateJumps();
    }

    StmtAttr parseStmtList() {
        StmtAttr sList;
        while (peek().type == TokenType::ID || peek().type == TokenType::IF ||
               peek().type == TokenType::WHILE || peek().type == TokenType::LBRACE ||
               peek().type == TokenType::BREAK || peek().type == TokenType::CONTINUE ||
               peek().type == TokenType::PRINT || peek().type == TokenType::SEMI) {

            if (peek().type == TokenType::SEMI) {
                expect(TokenType::SEMI);
                continue;
            }

            int marker = cg.nextQuad();
            StmtAttr s = parseStatement();
            cg.backpatch(sList.nextlist, marker);
            sList.nextlist = s.nextlist;
        }
        return sList;
    }

    StmtAttr parseStatement() {
        switch (peek().type) {
            case TokenType::ID:       return parseAssignment();
            case TokenType::IF:       return parseIf();
            case TokenType::WHILE:    return parseWhile();
            case TokenType::BREAK:    return parseBreak();
            case TokenType::CONTINUE: return parseContinue();
            case TokenType::PRINT:    return parsePrint();
            case TokenType::LBRACE:   return parseBlock();
            case TokenType::SEMI:     
                expect(TokenType::SEMI);
                return StmtAttr{};
            default:
                syntaxError(peek(), "Invalid statement beginning with " + describe(peek()));
        }
    }

    StmtAttr parseAssignment() {
        string name = expect(TokenType::ID).text;
        expect(TokenType::ASSIGN);
        ExprAttr e = parseExpr();
        expect(TokenType::SEMI);
        cg.emit("=", e.place, "", name);
        return StmtAttr{};
    }

    StmtAttr parsePrint() {
        expect(TokenType::PRINT);
        ExprAttr e = parseExpr();
        expect(TokenType::SEMI);
        cg.emit("print", e.place, "", "");
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
            cg.emit("goto", "", "", "-1");
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

    StmtAttr parseWhile() {
        expect(TokenType::WHILE);
        int mLoopStart = cg.nextQuad();

        expect(TokenType::LPAREN);
        BoolAttr b = parseBoolExpr();
        expect(TokenType::RPAREN);

        int mBody = cg.nextQuad();
        cg.backpatch(b.truelist, mBody);          

        breakStack.push_back({});
        continueStack.push_back(mLoopStart);

        StmtAttr sBody = parseStatement();

        cg.backpatch(sBody.nextlist, mLoopStart);
        cg.emit("goto", "", "", to_string(mLoopStart));

        int mExit = cg.nextQuad();
        cg.backpatch(b.falselist, mExit);

        vector<int> breaks = breakStack.back();
        breakStack.pop_back();
        continueStack.pop_back();
        cg.backpatch(breaks, mExit);

        return StmtAttr{};
    }

    StmtAttr parseBreak() {
        Token t = expect(TokenType::BREAK);
        expect(TokenType::SEMI);

        if (breakStack.empty()) {
            throw runtime_error("Semantic Error at line " + to_string(t.line) +
                                ": 'break' statement outside loop context.");
        }

        int q = cg.nextQuad();
        cg.emit("goto", "", "", "-1");
        breakStack.back().push_back(q);
        return StmtAttr{};
    }

    StmtAttr parseContinue() {
        Token t = expect(TokenType::CONTINUE);
        expect(TokenType::SEMI);

        if (continueStack.empty()) {
            throw runtime_error("Semantic Error at line " + to_string(t.line) +
                                ": 'continue' statement outside loop context.");
        }

        cg.emit("goto", "", "", to_string(continueStack.back()));
        return StmtAttr{};
    }

    StmtAttr parseBlock() {
        expect(TokenType::LBRACE);
        StmtAttr s = parseStmtList();
        expect(TokenType::RBRACE);
        return s;
    }

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

    BoolAttr parseBoolFactor() {
        if (peek().type == TokenType::NOT) {
            expect(TokenType::NOT);
            BoolAttr b = parseBoolFactor();
            return BoolAttr{b.falselist, b.truelist};
        }

        if (peek().type == TokenType::LPAREN && isNextGroupBoolean()) {
            expect(TokenType::LPAREN);
            BoolAttr b = parseBoolExpr();
            expect(TokenType::RPAREN);
            return b;
        }

        ExprAttr e1 = parseExpr();

        string relText = "!=";
        string rightPlace = "0";              
        if (peek().type == TokenType::RELOP) {
            relText = expect(TokenType::RELOP).text;
            ExprAttr e2 = parseExpr();
            rightPlace = e2.place;
        }

        BoolAttr b;
        b.truelist = cg.makelist(cg.nextQuad());
        b.falselist = cg.makelist(cg.nextQuad() + 1);

        cg.emit("if", e1.place, relText + " " + rightPlace, "-1");
        cg.emit("goto", "", "", "-1");
        return b;
    }

    ExprAttr parseExpr() {
        ExprAttr e1 = parseTerm();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            Token op = expect(peek().type);
            ExprAttr e2 = parseTerm();

            string temp = cg.newTemp();
            cg.emit(op.text, e1.place, e2.place, temp);
            e1 = ExprAttr(temp, false, 0);
        }
        return e1;
    }

    ExprAttr parseTerm() {
        ExprAttr f1 = parseFactor();
        while (peek().type == TokenType::MUL || peek().type == TokenType::DIV) {
            Token op = expect(peek().type);
            ExprAttr f2 = parseFactor();

            if (op.type == TokenType::DIV && f2.isConst && f2.constVal == 0) {
                throw runtime_error("Semantic Error at line " + to_string(op.line) +
                                    ": Division by constant zero is undefined.");
            }

            string temp = cg.newTemp();
            cg.emit(op.text, f1.place, f2.place, temp);
            f1 = ExprAttr(temp, false, 0);
        }
        return f1;
    }

    ExprAttr parseFactor() {
        const Token& t = peek();

        if (t.type == TokenType::ID) {
            return ExprAttr(expect(TokenType::ID).text, false, 0);
        }

        if (t.type == TokenType::NUM) {
            Token numTok = expect(TokenType::NUM);
            try {
                return ExprAttr(numTok.text, true, stoll(numTok.text));
            } catch (const out_of_range&) {
                throw runtime_error("Semantic Error at line " + to_string(numTok.line) +
                                    ": Integer literal '" + numTok.text + "' is too large.");
            }
        }

        if (t.type == TokenType::MINUS) {        
            expect(TokenType::MINUS);
            ExprAttr f = parseFactor();
            if (f.isConst) {
                long long v = -f.constVal;
                return ExprAttr(to_string(v), true, v);
            }
            string temp = cg.newTemp();
            cg.emit("-", "0", f.place, temp);
            return ExprAttr(temp, false, 0);
        }

        if (t.type == TokenType::LPAREN) {
            expect(TokenType::LPAREN);
            ExprAttr e = parseExpr();
            expect(TokenType::RPAREN);
            return e;
        }

        syntaxError(t, "Invalid expression factor " + describe(t));
    }
};

// =============================================================
// 4. TAC VIRTUAL MACHINE / INTERPRETER
// =============================================================

class TACInterpreter {
    const vector<Quad>& code;
    const unordered_set<string>& temps;
    map<string, long long> memory;            // ordered, so output is deterministic

    long long resolveValue(const string& arg) {
        if (arg.empty()) return 0;
        if (isDigitCh(arg[0]) || (arg.size() > 1 && arg[0] == '-' && isDigitCh(arg[1]))) {
            return stoll(arg);
        }
        auto it = memory.find(arg);
        if (it == memory.end()) {
            throw runtime_error("Runtime Error: Variable '" + arg + "' used before being assigned.");
        }
        return it->second;
    }

public:
    TACInterpreter(const vector<Quad>& c, const unordered_set<string>& t) : code(c), temps(t) {}

    void execute() {
        cout << "\n================ TAC RUNTIME EXECUTION ================\n";
        const long long MAX_STEPS = 5000000;   // guards against infinite loops
        int pc = 0;
        long long instructionsExecuted = 0;

        while (pc >= 0 && pc < static_cast<int>(code.size())) {
            if (++instructionsExecuted > MAX_STEPS) {
                throw runtime_error("Runtime Error: Execution limit exceeded (possible infinite loop).");
            }

            const Quad& q = code[pc];

            if (q.op == "HALT") {
                break;
            } else if (q.op == "=") {
                memory[q.target] = resolveValue(q.arg1);
                pc++;
            } else if (q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/") {
                long long v1 = resolveValue(q.arg1);
                long long v2 = resolveValue(q.arg2);
                long long res = 0;

                if (q.op == "+")      res = v1 + v2;
                else if (q.op == "-") res = v1 - v2;
                else if (q.op == "*") res = v1 * v2;
                else {
                    if (v2 == 0) throw runtime_error("Runtime Error: Dynamic division by zero.");
                    res = v1 / v2;
                }

                memory[q.target] = res;
                pc++;
            } else if (q.op == "goto") {
                pc = stoi(q.target);
            } else if (q.op == "if") {
                long long left = resolveValue(q.arg1);
                stringstream ss(q.arg2);
                string relop, rightStr;
                ss >> relop >> rightStr;
                long long right = resolveValue(rightStr);

                bool conditionMet = false;
                if (relop == "<")       conditionMet = (left < right);
                else if (relop == ">")  conditionMet = (left > right);
                else if (relop == "<=") conditionMet = (left <= right);
                else if (relop == ">=") conditionMet = (left >= right);
                else if (relop == "==") conditionMet = (left == right);
                else if (relop == "!=") conditionMet = (left != right);

                if (conditionMet) pc = stoi(q.target);
                else              pc++;
            } else if (q.op == "print") {
                cout << "[OUTPUT]: " << resolveValue(q.arg1) << "\n";
                pc++;
            } else {
                pc++;
            }
        }

        cout << "Execution completed (" << instructionsExecuted << " instructions executed).\n";
        cout << "\nFinal Variable States:\n";
        for (const auto& kv : memory) {
            if (!temps.count(kv.first)) {     
                cout << "  " << kv.first << " = " << kv.second << "\n";
            }
        }
        cout << "=======================================================\n";
    }
};

// =============================================================
// 5. MAIN DRIVER
// =============================================================

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

int main() {
    cout << "==========================================================\n";
    cout << "  COMPILER FOR IF-ELSE & LOOPS WITH TAC BACKPATCHING      \n";
    cout << "==========================================================\n";
    cout << "Enter your program code below.\n";
    cout << "Type 'END' on a new line (or press Ctrl+Shift+V to paste):\n\n";

    string programCode;
    string line;

    while (getline(cin, line)) {
        string t = trim(line);                 
        if (t == "END" || t == "end") {
            break;
        }
        programCode += line + "\n";
    }

    if (programCode.find_first_not_of(" \t\n\r") == string::npos) {
        cout << "Empty program input. Exiting.\n";
        return 0;
    }

    try {
        Lexer lexer(programCode);
        vector<Token> tokens = lexer.scanTokens();

        CodeGenerator cg;
        for (const auto& tk : tokens) {
            if (tk.type == TokenType::ID) cg.userNames.insert(tk.text);
        }

        SDTParser parser(tokens, cg);
        parser.parseProgram();

        cg.printTAC();

        TACInterpreter vm(cg.code, cg.tempNames);
        vm.execute();

    } catch (const exception& ex) {
        cerr << "\n[COMPILER / RUNTIME FAILURE]: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}