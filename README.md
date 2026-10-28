# cd_project
# Single-Pass If-Else Compiler with Syntax-Directed Translation & Backpatching

A complete, end-to-end single-pass compiler frontend and Three-Address Code (TAC) intermediate representation engine implemented in C++[cite: 1, 2, 4]. Developed for the Compiler Design Lab course (BCSE307P) at the School of Computer Science and Engineering (SCOPE), Vellore Institute of Technology.

---

## 1. Problem Identification & Motivation

Compiling control-flow branching constructs (`if-else`) and compound boolean expressions traditionally relies on a multi-pass compilation architecture[cite: 1]:
1. The compiler tokenizes the input and constructs an explicit, heap-allocated Abstract Syntax Tree (AST) in memory[cite: 1, 3].
2. Separate post-order tree-walking passes traverse the AST to resolve forward jump destinations and emit intermediate code[cite: 1, 3].

### Drawbacks of the Traditional Approach
- **Significant Heap Footprint:** Allocating explicit heap nodes for every token, operator, and statement creates substantial memory overhead[cite: 1, 3].
- **Multi-Pass Latency:** Resolving forward jump targets requires dedicated subsequent tree or symbol-table passes[cite: 1, 3].
- **Complex Short-Circuit Evaluation:** Ensuring that sub-expressions in logical expressions (`&&`, `||`) terminate early when an outcome is already known requires explicit subtree pruning or custom AST traversals.

---

## 2. Solution & Architectural Approach

This project replaces the AST-based workflow with an integrated, single-pass pipeline[cite: 1, 3]:
- **Syntax-Directed Translation (SDT):** Semantic actions are embedded directly into production rules during predictive recursive descent parsing, emitting TAC quadruples on the fly[cite: 2, 3].
- **Backpatching Mechanism:** Eliminates forward-reference issues by emitting branch instructions with placeholder addresses and asynchronously filling in real addresses once targets are encountered.
- **Control-Flow Boolean Representation:** Compound conditions are treated as control-flow steering paths using `truelist` and `falselist` arrays rather than evaluating numeric truth flags (`0` or `1`) into temporary registers.

---

## 3. Formal Context-Free Grammar (CFG)

The language syntax is formally specified to eliminate grammar ambiguities and resolve the dangling-else problem by greedily pairing an `else` token with the nearest unmatched `if` statement[cite: 1, 5]:

```text
Program       -> StmtList
StmtList      -> Stmt StmtList | ε
Stmt          -> AssignStmt | IfStmt | Block
AssignStmt    -> ID = Expr ;
IfStmt        -> if ( BoolExpr ) Stmt ElsePart
ElsePart      -> else Stmt | ε
Block         -> { StmtList }

Expr          -> Term ((+ | -) Term)*
Term          -> Factor ((* | /) Factor)*
Factor        -> ID | NUM | ( Expr )

BoolExpr      -> BoolTerm (|| BoolTerm)*
BoolTerm      -> BoolFactor (&& BoolFactor)*
BoolFactor    -> ! BoolFactor | ( BoolExpr ) | Expr RelOp Expr
RelOp         -> < | <= | > | >= | == | !=
```
