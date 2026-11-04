#include <gtest/gtest.h>
#include "infrastructure/pud_leaf_evaluation_store.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"

namespace {

struct PudLeafEvaluationStoreTest : public ::testing::Test {
    pud_leaf_evaluation_store store;
};

}

TEST_F(PudLeafEvaluationStoreTest, TheLastStoredEvaluationIsRead) {
    store.store(pud_leaf_evaluation{.leaf = pud_test_node(1), .refuted = true, .goals = {}});
    const pud_leaf_evaluation latest{
        .leaf = pud_test_node(2),
        .refuted = false,
        .goals = {{3, {pud_eager_candidate{.path = {pud_test_node(4)}, .witness_leaves = {pud_test_node(4)}}}}}};
    store.store(latest);
    EXPECT_EQ(store.get(), latest);
}
