#include "../../test_utils.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../hpp/clause_visitor.hpp"
#include "../hpp/database_visitor.hpp"

// Lightweight parse/lex helpers — return true iff the input matches.
static bool lexes_atom(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0
        && ctx->ATOM() != nullptr
        && ctx->VARIABLE() == nullptr
        && ctx->expr().empty();
}
static bool lexes_var(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0
        && ctx->VARIABLE() != nullptr
        && ctx->ATOM() == nullptr
        && ctx->expr().empty();
}
static bool parses_expr(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.expr();
    return parser.getNumberOfSyntaxErrors() == 0;
}
static bool parses_clause(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.clause();
    return parser.getNumberOfSyntaxErrors() == 0;
}
static bool parses_database(const std::string& s) {
    antlr4::ANTLRInputStream stream(s);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.database();
    return parser.getNumberOfSyntaxErrors() == 0;
}
void test_lex_atom() {
    // Lowercase identifiers
    assert(lexes_atom("foo"));
    assert(lexes_atom("bar_baz"));
    assert(lexes_atom("hello123"));
    assert(lexes_atom("a"));
    // Numeric literals
    assert(lexes_atom("0"));
    assert(lexes_atom("42"));
    assert(lexes_atom("100"));
    // Quoted strings
    assert(lexes_atom("'hello'"));
    assert(lexes_atom("'hello world'"));
    assert(lexes_atom("'it\\'s fine'"));
}

void test_lex_var() {
    // Uppercase-initial identifiers
    assert(lexes_var("X"));
    assert(lexes_var("Y"));
    assert(lexes_var("Foo"));
    assert(lexes_var("MyVar"));
    assert(lexes_var("ABC123"));
    // Underscore-initial (discard and named)
    assert(lexes_var("_"));
    assert(lexes_var("_X"));
    assert(lexes_var("_foo"));
}

void test_parse_cons() {
    // Normal spacing
    assert(parses_expr("(a . b)"));
    // No spaces around dot
    assert(parses_expr("(a.b)"));
    // Extra spaces
    assert(parses_expr("( a . b )"));
    // Newlines as whitespace
    assert(parses_expr("(a\n.\nb)"));
    // Nested cons
    assert(parses_expr("(a . (b . c))"));
    assert(parses_expr("((a . b) . c)"));
    // Cons of variables
    assert(parses_expr("(X . Y)"));
    // Mixed
    assert(parses_expr("(foo . X)"));

    // Confirm it really is a cons: exactly 2 sub-exprs with a dot between.
    {
        std::string input = "(a.b)";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        auto* ctx = parser.expr();
        assert(parser.getNumberOfSyntaxErrors() == 0);
        assert(ctx->expr().size() == 2);
        assert(ctx->children[2]->getText() == ".");
    }
}

void test_parse_list() {
    // Single-element list
    assert(parses_expr("(f x)"));
    // Multi-element list
    assert(parses_expr("(f x y)"));
    assert(parses_expr("(f x y z)"));
    // Newlines as whitespace
    assert(parses_expr("(f\nx\ny)"));
    // Nested lists
    assert(parses_expr("(f (g x) y)"));
    assert(parses_expr("(f (g (h x)))"));
    // Variables in list
    assert(parses_expr("(p X Y)"));
}

void test_parse_clause() {
    // Facts
    assert(parses_clause("foo."));
    assert(parses_clause("(p X)."));
    assert(parses_clause("42."));
    // Rules with one body atom
    assert(parses_clause("(p X) :- (q X)."));
    // Rules with multiple body atoms
    assert(parses_clause("(p X) :- (q X), (r X)."));
    assert(parses_clause("(p X Y) :- (q X), (r Y), (s X Y)."));
    // Whitespace variations
    assert(parses_clause("(p X)\n:-\n(q X)."));
}

void test_parse_database() {
    // Empty database
    assert(parses_database(""));
    // Single fact
    assert(parses_database("foo."));
    // Multiple clauses
    assert(parses_database("(base X). (step X) :- (base X)."));
    // Newlines between clauses
    assert(parses_database("(base X).\n(step X) :- (base X)."));
    // Verify clause count
    {
        std::string input = "(a). (b). (c X) :- (a), (b).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        auto* ctx = parser.database();
        assert(parser.getNumberOfSyntaxErrors() == 0);
        assert(ctx->clause().size() == 3);
    }
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
    auto* tree = parser.database();

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

void test_database_visitor_visitDatabase() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    database_visitor dv(pool, seq);

    // Two clauses: a fact and a rule. Variables are clause-scoped so each X
    // gets a fresh index: X in clause 1 → 0, X in clause 2 → 1.
    std::string input = "(base X). (step X) :- (base X).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto rules = std::any_cast<std::vector<rule>>(dv.visitDatabase(parser.database()));
    assert(rules.size() == 2);

    // Clause 1: "(base X)." — fact, X gets index 0.
    const expr* x0 = pool.var(0);
    assert(rules[0].head == pool.cons(pool.atom("base"), pool.cons(x0, pool.atom("nil"))));
    assert(rules[0].body.empty());

    // Clause 2: "(step X) :- (base X)." — rule, X gets index 1.
    const expr* x1 = pool.var(1);
    assert(rules[1].head == pool.cons(pool.atom("step"), pool.cons(x1, pool.atom("nil"))));
    assert(rules[1].body.size() == 1);
    assert(rules[1].body[0] == pool.cons(pool.atom("base"), pool.cons(x1, pool.atom("nil"))));
}

void unit_test_main() {
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_lex_atom);
    TEST(test_lex_var);
    TEST(test_parse_cons);
    TEST(test_parse_list);
    TEST(test_parse_clause);
    TEST(test_parse_database);
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
    TEST(test_database_visitor_visitDatabase);
}

int main() {
    unit_test_main();
    return 0;
}
