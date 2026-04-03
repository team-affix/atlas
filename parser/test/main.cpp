#include "../../test_utils.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../hpp/clause_visitor.hpp"

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

// Helper: parse a string as a bare expr (no trailing period required).
static CHCParser::ExprContext* first_expr(antlr4::ANTLRInputStream& stream,
                                          antlr4::CommonTokenStream& tokens,
                                          CHCLexer& lexer,
                                          CHCParser& parser) {
    (void)stream; (void)tokens; (void)lexer;
    auto* ctx = parser.expr();
    assert(parser.getNumberOfSyntaxErrors() == 0);
    return ctx;
}

void test_visitor_traversal() {
    std::string input = "foo.";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* tree = parser.program();

    assert(parser.getNumberOfSyntaxErrors() == 0);

    TestVisitor v;
    v.visit(tree);
    assert(v.clause_count == 1);
    assert(v.expr_count   == 1);
}

void test_expr_visitor_visitAtom() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "foo";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.atom("foo"));
    assert(var_map.empty());
}

void test_expr_visitor_visitVar() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    const expr* result;
    {
        std::string input = "X";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        result = std::any_cast<const expr*>(ev.visitExpr(first_expr(stream, tokens, lexer, parser)));
        assert(result == pool.var(var_map.at("X")));
    }

    // Visiting the same variable again must return the same interned expr*.
    {
        std::string input = "X";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        assert(std::any_cast<const expr*>(ev.visitExpr(first_expr(stream, tokens, lexer, parser))) == result);
    }
}

void test_expr_visitor_visitCons() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "(a . b)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.cons(pool.atom("a"), pool.atom("b")));
}

void test_expr_visitor_visitList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f x y) right-folds to cons(f, cons(x, cons(y, nil)))
    std::string input = "(f x y)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.cons(pool.atom("f"),
                    pool.cons(pool.atom("x"),
                    pool.cons(pool.atom("y"), pool.atom("nil")))));
}

void test_expr_visitor_visitCons_nested() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (a . (b . c)) → cons(a, cons(b, c))
    std::string input = "(a . (b . c))";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.cons(pool.atom("a"),
                    pool.cons(pool.atom("b"), pool.atom("c"))));
}

void test_expr_visitor_visitList_withVars() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f X Y) → cons(f, cons(var(X), cons(var(Y), nil))), X and Y distinct
    std::string input = "(f X Y)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(var_map.at("X") != var_map.at("Y"));
    assert(result == pool.cons(pool.atom("f"),
                    pool.cons(pool.var(var_map.at("X")),
                    pool.cons(pool.var(var_map.at("Y")), pool.atom("nil")))));
}

void test_expr_visitor_visitList_nestedList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f (g a) b) → cons(f, cons(cons(g, cons(a, nil)), cons(b, nil)))
    std::string input = "(f (g a) b)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* ga   = pool.cons(pool.atom("g"), pool.cons(pool.atom("a"), pool.atom("nil")));
    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.cons(pool.atom("f"),
                    pool.cons(ga,
                    pool.cons(pool.atom("b"), pool.atom("nil")))));
}

void test_expr_visitor_visitVar_sharing() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f X X) — X appears twice, both occurrences must be the same interned expr*
    std::string input = "(f X X)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    const expr* x = pool.var(var_map.at("X"));
    assert(result == pool.cons(pool.atom("f"),
                    pool.cons(x,
                    pool.cons(x, pool.atom("nil")))));
}

void test_expr_visitor_visitVar_discard() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // Each _ gets a fresh index — they must not be the same expr*.
    std::string input = "(_ . _)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    const expr* lhs = std::get<expr::cons>(result->content).lhs;
    const expr* rhs = std::get<expr::cons>(result->content).rhs;
    assert(lhs != rhs);
    assert(var_map.find("_") == var_map.end());
}

void test_expr_visitor_visitVar_discard_inList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f _ _) — both discards are distinct
    std::string input = "(f _ _)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    // cons(f, cons(d1, cons(d2, nil)))
    auto& outer = std::get<expr::cons>(result->content);
    assert(outer.lhs == pool.atom("f"));
    auto& mid   = std::get<expr::cons>(outer.rhs->content);
    auto& inner = std::get<expr::cons>(mid.rhs->content);
    assert(mid.lhs != inner.lhs);
    assert(var_map.find("_") == var_map.end());
}

// Helper: parse a string as a clause (trailing period required by grammar).
static CHCParser::ClauseContext* parse_clause(antlr4::ANTLRInputStream& stream,
                                              antlr4::CommonTokenStream& tokens,
                                              CHCLexer& lexer,
                                              CHCParser& parser) {
    (void)stream; (void)tokens; (void)lexer;
    auto* ctx = parser.clause();
    assert(parser.getNumberOfSyntaxErrors() == 0);
    return ctx;
}

void test_clause_visitor_visitClause_fact() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    clause_visitor cv(pool, seq);

    // "(p X Y)." is a fact — head present, body empty.
    std::string input = "(p X Y).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
    // Right-fold visits Y first (index 0), then X (index 1).
    const expr* y = pool.var(0);
    const expr* x = pool.var(1);
    assert(r.head == pool.cons(pool.atom("p"), pool.cons(x, pool.cons(y, pool.atom("nil")))));
    assert(r.body.empty());
}

void test_clause_visitor_visitClause_rule() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    clause_visitor cv(pool, seq);

    // "(p X) :- (q X), (r X)." — X is shared across head and both body atoms.
    std::string input = "(p X) :- (q X), (r X).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
    // Right-fold of (p X) visits X first → index 0.
    const expr* x = pool.var(0);
    assert(r.head == pool.cons(pool.atom("p"), pool.cons(x, pool.atom("nil"))));
    assert(r.body.size() == 2);
    assert(r.body[0] == pool.cons(pool.atom("q"), pool.cons(x, pool.atom("nil"))));
    assert(r.body[1] == pool.cons(pool.atom("r"), pool.cons(x, pool.atom("nil"))));
}

void test_clause_visitor_visitClause_varScope() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    clause_visitor cv(pool, seq);

    // seq is shared across clauses; var_map is reset each time.
    // Clause 1's X gets index 0, clause 2's X gets index 1.
    {
        std::string input = "(p X).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
        assert(r.head == pool.cons(pool.atom("p"), pool.cons(pool.var(0), pool.atom("nil"))));
    }
    {
        std::string input = "(p X).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
        assert(r.head == pool.cons(pool.atom("p"), pool.cons(pool.var(1), pool.atom("nil"))));
    }
    assert(pool.var(0) != pool.var(1));
}

void unit_test_main() {
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_visitor_traversal);
    TEST(test_expr_visitor_visitAtom);
    TEST(test_expr_visitor_visitVar);
    TEST(test_expr_visitor_visitCons);
    TEST(test_expr_visitor_visitList);
    TEST(test_expr_visitor_visitCons_nested);
    TEST(test_expr_visitor_visitList_withVars);
    TEST(test_expr_visitor_visitList_nestedList);
    TEST(test_expr_visitor_visitVar_sharing);
    TEST(test_expr_visitor_visitVar_discard);
    TEST(test_expr_visitor_visitVar_discard_inList);
    TEST(test_clause_visitor_visitClause_fact);
    TEST(test_clause_visitor_visitClause_rule);
    TEST(test_clause_visitor_visitClause_varScope);
}

int main() {
    unit_test_main();
    return 0;
}
