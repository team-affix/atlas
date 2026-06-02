// CHC grammar lexer/parser: atom vs variable classification, clause/database structure,
// and ANTLR visitor traversal counts. No core expression construction.

#include <gtest/gtest.h>
#include <string>
#include "parser/generated/CHCLexer.h"
#include "parser/generated/CHCParser.h"
#include "parser/generated/CHCBaseVisitor.h"

static bool lexes_atom(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0
        && ctx->VARIABLE() == nullptr
        && ctx->functor() != nullptr
        && ctx->functor()->ATOM() != nullptr
        && ctx->functor()->expr().empty()
        && ctx->functor()->list() == nullptr;
}

static bool lexes_var(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0
        && ctx->VARIABLE() != nullptr
        && ctx->functor() == nullptr;
}

static bool parses_expr(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0;
}

static bool parses_clause(const std::string& s, size_t expected_body = 0) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = parser.clause();
    if (parser.getNumberOfSyntaxErrors() != 0) return false;
    auto* b = ctx->body();
    size_t actual = b ? b->expr().size() : 0;
    return actual == expected_body;
}

static bool parses_database(const std::string& s, size_t expected_clauses) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = parser.database();
    return parser.getNumberOfSyntaxErrors() == 0
        && ctx->clause().size() == expected_clauses;
}

struct TestVisitor : public CHCBaseVisitor {
    int clause_count = 0;
    int expr_count   = 0;

    antlrcpp::Any visitClause(CHCParser::ClauseContext* ctx) override {
        clause_count++;
        return visitChildren(ctx);
    }
    antlrcpp::Any visitExpr(CHCParser::ExprContext* ctx) override {
        expr_count++;
        return visitChildren(ctx);
    }
};

// ---------------------------------------------------------------------------
// Lex
// ---------------------------------------------------------------------------

TEST(ChcParseTest, LexAtom) {
    EXPECT_TRUE(lexes_atom("foo"));
    EXPECT_TRUE(lexes_atom("bar_baz"));
    EXPECT_TRUE(lexes_atom("hello123"));
    EXPECT_TRUE(lexes_atom("a"));
    EXPECT_TRUE(lexes_atom("0"));
    EXPECT_TRUE(lexes_atom("42"));
    EXPECT_TRUE(lexes_atom("100"));
    EXPECT_TRUE(lexes_atom("'hello'"));
    EXPECT_TRUE(lexes_atom("'hello world'"));
    EXPECT_TRUE(lexes_atom("'it\\'s fine'"));
}

TEST(ChcParseTest, LexVar) {
    EXPECT_TRUE(lexes_var("X"));
    EXPECT_TRUE(lexes_var("Y"));
    EXPECT_TRUE(lexes_var("Foo"));
    EXPECT_TRUE(lexes_var("MyVar"));
    EXPECT_TRUE(lexes_var("ABC123"));
    EXPECT_TRUE(lexes_var("_"));
    EXPECT_TRUE(lexes_var("_X"));
    EXPECT_TRUE(lexes_var("_foo"));
}

// ---------------------------------------------------------------------------
// Parse expressions, clauses, databases
// ---------------------------------------------------------------------------

TEST(ChcParseTest, ParseFunctor) {
    EXPECT_TRUE(parses_expr("foo"));
    EXPECT_TRUE(parses_expr("0"));
    EXPECT_TRUE(parses_expr("42"));
    EXPECT_TRUE(parses_expr("foo()"));
    EXPECT_TRUE(parses_expr("f(x)"));
    EXPECT_TRUE(parses_expr("suc(zero)"));
    EXPECT_TRUE(parses_expr("f(x, y)"));
    EXPECT_TRUE(parses_expr("add(X, Y)"));
    EXPECT_TRUE(parses_expr("f(x, y, z)"));
    EXPECT_TRUE(parses_expr("step(X, Y, Z)"));
    EXPECT_TRUE(parses_expr("f(g(x), y)"));
    EXPECT_TRUE(parses_expr("f(g(h(x)))"));
    EXPECT_TRUE(parses_expr("p(X, Y)"));
}

TEST(ChcParseTest, ParseList) {
    EXPECT_TRUE(parses_expr("[]"));
    EXPECT_TRUE(parses_expr("[a]"));
    EXPECT_TRUE(parses_expr("[a, b]"));
    EXPECT_TRUE(parses_expr("[a, b, c]"));
    EXPECT_TRUE(parses_expr("[X|T]"));
    EXPECT_TRUE(parses_expr("[a|b]"));
    EXPECT_TRUE(parses_expr("[a, b|T]"));
    EXPECT_TRUE(parses_expr("[a, b, c|T]"));
    EXPECT_TRUE(parses_expr("[a,\nb,\nc]"));
    EXPECT_TRUE(parses_expr("[[a, b], c]"));
    EXPECT_TRUE(parses_expr("[f(x), g(y)]"));
    EXPECT_TRUE(parses_expr("[X, Y, Z]"));
}

TEST(ChcParseTest, ParseClause) {
    EXPECT_TRUE(parses_clause("foo.", 0));
    EXPECT_TRUE(parses_clause("p(X).", 0));
    EXPECT_TRUE(parses_clause("42.", 0));
    EXPECT_TRUE(parses_clause("p(X) :- q(X).", 1));
    EXPECT_TRUE(parses_clause("p(X) :- q(X), r(X).", 2));
    EXPECT_TRUE(parses_clause("p(X, Y) :- q(X), r(Y), s(X, Y).", 3));
    EXPECT_TRUE(parses_clause("p(X)\n:-\nq(X).", 1));
}

TEST(ChcParseTest, ParseDatabase) {
    EXPECT_TRUE(parses_database("", 0));
    EXPECT_TRUE(parses_database("foo.", 1));
    EXPECT_TRUE(parses_database("base(X). step(X) :- base(X).", 2));
    EXPECT_TRUE(parses_database("base(X).\nstep(X) :- base(X).", 2));
    EXPECT_TRUE(parses_database("a. b. c(X) :- a, b.", 3));
}

// ---------------------------------------------------------------------------
// Visitor traversal
// ---------------------------------------------------------------------------

TEST(ChcParseTest, VisitorTraversal) {
    std::string input = "foo.";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* tree = parser.database();

    ASSERT_EQ(parser.getNumberOfSyntaxErrors(), 0);

    TestVisitor v;
    v.visit(tree);
    EXPECT_EQ(v.clause_count, 1);
    EXPECT_EQ(v.expr_count, 1);
}
