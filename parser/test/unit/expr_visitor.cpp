// expr_visitor: parses CHC expressions into canonical expr* via i_make_functor / i_make_var /
// i_var_sequencer. Invariants: interning identity, list right-fold to cons/nil, var sharing,
// distinct discard indices, named vars recorded in var_map.

#include <gtest/gtest.h>
#include <map>
#include <string>
#include "parser_fixture.hpp"
#include "parser/generated/CHCLexer.h"
#include "parser/generated/CHCParser.h"
#include "parser/hpp/expr_visitor.hpp"

static CHCParser::ExprContext* first_expr(antlr4::ANTLRInputStream& stream,
                                          antlr4::CommonTokenStream& tokens,
                                          CHCLexer& lexer,
                                          CHCParser& parser) {
    (void)stream;
    (void)tokens;
    (void)lexer;
    auto* ctx = parser.expr();
    EXPECT_EQ(parser.getNumberOfSyntaxErrors(), 0);
    return ctx;
}

struct ExprVisitorTest : ParserCoreFixture {};

// ---------------------------------------------------------------------------
// Atoms and variables
// ---------------------------------------------------------------------------

TEST_F(ExprVisitorTest, VisitAtom) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "foo";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_EQ(result, pool->make("foo", {}));
    EXPECT_TRUE(var_map.empty());
}

TEST_F(ExprVisitorTest, VisitVar) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    const expr* result = nullptr;
    {
        std::string input = "X";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        result = std::any_cast<const expr*>(ev.visitExpr(first_expr(stream, tokens, lexer, parser)));
        EXPECT_EQ(result, pool->make(var_map.at("X")));
    }
    {
        std::string input = "X";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        EXPECT_EQ(std::any_cast<const expr*>(ev.visitExpr(first_expr(stream, tokens, lexer, parser))), result);
    }
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(ExprVisitorTest, VisitFunctorNullary) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "foo";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_EQ(result, pool->make("foo", {}));
}

TEST_F(ExprVisitorTest, VisitFunctorUnary) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "suc(X)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_EQ(result, pool->make("suc", {pool->make(var_map.at("X"))}));
}

TEST_F(ExprVisitorTest, VisitFunctorBinary) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "add(X, Y)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_TRUE(var_map.count("X") && var_map.count("Y"));
    EXPECT_NE(var_map.at("X"), var_map.at("Y"));
    EXPECT_EQ(result, pool->make("add", {pool->make(var_map.at("X")), pool->make(var_map.at("Y"))}));
}

// ---------------------------------------------------------------------------
// Lists
// ---------------------------------------------------------------------------

TEST_F(ExprVisitorTest, VisitListEmpty) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    EXPECT_EQ(std::any_cast<const expr*>(ev.visitExpr(ctx)), pool->make("nil", {}));
}

TEST_F(ExprVisitorTest, VisitList) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[f, x, y]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_EQ(result, pool->make("cons", {pool->make("f", {}),
                    pool->make("cons", {pool->make("x", {}),
                    pool->make("cons", {pool->make("y", {}), pool->make("nil", {})})})}));
}

TEST_F(ExprVisitorTest, VisitListPipe) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[a, b|c]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_EQ(result, pool->make("cons", {pool->make("a", {}),
                    pool->make("cons", {pool->make("b", {}), pool->make("c", {})})}));
}

TEST_F(ExprVisitorTest, VisitListWithVars) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[f, X, Y]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_TRUE(var_map.count("X") && var_map.count("Y"));
    EXPECT_NE(var_map.at("X"), var_map.at("Y"));
    EXPECT_EQ(result, pool->make("cons", {pool->make("f", {}),
                    pool->make("cons", {pool->make(var_map.at("X")),
                    pool->make("cons", {pool->make(var_map.at("Y")), pool->make("nil", {})})})}));
}

TEST_F(ExprVisitorTest, VisitListNestedList) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[f, [g, a], b]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* ga = pool->make("cons", {pool->make("g", {}), pool->make("cons", {pool->make("a", {}), pool->make("nil", {})})});
    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    EXPECT_EQ(result, pool->make("cons", {pool->make("f", {}),
                    pool->make("cons", {ga,
                    pool->make("cons", {pool->make("b", {}), pool->make("nil", {})})})}));
}

TEST_F(ExprVisitorTest, VisitVarSharing) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[f, X, X]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    const expr* x = pool->make(var_map.at("X"));
    EXPECT_EQ(result, pool->make("cons", {pool->make("f", {}),
                    pool->make("cons", {x,
                    pool->make("cons", {x, pool->make("nil", {})})})}));
}

TEST_F(ExprVisitorTest, VisitVarDiscard) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[_|_]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    const expr* lhs = std::get<expr::functor>(result->content).args[0];
    const expr* rhs = std::get<expr::functor>(result->content).args[1];
    EXPECT_NE(lhs, rhs);
    EXPECT_EQ(var_map.find("_"), var_map.end());
}

TEST_F(ExprVisitorTest, VisitVarDiscardInList) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev{*pool, *pool, *var_seq, var_map};

    std::string input = "[f, _, _]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    auto& outer = std::get<expr::functor>(result->content);
    EXPECT_EQ(outer.args[0], pool->make("f", {}));
    auto& mid   = std::get<expr::functor>(outer.args[1]->content);
    auto& inner = std::get<expr::functor>(mid.args[1]->content);
    EXPECT_NE(mid.args[0], inner.args[0]);
    EXPECT_EQ(var_map.find("_"), var_map.end());
}
