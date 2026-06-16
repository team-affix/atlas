// Goal database: vector-backed rules indexed by rule_id, with total_rule_set holding
// every id 0 .. rules.size() - 1. All goals share the same rule_id_set reference.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::UnorderedElementsAre;
#include "infrastructure/db.hpp"
#include "value_objects/lineage.hpp"

struct DbTest : public ::testing::Test {
    expr head{expr::var{0}};
    rule r{&head, {}, 1};
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(DbTest, DefaultDbReturnsEmptyRuleSet) {
    db database;
    EXPECT_EQ(database.get(&gl0).size(), 0u);
}

TEST_F(DbTest, PushAssignsIdAndExposesRule) {
    db database;
    const rule_id id = database.push(r);
    EXPECT_EQ(id, 0u);
    EXPECT_EQ(*database.get(id), r);
    EXPECT_EQ(database.get(&gl0).size(), 1u);
}

TEST_F(DbTest, TotalRuleSetContainsAllIndices) {
    db database;
    database.push(r);
    expr head1{expr::var{1}};
    rule r1{&head1, {}};
    database.push(r1);

    std::vector<rule_id> ids;
    auto it = database.get(&gl0).iterate();
    while (!it.done()) {
        it.resume();
        if (it.has_yield())
            ids.push_back(it.consume_yield());
    }
    EXPECT_THAT(ids, UnorderedElementsAre(0, 1));
}

TEST_F(DbTest, SameTotalRulesForDifferentGoals) {
    db database;
    database.push(r);
    EXPECT_EQ(&database.get(&gl0), &database.get(&gl1));
}

TEST_F(DbTest, GetRuleIdOutOfRangeThrows) {
    db database;
    database.push(r);
    EXPECT_THROW(database.get(1), std::out_of_range);
}

TEST_F(DbTest, MutationsThroughGetVisibleToAllGoals) {
    db database;
    database.push(r);
    database.get(&gl0).erase(0);
    EXPECT_EQ(database.get(&gl1).size(), 0u);
}
