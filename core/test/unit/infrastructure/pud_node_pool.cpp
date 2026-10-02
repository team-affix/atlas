#include <gtest/gtest.h>
#include "infrastructure/pud_node_pool.hpp"
#include "value_objects/expr.hpp"

struct PudNodePoolTest : public ::testing::Test {
    pud_node_pool pool;
    expr var{expr::var{1}};
    expr other{expr::var{2}};
};

TEST_F(PudNodePoolTest, PointerStillNamesPayloadAfterLaterMakes) {
    const pud_node* first = pool.make(
        {pud_specialization{.var_idx = 3, .value = &var}},
        {&var},
        4);
    pool.make({}, {}, 1);
    EXPECT_EQ(first->added_specializations.size(), 1u);
    EXPECT_EQ(first->added_specializations[0].var_idx, 3u);
    EXPECT_EQ(first->added_specializations[0].value, &var);
    EXPECT_EQ(first->added_body_goals.size(), 1u);
    EXPECT_EQ(first->added_body_goals[0], &var);
    EXPECT_EQ(first->added_var_count, 4u);
}

TEST_F(PudNodePoolTest, EmptyVectorsReadBackEmpty) {
    const pud_node* node = pool.make({}, {}, 7);
    EXPECT_TRUE(node->added_specializations.empty());
    EXPECT_TRUE(node->added_body_goals.empty());
    EXPECT_EQ(node->added_var_count, 7u);
}

TEST_F(PudNodePoolTest, ZeroVarCountReadsBackZero) {
    const pud_node* node = pool.make(
        {pud_specialization{.var_idx = 0, .value = &other}},
        {&other},
        0);
    EXPECT_EQ(node->added_var_count, 0u);
}

TEST_F(PudNodePoolTest, MakeReturnsPointerWhoseFieldsMatchArguments) {
    const pud_node* node = pool.make(
        {pud_specialization{.var_idx = 1, .value = &var}},
        {&var, &other},
        9);
    EXPECT_EQ(node->added_specializations.size(), 1u);
    EXPECT_EQ(node->added_specializations[0].var_idx, 1u);
    EXPECT_EQ(node->added_specializations[0].value, &var);
    EXPECT_EQ(node->added_body_goals.size(), 2u);
    EXPECT_EQ(node->added_body_goals[0], &var);
    EXPECT_EQ(node->added_body_goals[1], &other);
    EXPECT_EQ(node->added_var_count, 9u);
}
