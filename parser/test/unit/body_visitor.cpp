// body_visitor: comma-separated goal/body atoms share one var_map per visit.
// Invariants: atom count, left-to-right var index assignment, cross-atom sharing.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <map>
#include <string>
#include <vector>
#include "parser_fixture.hpp"
#include "infrastructure/functor_names.hpp"
#include "parser/generated/CHCLexer.h"
#include "parser/generated/CHCParser.h"
#include "parser/hpp/body_visitor.hpp"

using ::testing::SizeIs;

static CHCParser::BodyContext* parse_body(antlr4::ANTLRInputStream& stream,
                                          antlr4::CommonTokenStream& tokens,
                                          CHCLexer& lexer,
                                          CHCParser& parser) {
    (void)stream;
    (void)tokens;
    (void)lexer;
    auto* ctx = parser.body();
    EXPECT_EQ(parser.getNumberOfSyntaxErrors(), 0);
    return ctx;
}

struct BodyVisitorTest : ParserCoreFixture {};

TEST_F(BodyVisitorTest, SingleAtom) {
    std::map<std::string, uint32_t> var_map;
    body_visitor bv{*pool, *pool, *var_seq, var_map, functor_map, next_functor_id};

    std::string input = "foo";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(parse_body(stream, tokens, lexer, parser)));
    EXPECT_THAT(body, SizeIs(1));
    EXPECT_EQ(body[0], pool->make_functor(functor_map.at("foo"), {}));
}

TEST_F(BodyVisitorTest, MultipleAtomsShareVar) {
    std::map<std::string, uint32_t> var_map;
    body_visitor bv{*pool, *pool, *var_seq, var_map, functor_map, next_functor_id};

    std::string input = "p(X), q(X)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(parse_body(stream, tokens, lexer, parser)));
    EXPECT_THAT(body, SizeIs(2));

    const expr* x = pool->make_var(0);
    EXPECT_EQ(body[0], pool->make_functor(functor_map.at("p"), {x}));
    EXPECT_EQ(body[1], pool->make_functor(functor_map.at("q"), {x}));
}

TEST_F(BodyVisitorTest, VarSharingAcrossAtoms) {
    std::map<std::string, uint32_t> var_map;
    body_visitor bv{*pool, *pool, *var_seq, var_map, functor_map, next_functor_id};

    std::string input = "f(X, Y), g(Y, Z)";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(parse_body(stream, tokens, lexer, parser)));
    EXPECT_THAT(body, SizeIs(2));

    const expr* x = pool->make_var(0);
    const expr* y = pool->make_var(1);
    EXPECT_EQ(body[0], pool->make_functor(functor_map.at("f"), {x, y}));

    const expr* z = pool->make_var(2);
    EXPECT_EQ(body[1], pool->make_functor(functor_map.at("g"), {y, z}));
}
