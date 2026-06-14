// clause_visitor: one CHC clause → rule{head, body} with clause-scoped var_map reset.
// Invariants: facts have empty body, head/body var sharing, fresh indices per clause.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <map>
#include <string>
#include "parser_fixture.hpp"
#include "infrastructure/functor_names.hpp"
#include "parser/generated/CHCLexer.h"
#include "parser/generated/CHCParser.h"
#include "parser/hpp/clause_visitor.hpp"
#include "value_objects/rule.hpp"

using ::testing::IsEmpty;
using ::testing::SizeIs;

static CHCParser::ClauseContext* parse_clause(antlr4::ANTLRInputStream& stream,
                                              antlr4::CommonTokenStream& tokens,
                                              CHCLexer& lexer,
                                              CHCParser& parser) {
    (void)stream;
    (void)tokens;
    (void)lexer;
    auto* ctx = parser.clause();
    EXPECT_EQ(parser.getNumberOfSyntaxErrors(), 0);
    return ctx;
}

struct ClauseVisitorTest : ParserCoreFixture {};

TEST_F(ClauseVisitorTest, VisitClauseFact) {
    clause_visitor cv{*pool, *pool, *var_seq, functor_map, next_functor_id};

    std::string input = "p(X, Y).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
    const expr* x = pool->make_var(0);
    const expr* y = pool->make_var(1);
    EXPECT_EQ(r.head, pool->make_functor(functor_map.at("p"), {x, y}));
    EXPECT_THAT(r.body, IsEmpty());
}

TEST_F(ClauseVisitorTest, VisitClauseRule) {
    clause_visitor cv{*pool, *pool, *var_seq, functor_map, next_functor_id};

    std::string input = "p(X) :- q(X), r(X).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
    const expr* x = pool->make_var(0);
    EXPECT_EQ(r.head, pool->make_functor(functor_map.at("p"), {x}));
    EXPECT_THAT(r.body, SizeIs(2));
    EXPECT_EQ(r.body[0], pool->make_functor(functor_map.at("q"), {x}));
    EXPECT_EQ(r.body[1], pool->make_functor(functor_map.at("r"), {x}));
}

TEST_F(ClauseVisitorTest, VisitClauseVarScope) {
    clause_visitor cv{*pool, *pool, *var_seq, functor_map, next_functor_id};

    {
        std::string input = "p(X).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
        EXPECT_EQ(r.head, pool->make_functor(functor_map.at("p"), {pool->make_var(0)}));
    }
    {
        std::string input = "p(X).";
        antlr4::ANTLRInputStream stream(input);
        CHCLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        CHCParser parser(&tokens);
        rule r = std::any_cast<rule>(cv.visitClause(parse_clause(stream, tokens, lexer, parser)));
        EXPECT_EQ(r.head, pool->make_functor(functor_map.at("p"), {pool->make_var(1)}));
    }
    EXPECT_NE(pool->make_var(0), pool->make_var(1));
}
