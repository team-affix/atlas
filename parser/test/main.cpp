#include "../../test_utils.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../hpp/body_visitor.hpp"
#include "../hpp/clause_visitor.hpp"
#include "../hpp/database_visitor.hpp"
#include "../hpp/import_database_from_file.hpp"
#include "../hpp/import_goals_from_string.hpp"

// Lightweight parse/lex helpers — return true iff the input matches.
// In the new grammar, an atom-only expression appears as ExprContext->FunctorContext->ATOM().
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
// expected_body: 0 = fact (no body), n>0 = rule with n body atoms.
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

void test_parse_functor() {
    // Nullary functor (atom)
    assert(parses_expr("foo"));
    assert(parses_expr("0"));
    assert(parses_expr("42"));
    // Nullary functor with empty parens
    assert(parses_expr("foo()"));
    // Unary functor
    assert(parses_expr("f(x)"));
    assert(parses_expr("suc(zero)"));
    // Binary functor
    assert(parses_expr("f(x, y)"));
    assert(parses_expr("add(X, Y)"));
    // Multi-arg functor
    assert(parses_expr("f(x, y, z)"));
    assert(parses_expr("step(X, Y, Z)"));
    // Nested functors
    assert(parses_expr("f(g(x), y)"));
    assert(parses_expr("f(g(h(x)))"));
    // Variables in functor
    assert(parses_expr("p(X, Y)"));
}

void test_parse_list() {
    // Empty list
    assert(parses_expr("[]"));
    // Single-element list
    assert(parses_expr("[a]"));
    // Multi-element list
    assert(parses_expr("[a, b]"));
    assert(parses_expr("[a, b, c]"));
    // List with tail (pipe notation)
    assert(parses_expr("[X|T]"));
    assert(parses_expr("[a|b]"));
    // Multi-element list with tail
    assert(parses_expr("[a, b|T]"));
    assert(parses_expr("[a, b, c|T]"));
    // Newlines as whitespace
    assert(parses_expr("[a,\nb,\nc]"));
    // Nested lists
    assert(parses_expr("[[a, b], c]"));
    assert(parses_expr("[f(x), g(y)]"));
    // Variables in list
    assert(parses_expr("[X, Y, Z]"));
}

void test_parse_clause() {
    // Facts (0 body atoms)
    assert(parses_clause("foo.",      0));
    assert(parses_clause("p(X).",     0));
    assert(parses_clause("42.",       0));
    // Rules with one body atom
    assert(parses_clause("p(X) :- q(X).",           1));
    // Rules with multiple body atoms
    assert(parses_clause("p(X) :- q(X), r(X).",     2));
    assert(parses_clause("p(X, Y) :- q(X), r(Y), s(X, Y).", 3));
    // Whitespace variations
    assert(parses_clause("p(X)\n:-\nq(X).", 1));
}

void test_parse_database() {
    assert(parses_database("",                                         0));
    assert(parses_database("foo.",                                     1));
    assert(parses_database("base(X). step(X) :- base(X).",            2));
    assert(parses_database("base(X).\nstep(X) :- base(X).",           2));
    assert(parses_database("a. b. c(X) :- a, b.",                     3));
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
    assert(result == pool.functor("foo", {}));
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

void test_expr_visitor_visitFunctor_nullary() {
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
    assert(result == pool.functor("foo", {}));
}

void test_expr_visitor_visitFunctor_unary() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "suc(X)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.functor("suc", {pool.var(var_map.at("X"))}));
}

void test_expr_visitor_visitFunctor_binary() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // add(X, Y) → functor("add", {var(X), var(Y)})
    std::string input = "add(X, Y)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(var_map.count("X") && var_map.count("Y"));
    assert(var_map.at("X") != var_map.at("Y"));
    assert(result == pool.functor("add", {pool.var(var_map.at("X")), pool.var(var_map.at("Y"))}));
}

void test_expr_visitor_visitList_empty() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "[]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    assert(std::any_cast<const expr*>(ev.visitExpr(ctx)) == pool.functor("nil", {}));
}

void test_expr_visitor_visitList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // [f, x, y] right-folds to cons(f, cons(x, cons(y, nil)))
    std::string input = "[f, x, y]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.functor("cons", {pool.functor("f", {}), 
                    pool.functor("cons", {pool.functor("x", {}), 
                    pool.functor("cons", {pool.functor("y", {}), pool.functor("nil", {})})})}));
}

void test_expr_visitor_visitList_pipe() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // [a, b|c] → cons(a, cons(b, c))
    std::string input = "[a, b|c]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.functor("cons", {pool.functor("a", {}), 
                    pool.functor("cons", {pool.functor("b", {}), pool.functor("c", {})})}));
}

void test_expr_visitor_visitList_withVars() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // [f, X, Y] → cons(f, cons(var(X), cons(var(Y), nil))), X and Y distinct
    std::string input = "[f, X, Y]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(var_map.count("X") && var_map.count("Y"));
    assert(var_map.at("X") != var_map.at("Y"));
    assert(result == pool.functor("cons", {pool.functor("f", {}), 
                    pool.functor("cons", {pool.var(var_map.at("X")), 
                    pool.functor("cons", {pool.var(var_map.at("Y")), pool.functor("nil", {})})})}));
}

void test_expr_visitor_visitList_nestedList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // [f, [g, a], b] → cons(f, cons(cons(g, cons(a, nil)), cons(b, nil)))
    std::string input = "[f, [g, a], b]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* ga   = pool.functor("cons", {pool.functor("g", {}), pool.functor("cons", {pool.functor("a", {}), pool.functor("nil", {})})});
    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    assert(result == pool.functor("cons", {pool.functor("f", {}), 
                    pool.functor("cons", {ga, 
                    pool.functor("cons", {pool.functor("b", {}), pool.functor("nil", {})})})}));
}

void test_expr_visitor_visitVar_sharing() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // [f, X, X] — X appears twice, both occurrences must be the same interned expr*
    std::string input = "[f, X, X]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    const expr* x = pool.var(var_map.at("X"));
    assert(result == pool.functor("cons", {pool.functor("f", {}), 
                    pool.functor("cons", {x, 
                    pool.functor("cons", {x, pool.functor("nil", {})})})}));
}

void test_expr_visitor_visitVar_discard() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // Each _ gets a fresh index — they must not be the same expr*.
    std::string input = "[_|_]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    const expr* lhs = std::get<expr::functor>(result->content).args[0];
    const expr* rhs = std::get<expr::functor>(result->content).args[1];
    assert(lhs != rhs);
    assert(var_map.find("_") == var_map.end());
}

void test_expr_visitor_visitVar_discard_inList() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // [f, _, _] — both discards are distinct
    std::string input = "[f, _, _]";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* ctx = first_expr(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitExpr(ctx));
    // cons(f, cons(d1, cons(d2, nil)))
    auto& outer = std::get<expr::functor>(result->content);
    assert(outer.args[0] == pool.functor("f", {}));
    auto& mid   = std::get<expr::functor>(outer.args[1]->content);
    auto& inner = std::get<expr::functor>(mid.args[1]->content);
    assert(mid.args[0] != inner.args[0]);
    assert(var_map.find("_") == var_map.end());
}

// Helper: parse a body context from a string.
static CHCParser::BodyContext* parse_body(antlr4::ANTLRInputStream& stream,
                                          antlr4::CommonTokenStream& tokens,
                                          CHCLexer& lexer,
                                          CHCParser& parser) {
    (void)stream; (void)tokens; (void)lexer;
    auto* ctx = parser.body();
    assert(parser.getNumberOfSyntaxErrors() == 0);
    return ctx;
}

void test_body_visitor_single_atom() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    body_visitor bv(pool, seq, var_map);

    std::string input = "foo";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(parse_body(stream, tokens, lexer, parser)));
    assert(body.size() == 1);
    assert(body[0] == pool.functor("foo", {}));
}

void test_body_visitor_multiple() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    body_visitor bv(pool, seq, var_map);

    // p(X), q(X) — X gets index 0 (first seen).
    std::string input = "p(X), q(X)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(parse_body(stream, tokens, lexer, parser)));
    assert(body.size() == 2);

    // X is first seen in p(X) → idx 0.
    const expr* x = pool.var(0);
    assert(body[0] == pool.functor("p", {x}));
    assert(body[1] == pool.functor("q", {x}));
}

void test_body_visitor_varSharing() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    body_visitor bv(pool, seq, var_map);

    // f(X, Y), g(Y, Z) — X=0, Y=1, Z=2 (left-to-right order).
    std::string input = "f(X, Y), g(Y, Z)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(parse_body(stream, tokens, lexer, parser)));
    assert(body.size() == 2);

    // f(X, Y): X first → 0, Y next → 1.
    const expr* x = pool.var(0);
    const expr* y = pool.var(1);
    assert(body[0] == pool.functor("f", {x, y}));

    // g(Y, Z): Y already 1, Z → 2.
    const expr* z = pool.var(2);
    assert(body[1] == pool.functor("g", {y, z}));
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

    // "p(X, Y)." is a fact — head present, body empty.
    // Left-to-right: X → idx 0, Y → idx 1.
    std::string input = "p(X, Y).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
    const expr* x = pool.var(0);
    const expr* y = pool.var(1);
    assert(r.head == pool.functor("p", {x, y}));
    assert(r.body.empty());
}

void test_clause_visitor_visitClause_rule() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    clause_visitor cv(pool, seq);

    // "p(X) :- q(X), r(X)." — X is shared across head and both body atoms.
    std::string input = "p(X) :- q(X), r(X).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
    // X first seen in head p(X) → index 0.
    const expr* x = pool.var(0);
    assert(r.head == pool.functor("p", {x}));
    assert(r.body.size() == 2);
    assert(r.body[0] == pool.functor("q", {x}));
    assert(r.body[1] == pool.functor("r", {x}));
}

void test_clause_visitor_visitClause_varScope() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    clause_visitor cv(pool, seq);

    // seq is shared across clauses; var_map is reset each time.
    // Clause 1's X gets index 0, clause 2's X gets index 1.
    {
        std::string input = "p(X).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
        assert(r.head == pool.functor("p", {pool.var(0)}));
    }
    {
        std::string input = "p(X).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
        assert(r.head == pool.functor("p", {pool.var(1)}));
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
    std::string input = "base(X). step(X) :- base(X).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto rules = std::any_cast<std::vector<rule>>(dv.visitDatabase(parser.database()));
    assert(rules.size() == 2);

    // Clause 1: "base(X)." — fact, X gets index 0.
    const expr* x0 = pool.var(0);
    assert(rules[0].head == pool.functor("base", {x0}));
    assert(rules[0].body.empty());

    // Clause 2: "step(X) :- base(X)." — rule, X gets index 1.
    const expr* x1 = pool.var(1);
    assert(rules[1].head == pool.functor("step", {x1}));
    assert(rules[1].body.size() == 1);
    assert(rules[1].body[0] == pool.functor("base", {x1}));
}

void test_import_goals_from_string_single() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    auto [gl, var_name_to_idx] = import_goals_from_string("reach(0, 2)", pool, seq);
    assert(gl.size() == 1);
    assert(gl[0] == pool.functor("reach", {pool.functor("0", {}), pool.functor("2", {})}));
    assert(var_name_to_idx.empty());
}

void test_import_goals_from_string_multiple() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    auto [gl, var_name_to_idx] = import_goals_from_string("p(X), q(X)", pool, seq);
    assert(gl.size() == 2);

    // X first seen → idx 0.
    const expr* x = pool.var(0);
    assert(gl[0] == pool.functor("p", {x}));
    assert(gl[1] == pool.functor("q", {x}));
    assert(var_name_to_idx.at("X") == 0);
}

void test_import_goals_from_string_complex() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    // Three goals, three distinct variables X, Y, Z.
    // Left-to-right: reach(X, Y) → X=0, Y=1; next(Y, Z) → Z=2; base(Z).
    auto [gl, var_name_to_idx] = import_goals_from_string("reach(X, Y), next(Y, Z), base(Z)", pool, seq);

    assert(gl.size() == 3);
    assert(var_name_to_idx.size() == 3);
    assert(var_name_to_idx.at("X") == 0);
    assert(var_name_to_idx.at("Y") == 1);
    assert(var_name_to_idx.at("Z") == 2);

    const expr* x = pool.var(0);
    const expr* y = pool.var(1);
    const expr* z = pool.var(2);

    assert(gl[0] == pool.functor("reach", {x, y}));
    assert(gl[1] == pool.functor("next",  {y, z}));
    assert(gl[2] == pool.functor("base",  {z}));
}

void test_import_goals_from_string_bad() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    assert_throws(import_goals_from_string(":-", pool, seq), const std::runtime_error&);
}

void test_import_database_from_file_facts() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    database db = import_database_from_file("parser/fixtures/facts.chc", pool, seq);
    assert(db.size() == 3);

    // All three are facts (empty body).
    for (const auto& r : db)
        assert(r.body.empty());

    // Each head is base(N) — functor("base", {functor(N, {})}).
    assert(db[0].head == pool.functor("base", {pool.functor("0", {})}));
    assert(db[1].head == pool.functor("base", {pool.functor("1", {})}));
    assert(db[2].head == pool.functor("base", {pool.functor("2", {})}));
}

void test_import_database_from_file_rules() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    database db = import_database_from_file("parser/fixtures/rules.chc", pool, seq);
    assert(db.size() == 2);

    // Both clauses are rules (non-empty body).
    assert(db[0].body.size() == 2);
    assert(db[1].body.size() == 2);

    // Clause 0: step(X, Y) :- base(X), next(X, Y).
    // Left-to-right: X → 0, Y → 1.
    const expr* x0 = pool.var(0);
    const expr* y0 = pool.var(1);
    assert(db[0].head == pool.functor("step", {x0, y0}));
    assert(db[0].body[0] == pool.functor("base", {x0}));
    assert(db[0].body[1] == pool.functor("next", {x0, y0}));
}

void test_import_database_from_file_mixed() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    database db = import_database_from_file("parser/fixtures/mixed.chc", pool, seq);
    assert(db.size() == 5);

    // First three clauses are facts.
    assert(db[0].body.empty());
    assert(db[1].body.empty());
    assert(db[2].body.empty());

    // Last two clauses are rules.
    assert(db[3].body.size() == 2);
    assert(db[4].body.size() == 2);
}

void test_import_database_from_file_bad_path() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);

    assert_throws(import_database_from_file("parser/fixtures/nonexistent.chc", pool, seq), const std::runtime_error&);
}

void unit_test_main() {
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_lex_atom);
    TEST(test_lex_var);
    TEST(test_parse_functor);
    TEST(test_parse_list);
    TEST(test_parse_clause);
    TEST(test_parse_database);
    TEST(test_visitor_traversal);
    TEST(test_expr_visitor_visitAtom);
    TEST(test_expr_visitor_visitVar);
    TEST(test_expr_visitor_visitFunctor_nullary);
    TEST(test_expr_visitor_visitFunctor_unary);
    TEST(test_expr_visitor_visitFunctor_binary);
    TEST(test_expr_visitor_visitList_empty);
    TEST(test_expr_visitor_visitList);
    TEST(test_expr_visitor_visitList_pipe);
    TEST(test_expr_visitor_visitList_withVars);
    TEST(test_expr_visitor_visitList_nestedList);
    TEST(test_expr_visitor_visitVar_sharing);
    TEST(test_expr_visitor_visitVar_discard);
    TEST(test_expr_visitor_visitVar_discard_inList);
    TEST(test_body_visitor_single_atom);
    TEST(test_body_visitor_multiple);
    TEST(test_body_visitor_varSharing);
    TEST(test_clause_visitor_visitClause_fact);
    TEST(test_clause_visitor_visitClause_rule);
    TEST(test_clause_visitor_visitClause_varScope);
    TEST(test_database_visitor_visitDatabase);
    TEST(test_import_goals_from_string_single);
    TEST(test_import_goals_from_string_multiple);
    TEST(test_import_goals_from_string_complex);
    TEST(test_import_goals_from_string_bad);
    TEST(test_import_database_from_file_facts);
    TEST(test_import_database_from_file_rules);
    TEST(test_import_database_from_file_mixed);
    TEST(test_import_database_from_file_bad_path);
}

int main() {
    unit_test_main();
    return 0;
}
