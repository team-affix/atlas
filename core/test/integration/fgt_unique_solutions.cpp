// Integration: fgt runtimes yield each resolution lemma as solved at most once.
// A tiny max_clauses store forgets avoidances, so uniqueness comes from the
// repeat-solution overlay (pin + remember). Keys match production: sorted
// interned resolution_lineage* (unordered_set iteration is not a key).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/dbuct_ridge_fgt_runtime.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_bt_fgt_runtime.hpp"
#include "infrastructure/ridge_fgt_runtime.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "functor_fixture.hpp"

namespace {

inline constexpr size_t kMaxResolutions = 32;
inline constexpr uint32_t kSeed = 41;
inline constexpr double kExplorationConstant = 1.414;
inline constexpr double kGrantK = 0.1;
// One stored avoidance: extra solutions must be unique via the overlay, not CDCL.
inline constexpr size_t kTinyClauseStore = 1;
inline constexpr size_t kMaxTicks = 100000;

using resolution_lemma_key = std::vector<const resolution_lineage*>;

resolution_lemma_key make_resolution_lemma_key(const lemma& l) {
    resolution_lemma_key key(l.get_resolutions().begin(), l.get_resolutions().end());
    std::sort(key.begin(), key.end());
    return key;
}

template<typename Runtime>
void expect_each_solved_lemma_once(Runtime& session, size_t expected_solved) {
    std::set<resolution_lemma_key> seen;
    size_t solved_count = 0;
    size_t tick_count = 0;
    while (tick_count < kMaxTicks && session.next()) {
        ++tick_count;
        if (!session.solved())
            continue;
        const bool inserted =
            seen.insert(make_resolution_lemma_key(session.derive_resolution_lemma())).second;
        EXPECT_TRUE(inserted) << "duplicate solved resolution lemma at solved tick "
                              << solved_count;
        ++solved_count;
    }
    EXPECT_EQ(solved_count, expected_solved);
    EXPECT_EQ(seen.size(), expected_solved);
}

}  // namespace

struct FgtUniqueSolutionsTest : public ::testing::Test {
    test_functors functors;
    db database;
    initial_goal_exprs initial_goals;
    expr_pool saved_expr_pool_;

    void push_four_two_goal_combinations() {
        initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {}));
        initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {}));
        database.push(rule{saved_expr_pool_.make_functor(functors.id("f"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("f"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("g"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("g"), {}), {}, 0});
    }

    void push_eight_three_goal_combinations() {
        initial_goals.push(saved_expr_pool_.make_functor(functors.id("f"), {}));
        initial_goals.push(saved_expr_pool_.make_functor(functors.id("g"), {}));
        initial_goals.push(saved_expr_pool_.make_functor(functors.id("h"), {}));
        database.push(rule{saved_expr_pool_.make_functor(functors.id("f"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("f"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("g"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("g"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("h"), {}), {}, 0});
        database.push(rule{saved_expr_pool_.make_functor(functors.id("h"), {}), {}, 0});
    }
};

TEST_F(FgtUniqueSolutionsTest, RidgeFgtFourCombinationsWithTinyClauseStore) {
    push_four_two_goal_combinations();
    ridge_fgt_runtime session{
        database, initial_goals, 0, kMaxResolutions, kSeed,
        kExplorationConstant, kTinyClauseStore};
    expect_each_solved_lemma_once(session, 4);
}

TEST_F(FgtUniqueSolutionsTest, RidgeBtFgtFourCombinationsWithTinyClauseStore) {
    push_four_two_goal_combinations();
    ridge_bt_fgt_runtime session{
        database, initial_goals, 0, kMaxResolutions, kSeed,
        kExplorationConstant, kTinyClauseStore};
    expect_each_solved_lemma_once(session, 4);
}

TEST_F(FgtUniqueSolutionsTest, DbuctRidgeFgtFourCombinationsWithTinyClauseStore) {
    push_four_two_goal_combinations();
    dbuct_ridge_fgt_runtime session{
        database, initial_goals, 0, kMaxResolutions, kSeed,
        kExplorationConstant, kGrantK, kTinyClauseStore};
    expect_each_solved_lemma_once(session, 4);
}

TEST_F(FgtUniqueSolutionsTest, RidgeFgtEightCombinationsWithTinyClauseStore) {
    push_eight_three_goal_combinations();
    ridge_fgt_runtime session{
        database, initial_goals, 0, kMaxResolutions, kSeed,
        kExplorationConstant, kTinyClauseStore};
    expect_each_solved_lemma_once(session, 8);
}

TEST_F(FgtUniqueSolutionsTest, RidgeBtFgtEightCombinationsWithTinyClauseStore) {
    push_eight_three_goal_combinations();
    ridge_bt_fgt_runtime session{
        database, initial_goals, 0, kMaxResolutions, kSeed,
        kExplorationConstant, kTinyClauseStore};
    expect_each_solved_lemma_once(session, 8);
}

TEST_F(FgtUniqueSolutionsTest, DbuctRidgeFgtEightCombinationsWithTinyClauseStore) {
    push_eight_three_goal_combinations();
    dbuct_ridge_fgt_runtime session{
        database, initial_goals, 0, kMaxResolutions, kSeed,
        kExplorationConstant, kGrantK, kTinyClauseStore};
    expect_each_solved_lemma_once(session, 8);
}
