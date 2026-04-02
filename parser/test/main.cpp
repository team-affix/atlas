#include "../../test_utils.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../generated/CHCBaseVisitor.h"

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

void unit_test_main() {
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_visitor_traversal);
}

#ifdef DEBUG
int main() {
    unit_test_main();
    return 0;
}
#endif
