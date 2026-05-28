// Goal database rules: every goal lineage receives the same total rule set reference.
// The store must not fork per-goal copies unless rules are added to total_rules.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/db.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

struct DbTest : public ::testing::Test {
    static constexpr rule_id kRule0 = 0;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(DbTest, DefaultDbReturnsEmptyRuleSet) {
    db database;
    EXPECT_EQ(database.get(&gl0).size(), 0u);
}

TEST_F(DbTest, SameTotalRulesForDifferentGoals) {
    rule_set total;
    total.insert(kRule0);
    db database{std::move(total)};
    EXPECT_EQ(&database.get(&gl0), &database.get(&gl1));
}

TEST_F(DbTest, MutationsThroughGetVisibleToAllGoals) {
    db database;
    database.get(&gl0).insert(kRule0);
    EXPECT_EQ(database.get(&gl1).size(), 1u);
}
