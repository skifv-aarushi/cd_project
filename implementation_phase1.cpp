#include <iostream>
#include <vector>
#include <string>
#include <cctype>

using namespace std;

// -------------------------------------------------------------
// 1. TOKEN DEFINITIONS & LEXER
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

            // Keywords and identifiers
            if (isalpha(ch) || ch == '_') {
                size_t start = idx;
                while (idx < src.size() && (isalnum(src[idx]) || src[idx] == '_')) idx++;
                string word = src.substr(start, idx - start);

                if (word == "if")        tokens.push_back({TokenType::IF, word, line});
                else if (word == "else") tokens.push_back({TokenType::ELSE, word, line});
                else                     tokens.push_back({TokenType::ID, word, line});
                continue;
            }

            // Numbers
            if (isdigit(ch)) {
                size_t start = idx;
                while (idx < src.size() && isdigit(src[idx])) idx++;
                tokens.push_back({TokenType::NUM, src.substr(start, idx - start), line});
                continue;
            }

            // Two-character operators
            if (idx + 1 < src.size()) {
                string op2 = src.substr(idx, 2);
                if (op2 == "<=" || op2 == ">=" || op2 == "==" || op2 == "!=") {
                    tokens.push_back({TokenType::RELOP, op2, line});
                    idx += 2; continue;
                }
                if (op2 == "&&") { tokens.push_back({TokenType::AND, op2, line}); idx += 2; continue; }
                if (op2 == "||") { tokens.push_back({TokenType::OR, op2, line}); idx += 2; continue; }
            }

            // Single-character symbols
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
// 2. RECURSIVE DESCENT PARSER
// -------------------------------------------------------------

class Parser {
    vector<Token> tokens;
    size_t pos = 0;

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
    Parser(vector<Token> t) : tokens(move(t)) {}

    void parseProgram() {
        while (peek().type != TokenType::END) {
            parseStatement();
        }
        cout << ">> Program parsed successfully! Syntax is valid.\n";
    }

    void parseStatement() {
        if (peek().type == TokenType::ID) {
            parseAssignment();
        } else if (peek().type == TokenType::IF) {
            parseIf();
        } else if (peek().type == TokenType::LBRACE) {
            parseBlock();
        } else {
            cerr << "Parser error: cannot start statement with '" << peek().text << "'\n";
            exit(1);
        }
    }

    void parseAssignment() {
        string name = expect(TokenType::ID).text;
        expect(TokenType::ASSIGN);
        parseExpr();
        expect(TokenType::SEMI);
        cout << "Parsed assignment to variable: " << name << "\n";
    }

    void parseIf() {
        expect(TokenType::IF);
        expect(TokenType::LPAREN);
        parseBoolExpr();
        expect(TokenType::RPAREN);

        cout << "Parsed if condition\n";
        parseStatement();

        if (peek().type == TokenType::ELSE) {
            expect(TokenType::ELSE);
            cout << "Parsed else branch\n";
            parseStatement();
        }
    }

    void parseBlock() {
        expect(TokenType::LBRACE);
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::END) {
            parseStatement();
        }
        expect(TokenType::RBRACE);
    }

    // Logical OR (lowest precedence among booleans)
    void parseBoolExpr() {
        parseBoolTerm();
        while (peek().type == TokenType::OR) {
            expect(TokenType::OR);
            parseBoolTerm();
        }
    }

    // Logical AND
    void parseBoolTerm() {
        parseBoolFactor();
        while (peek().type == TokenType::AND) {
            expect(TokenType::AND);
            parseBoolFactor();
        }
    }

    // Logical NOT or comparison (a < b)
    void parseBoolFactor() {
        if (peek().type == TokenType::NOT) {
            expect(TokenType::NOT);
            parseBoolFactor();
            return;
        }

        parseExpr();
        if (peek().type == TokenType::RELOP) {
            expect(TokenType::RELOP);
            parseExpr();
        }
    }

    // Addition / Subtraction
    void parseExpr() {
        parseTerm();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            expect(peek().type);
            parseTerm();
        }
    }

    // Multiplication / Division
    void parseTerm() {
        parseFactor();
        while (peek().type == TokenType::MUL || peek().type == TokenType::DIV) {
            expect(peek().type);
            parseFactor();
        }
    }

    // Base units: ID, NUM, or nested (expr)
    void parseFactor() {
        if (peek().type == TokenType::ID) {
            expect(TokenType::ID);
        } else if (peek().type == TokenType::NUM) {
            expect(TokenType::NUM);
        } else if (peek().type == TokenType::LPAREN) {
            expect(TokenType::LPAREN);
            parseBoolExpr();
            expect(TokenType::RPAREN);
        } else {
            cerr << "Parser error: invalid expression at '" << peek().text << "'\n";
            exit(1);
        }
    }
};

// -------------------------------------------------------------
// 3. MAIN (DEMO)
// -------------------------------------------------------------

int main() {
    string testCode = R"(
        if (a < b || c == 10) {
            x = a + b * 2;
        } else {
            x = 0;
        }
    )";

    cout << "Source Code to test:\n" << testCode << "\n";

    Lexer lexer(testCode);
    vector<Token> tokens = lexer.scanTokens();

    Parser parser(tokens);
    parser.parseProgram();

    return 0;
}
