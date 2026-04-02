#include "../../test_utils.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"

struct TestVisitor : public CHCBaseVisitor {
    int clause_count = 0;
    int sexp_count   = 0;

    antlrcpp::Any visitClause(CHCParser::ClauseContext* ctx) override {
        clause_count++;
        return visitChildren(ctx);
    }
    antlrcpp::Any visitSexp(CHCParser::SexpContext* ctx) override {
        sexp_count++;
        return visitChildren(ctx);
    }
};

// Helper: parse a string as a bare sexp (no trailing period required).
static CHCParser::SexpContext* first_sexp(antlr4::ANTLRInputStream& stream,
                                          antlr4::CommonTokenStream& tokens,
                                          CHCLexer& lexer,
                                          CHCParser& parser) {
    (void)stream; (void)tokens; (void)lexer;
    auto* sexp = parser.sexp();
    assert(parser.getNumberOfSyntaxErrors() == 0);
    return sexp;
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
    assert(v.sexp_count   == 1);
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    assert(result == pool.atom("foo"));
    assert(var_map.empty());
}

void test_expr_visitor_visitVar() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "X";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    assert(result == pool.var(var_map.at("X")));

    // Visiting the same variable again must return the same interned expr*.
    std::string input2 = "X";
    antlr4::ANTLRInputStream stream2(input2);
    CHCLexer lexer2(&stream2);
    antlr4::CommonTokenStream tokens2(&lexer2);
    CHCParser parser2(&tokens2);
    auto* sexp2 = first_sexp(stream2, tokens2, lexer2, parser2);

    assert(std::any_cast<const expr*>(ev.visitSexp(sexp2)) == result);
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* ga   = pool.cons(pool.atom("g"), pool.cons(pool.atom("a"), pool.atom("nil")));
    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
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
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    // Extract the two arms of the cons.
    const expr* lhs = std::get<expr::cons>(result->content).lhs;
    const expr* rhs = std::get<expr::cons>(result->content).rhs;
    assert(lhs != rhs);
    // Neither _ should appear in var_map.
    assert(var_map.find("_") == var_map.end());
}

void test_expr_visitor_visitVar_discard_inList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f _ _) — both discards are distinct, X is tracked normally
    std::string input = "(f _ _)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    // cons(f, cons(d1, cons(d2, nil)))
    auto& outer = std::get<expr::cons>(result->content);
    assert(outer.lhs == pool.atom("f"));
    auto& mid   = std::get<expr::cons>(outer.rhs->content);
    auto& inner = std::get<expr::cons>(mid.rhs->content);
    assert(mid.lhs != inner.lhs);   // the two _ are distinct
    assert(var_map.find("_") == var_map.end());
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
}

int main() {
    unit_test_main();
    return 0;
}
