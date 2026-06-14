// database_visitor: full database parse pushes rules via i_push_db_rule.
// Invariants: clause count, per-clause var scoping (indices restart), fact vs rule bodies.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "parser_fixture.hpp"
#include "infrastructure/functor_names.hpp"
#include "parser/generated/CHCLexer.h"
#include "parser/generated/CHCParser.h"
#include "parser/hpp/database_visitor.hpp"
#include "infrastructure/db.hpp"
#include "value_objects/lineage.hpp"
#include "interfaces/i_get_rule.hpp"

using ::testing::IsEmpty;
using ::testing::SizeIs;

static const rule* rule_at(const db& database, rule_id id) {
    return static_cast<const i_get_rule&>(database).get(id);
}

struct DatabaseVisitorTest : ParserCoreFixture {};

TEST_F(DatabaseVisitorTest, VisitDatabase) {
    db database;
    database_visitor dv{*pool, *pool, *var_seq, database, functor_map, next_functor_id};

    std::string input = "base(X). step(X) :- base(X).";
    antlr4::ANTLRInputStream stream(input);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);

    dv.visitDatabase(parser.database());

    const expr* x0 = pool->make_var(0);
    const rule* r0 = rule_at(database, rule_id{0});
    EXPECT_EQ(r0->head, pool->make_functor(functor_map.at("base"), {x0}));
    EXPECT_THAT(r0->body, IsEmpty());

    const expr* x1 = pool->make_var(1);
    const rule* r1 = rule_at(database, rule_id{1});
    EXPECT_EQ(r1->head, pool->make_functor(functor_map.at("step"), {x1}));
    EXPECT_THAT(r1->body, SizeIs(1));
    EXPECT_EQ(r1->body[0], pool->make_functor(functor_map.at("base"), {x1}));

    EXPECT_THROW({ rule_at(database, rule_id{2}); }, std::out_of_range);
}
