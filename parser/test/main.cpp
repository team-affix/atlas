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

// Helper: parse a string and return the first sexp context in the first clause.
static CHCParser::SexpContext* first_sexp(antlr4::ANTLRInputStream& stream,
                                          antlr4::CommonTokenStream& tokens,
                                          CHCLexer& lexer,
                                          CHCParser& parser) {
    (void)stream; (void)tokens; (void)lexer;
    auto* tree = parser.program();
    assert(parser.getNumberOfSyntaxErrors() == 0);
    return tree->clause(0)->sexp();
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

void test_expr_visitor_atom() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "foo.";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    assert(std::holds_alternative<expr::atom>(result->content));
    assert(std::get<expr::atom>(result->content).value == "foo");
    assert(var_map.empty());
}

void test_expr_visitor_var() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "X.";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    assert(std::holds_alternative<expr::var>(result->content));
    assert(var_map.count("X") == 1);
    assert(std::get<expr::var>(result->content).index == var_map["X"]);

    // Visiting the same variable again must return the same index.
    std::string input2 = "X.";
    antlr4::ANTLRInputStream stream2(input2);
    CHCLexer lexer2(&stream2);
    antlr4::CommonTokenStream tokens2(&lexer2);
    CHCParser parser2(&tokens2);
    auto* sexp2 = first_sexp(stream2, tokens2, lexer2, parser2);

    const expr* result2 = std::any_cast<const expr*>(ev.visitSexp(sexp2));
    assert(result == result2);
}

void test_expr_visitor_cons() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    std::string input = "(a . b).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    assert(std::holds_alternative<expr::cons>(result->content));
    const expr::cons& c = std::get<expr::cons>(result->content);
    assert(std::get<expr::atom>(c.lhs->content).value == "a");
    assert(std::get<expr::atom>(c.rhs->content).value == "b");
}

void test_expr_visitor_app() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    // (f x y) right-folds to cons(f, cons(x, cons(y, nil)))
    std::string input = "(f x y).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    auto* sexp = first_sexp(stream, tokens, lexer, parser);

    const expr* result = std::any_cast<const expr*>(ev.visitSexp(sexp));
    // cons(f, cons(x, cons(y, nil)))
    auto& c0 = std::get<expr::cons>(result->content);
    assert(std::get<expr::atom>(c0.lhs->content).value == "f");
    auto& c1 = std::get<expr::cons>(c0.rhs->content);
    assert(std::get<expr::atom>(c1.lhs->content).value == "x");
    auto& c2 = std::get<expr::cons>(c1.rhs->content);
    assert(std::get<expr::atom>(c2.lhs->content).value == "y");
    assert(std::get<expr::atom>(c2.rhs->content).value == "nil");
}

void unit_test_main() {
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_visitor_traversal);
    TEST(test_expr_visitor_atom);
    TEST(test_expr_visitor_var);
    TEST(test_expr_visitor_cons);
    TEST(test_expr_visitor_app);
}

#ifdef DEBUG
int main() {
    unit_test_main();
    return 0;
}
#endif
