// pud_node_added_unifications: store and get by id.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_node_added_unifications.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeAddedUnificationsTest : public ::testing::Test {
    PudNodeAddedUnificationsTest()
        : pred_{expr::functor{1, {}}}
        , axiom_{pud_rule_id::axiom{0}}
        , unifs_() {}

    expr pred_;
    pud_rule_id axiom_;
    pud_node_added_unifications unifs_;
};

TEST_F(PudNodeAddedUnificationsTest, StoreThenGet) {
    unifs_.store(&axiom_, {{0, &pred_}});
    ASSERT_EQ(unifs_.get(&axiom_).size(), 1u);
    EXPECT_EQ(unifs_.get(&axiom_)[0].var_idx, 0u);
    EXPECT_EQ(unifs_.get(&axiom_)[0].value, &pred_);
}

TEST_F(PudNodeAddedUnificationsTest, GetUnknownThrows) {
    EXPECT_THROW(unifs_.get(&axiom_), std::out_of_range);
}
