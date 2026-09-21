// pud_node_added_body_goals: store and get by id.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeAddedBodyGoalsTest : public ::testing::Test {
    PudNodeAddedBodyGoalsTest()
        : body_{expr::functor{1, {}}}
        , axiom_{pud_rule_id::axiom{0}}
        , goals_() {}

    expr body_;
    pud_rule_id axiom_;
    pud_node_added_body_goals goals_;
};

TEST_F(PudNodeAddedBodyGoalsTest, StoreThenGet) {
    goals_.store(&axiom_, {&body_});
    ASSERT_EQ(goals_.get(&axiom_).size(), 1u);
    EXPECT_EQ(goals_.get(&axiom_)[0], &body_);
}

TEST_F(PudNodeAddedBodyGoalsTest, EmptyStore) {
    goals_.store(&axiom_, {});
    EXPECT_TRUE(goals_.get(&axiom_).empty());
}

TEST_F(PudNodeAddedBodyGoalsTest, GetUnknownThrows) {
    EXPECT_THROW(goals_.get(&axiom_), std::out_of_range);
}
