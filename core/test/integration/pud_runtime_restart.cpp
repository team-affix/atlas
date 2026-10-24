#include <cstdint>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_reload_harness.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"

using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAreArray;

// Restarting on the same modules: the old runtime is thrown away and a new one registers the very
// modules it left behind, then carries on unfolding where it left off. A twin built from the same
// program goes through the same unfolds without ever restarting, and every unfold of the restarted
// world must report exactly what the twin's does.
template<typename IRuntime>
struct PudRuntimeRestartIntegrationTest : public ::testing::Test {
    pud_reload_harness<IRuntime> h;
};

using restarted_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudRuntimeRestartIntegrationTest, restarted_runtimes_t);

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingBeforeAnyUnfoldPutsBackTheAxiomFrontier) {
    auto& h = this->h;
    auto& twin = h.build(h.colors());
    auto& restarted = h.build(h.colors());
    h.expect_faithful_restart(restarted, twin);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingAProgramThatRefutedOnRegistrationRefutesNothingAgain) {
    auto& h = this->h;
    auto& twin = h.build(h.refutations());
    auto& restarted = h.build(h.refutations());
    ASSERT_THAT(restarted.refuted_on_registration, Not(IsEmpty()));
    h.expect_faithful_restart(restarted, twin);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedArithmeticContinuesLikeItsTwin) {
    auto& h = this->h;
    h.rng.seed(40);
    auto& twin = h.build(h.arithmetic());
    auto& restarted = h.build(h.arithmetic());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 20, std::nullopt, goal_choice::random), 20u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedListsContinueLikeTheirTwin) {
    auto& h = this->h;
    h.rng.seed(41);
    auto& twin = h.build(h.lists());
    auto& restarted = h.build(h.lists());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 15, std::nullopt, goal_choice::random), 15u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedGraphQueriesRunOutOfUnfoldsLikeTheirTwin) {
    auto& h = this->h;
    h.rng.seed(42);
    auto& twin = h.build(h.graph());
    auto& restarted = h.build(h.graph());
    h.resume_in_lockstep(twin, restarted, 8, std::nullopt, goal_choice::random);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_LT(h.resume_in_lockstep(twin, restarted, 2000, 4002, goal_choice::leftmost), 2000u);
    EXPECT_FALSE(h.choose_step(restarted, 4002, goal_choice::leftmost).has_value());
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedTiersRunOutOfUnfoldsLikeTheirTwin) {
    auto& h = this->h;
    h.rng.seed(43);
    auto& twin = h.build(h.tiers());
    auto& restarted = h.build(h.tiers());
    h.resume_in_lockstep(twin, restarted, 2, std::nullopt, goal_choice::leftmost);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_LT(h.resume_in_lockstep(twin, restarted, 2000, std::nullopt, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedSiblingModulesContinueLikeTheirTwin) {
    auto& h = this->h;
    h.rng.seed(44);
    auto& twin = h.build(h.siblings());
    auto& restarted = h.build(h.siblings());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 20, std::nullopt, goal_choice::random), 20u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedRefutationsCascadeLikeTheirTwin) {
    auto& h = this->h;
    h.rng.seed(45);
    auto& twin = h.build(h.refutations());
    auto& restarted = h.build(h.refutations());
    h.resume_in_lockstep(twin, restarted, 2, std::nullopt, goal_choice::leftmost);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_LT(h.resume_in_lockstep(twin, restarted, 2000, std::nullopt, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, AfterARestartAnUnfoldInADependencyRefutesTheDependentsLeavesLikeTheTwin) {
    auto& h = this->h;
    auto& twin = h.build(h.cascade());
    auto& restarted = h.build(h.cascade());
    h.expect_faithful_restart(restarted, twin);
    const pud_global_node_id mid{.module = 8001, .node = 2};
    const pud_frontier_change expected = twin.unfold(mid, 0);
    const pud_frontier_change actual = restarted.unfold(mid, 0);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    EXPECT_TRUE(restarted.module_of(8002).check_refuted(0));
    EXPECT_FALSE(restarted.module_of(8002).check_refuted(1));
    h.expect_same_modules(twin, restarted);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ARestartedRuntimeStoresNewInferencesAfterTheOnesAlreadyThere) {
    auto& h = this->h;
    h.rng.seed(46);
    auto& twin = h.build(h.arithmetic());
    auto& restarted = h.build(h.arithmetic());
    h.resume_in_lockstep(twin, restarted, 15, std::nullopt, goal_choice::random);
    h.expect_faithful_restart(restarted, twin);
    const std::optional<unfold_step> step = h.choose_step(twin, std::nullopt, goal_choice::random);
    ASSERT_TRUE(step.has_value());
    const pud_module& module = restarted.module_of(step->leaf.module);
    const size_t stored_before = h.count_module_nodes(module);
    const pud_frontier_change expected = twin.unfold(step->leaf, step->goal);
    const pud_frontier_change actual = restarted.unfold(step->leaf, step->goal);
    EXPECT_EQ(actual.offspring, expected.offspring);
    ASSERT_THAT(module.get_children(step->leaf.node), Not(IsEmpty()));
    for (pud_node_id child : module.get_children(step->leaf.node))
        EXPECT_GE(child, stored_before);
    EXPECT_EQ(h.count_module_nodes(module), stored_before + module.get_children(step->leaf.node).size());
    h.expect_same_modules(twin, restarted);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingNeverStoresAnInferenceOrMarksARefutation) {
    auto& h = this->h;
    h.rng.seed(47);
    auto& twin = h.build(h.graph());
    auto& restarted = h.build(h.graph());
    h.resume_in_lockstep(twin, restarted, 25, std::nullopt, goal_choice::random);
    const size_t stored_before = h.count_nodes(restarted);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.count_nodes(restarted), stored_before);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingTwiceWithoutUnfoldingChangesNothing) {
    auto& h = this->h;
    h.rng.seed(48);
    auto& twin = h.build(h.lists());
    auto& restarted = h.build(h.lists());
    h.resume_in_lockstep(twin, restarted, 20, std::nullopt, goal_choice::random);
    h.expect_faithful_restart(restarted, twin);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 10, std::nullopt, goal_choice::random), 10u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingAfterEveryUnfoldMatchesTheTwin) {
    auto& h = this->h;
    h.rng.seed(49);
    auto& twin = h.build(h.arithmetic());
    auto& restarted = h.build(h.arithmetic());
    for (int unfold_index = 0; unfold_index < 20 && !this->HasFailure(); ++unfold_index) {
        ASSERT_TRUE(h.unfold_in_lockstep(twin, restarted, std::nullopt, goal_choice::random)) << "unfold " << unfold_index;
        h.expect_faithful_restart(restarted, twin);
    }
    h.expect_same_modules(twin, restarted);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartingEveryFewUnfoldsMatchesTheTwin) {
    auto& h = this->h;
    h.rng.seed(50);
    auto& twin = h.build(h.siblings());
    auto& restarted = h.build(h.siblings());
    for (int generation = 0; generation < 6 && !this->HasFailure(); ++generation) {
        EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 6, std::nullopt, goal_choice::random), 6u);
        h.expect_faithful_restart(restarted, twin);
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, ARestartAndACopyReloadAgreeAndContinueAlike) {
    auto& h = this->h;
    h.rng.seed(51);
    auto& twin = h.build(h.lists());
    auto& restarted = h.build(h.lists());
    h.resume_in_lockstep(twin, restarted, 12, std::nullopt, goal_choice::random);
    auto& copy = h.expect_faithful_reload(restarted);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(copy.live, restarted.live);
    for (int unfold_index = 0; unfold_index < 15 && !this->HasFailure(); ++unfold_index) {
        const std::optional<unfold_step> step = h.choose_step(twin, std::nullopt, goal_choice::random);
        ASSERT_TRUE(step.has_value());
        const pud_frontier_change expected = twin.unfold(step->leaf, step->goal);
        const pud_frontier_change by_restart = restarted.unfold(step->leaf, step->goal);
        const pud_frontier_change by_copy = copy.unfold(step->leaf, step->goal);
        EXPECT_EQ(by_restart.offspring, expected.offspring);
        EXPECT_EQ(by_copy.offspring, expected.offspring);
        EXPECT_THAT(by_restart.refuted, UnorderedElementsAreArray(expected.refuted));
        EXPECT_THAT(by_copy.refuted, UnorderedElementsAreArray(expected.refuted));
    }
    h.expect_same_modules(twin, restarted);
    h.expect_same_modules(twin, copy);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedPeanoMulMatchesTwinUnderHeavyLoad) {
    auto& h = this->h;
    h.rng.seed(80);
    auto& twin = h.build(h.peano_mul());
    auto& restarted = h.build(h.peano_mul());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 50, std::nullopt, goal_choice::random), 50u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 40, std::nullopt, goal_choice::random), 40u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedListMembersMatchTwinToExhaustion) {
    auto& h = this->h;
    h.rng.seed(81);
    auto& twin = h.build(h.list_members());
    auto& restarted = h.build(h.list_members());
    h.resume_in_lockstep(twin, restarted, 10, std::nullopt, goal_choice::random);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_LT(h.resume_in_lockstep(twin, restarted, 2000, 9102, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedMirrorTreesMatchTwin) {
    auto& h = this->h;
    h.rng.seed(82);
    auto& twin = h.build(h.mirror_trees());
    auto& restarted = h.build(h.mirror_trees());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 15, std::nullopt, goal_choice::random), 15u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_LT(h.resume_in_lockstep(twin, restarted, 2000, 9202, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedNestedUnaryChainMatchesTwin) {
    auto& h = this->h;
    h.rng.seed(83);
    auto& twin = h.build(h.nested_unary_chain());
    auto& restarted = h.build(h.nested_unary_chain());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 30, std::nullopt, goal_choice::random), 30u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 25, std::nullopt, goal_choice::random), 25u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedTripleAppendMatchesTwinUnderStress) {
    auto& h = this->h;
    h.rng.seed(84);
    auto& twin = h.build(h.triple_append());
    auto& restarted = h.build(h.triple_append());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 45, std::nullopt, goal_choice::random), 45u);
    h.expect_faithful_restart(restarted, twin);
    for (int generation = 0; generation < 4 && !this->HasFailure(); ++generation) {
        EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 12, std::nullopt, goal_choice::random), 12u);
        h.expect_faithful_restart(restarted, twin);
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, RestartedSharedBinaryMatchesTwin) {
    auto& h = this->h;
    h.rng.seed(85);
    auto& twin = h.build(h.shared_binary());
    auto& restarted = h.build(h.shared_binary());
    ASSERT_EQ(h.resume_in_lockstep(twin, restarted, 25, std::nullopt, goal_choice::random), 25u);
    h.expect_faithful_restart(restarted, twin);
    EXPECT_EQ(h.resume_in_lockstep(twin, restarted, 20, std::nullopt, goal_choice::random), 20u);
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, EveryStressProgramRestartsAndContinuesLikeItsTwin) {
    auto& h = this->h;
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        for (const program_t& program : h.stress_programs()) {
            h.rng.seed(500 + seed);
            auto& twin = h.build(program);
            auto& restarted = h.build(program);
            h.resume_in_lockstep(twin, restarted, 25 + h.rng() % 25, std::nullopt, goal_choice::random);
            h.expect_faithful_restart(restarted, twin);
            h.resume_in_lockstep(twin, restarted, 30, std::nullopt, goal_choice::random);
            if (this->HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}

TYPED_TEST(PudRuntimeRestartIntegrationTest, EveryProgramRestartsAndContinuesLikeItsTwin) {
    auto& h = this->h;
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        for (const program_t& program : h.every_program()) {
            h.rng.seed(200 + seed);
            auto& twin = h.build(program);
            auto& restarted = h.build(program);
            h.resume_in_lockstep(twin, restarted, h.rng() % 25, std::nullopt, goal_choice::random);
            h.expect_faithful_restart(restarted, twin);
            h.resume_in_lockstep(twin, restarted, 20, std::nullopt, goal_choice::random);
            if (this->HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}
