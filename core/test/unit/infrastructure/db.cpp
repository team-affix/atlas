// Goal database: vector-backed rules indexed by rule_id, with functor-indexed lookup buckets.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::UnorderedElementsAre;
#include "infrastructure/db.hpp"
#include "functor_fixture.hpp"

struct DbTest : public ::testing::Test {
    test_functors functors;
    expr f_head{expr::functor{functors.id("f"), {}}};
    expr g_head{expr::functor{functors.id("g"), {}}};
    expr var_head{expr::var{0}};
    rule f_rule{&f_head, {}, 1};
    rule g_rule{&g_head, {}};
    rule var_rule{&var_head, {}};
};

TEST_F(DbTest, DefaultDbLookupAllRulesEmpty) {
    db database;
    EXPECT_EQ(database.lookup_all_rules().size(), 0u);
}

TEST_F(DbTest, PushVarHeadedRuleThrows) {
    db database;
    EXPECT_THROW(database.push(var_rule), std::invalid_argument);
}

TEST_F(DbTest, PushAssignsIdAndExposesRule) {
    db database;
    const rule_id id = database.push(f_rule);
    EXPECT_EQ(id, 0u);
    EXPECT_EQ(*database.get_rule(id), f_rule);
    EXPECT_EQ(database.lookup_all_rules().size(), 1u);
}

TEST_F(DbTest, LookupByFunctorReturnsMatchingRules) {
    db database;
    const rule_id f0 = database.push(f_rule);
    expr f_head2{expr::functor{functors.id("f"), {}}};
    const rule_id f1 = database.push(rule{&f_head2, {}});
    database.push(g_rule);

    std::vector<rule_id> ids;
    auto it = database.lookup_rule_by_outermost_functor(functors.id("f")).iterate();
    while (!it.done()) {
        it.resume();
        if (it.has_yield())
            ids.push_back(it.consume_yield());
    }
    EXPECT_THAT(ids, UnorderedElementsAre(f0, f1));
}

TEST_F(DbTest, LookupByFunctorExcludesOtherFunctors) {
    db database;
    database.push(f_rule);
    database.push(g_rule);

    EXPECT_EQ(database.lookup_rule_by_outermost_functor(functors.id("g")).size(), 1u);
    EXPECT_EQ(database.lookup_rule_by_outermost_functor(functors.id("f")).size(), 1u);
}

TEST_F(DbTest, LookupUnknownFunctorReturnsEmpty) {
    db database;
    database.push(f_rule);
    EXPECT_EQ(database.lookup_rule_by_outermost_functor(functors.id("h")).size(), 0u);
}

TEST_F(DbTest, LookupAllRulesReturnsEveryPushedRule) {
    db database;
    database.push(f_rule);
    database.push(g_rule);

    std::vector<rule_id> ids;
    auto it = database.lookup_all_rules().iterate();
    while (!it.done()) {
        it.resume();
        if (it.has_yield())
            ids.push_back(it.consume_yield());
    }
    EXPECT_THAT(ids, UnorderedElementsAre(0, 1));
}

TEST_F(DbTest, GetRuleIdOutOfRangeThrows) {
    db database;
    database.push(f_rule);
    EXPECT_THROW(database.get_rule(1), std::out_of_range);
}
