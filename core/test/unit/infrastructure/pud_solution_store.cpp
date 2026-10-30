#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_solution_store.hpp"

namespace {

struct PudSolutionStoreTest : public ::testing::Test {
    expr_pool exprs;
    pud_solution_store store;
    const expr* nat = exprs.make_functor(4, {});
};

}

TEST_F(PudSolutionStoreTest, ANewStoreHasNoSolution) {
    EXPECT_EQ(store.solution(), nullptr);
    EXPECT_EQ(store.solution_depth(), 0u);
    EXPECT_EQ(store.solution_count(), 0u);
}

TEST_F(PudSolutionStoreTest, SetStoresTheSolutionAndCountsIt) {
    store.set(nat, 3);
    EXPECT_EQ(store.solution(), nat);
    EXPECT_EQ(store.solution_depth(), 3u);
    EXPECT_EQ(store.solution_count(), 1u);
    store.set(nat, 1);
    EXPECT_EQ(store.solution_depth(), 1u);
    EXPECT_EQ(store.solution_count(), 2u);
}

TEST_F(PudSolutionStoreTest, ClearDropsTheCurrentSolutionAndKeepsTheCount) {
    store.set(nat, 3);
    store.clear();
    EXPECT_EQ(store.solution(), nullptr);
    EXPECT_EQ(store.solution_depth(), 0u);
    EXPECT_EQ(store.solution_count(), 1u);
}

TEST_F(PudSolutionStoreTest, SettingANullSolutionIsRejected) {
    EXPECT_THROW(store.set(nullptr, 0), std::logic_error);
    EXPECT_EQ(store.solution_count(), 0u);
}
