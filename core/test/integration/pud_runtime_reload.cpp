#include <cstdint>
#include <limits>
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

// Reloading onto copies: a fresh runtime registers copies of a world's modules, while the original
// world goes on, so the two can be compared at every step.
template<typename IRuntime>
struct PudRuntimeReloadIntegrationTest : public ::testing::Test {
    pud_reload_harness<IRuntime> h;
};

using reloaded_runtimes_t = ::testing::Types<pud_runtime<pud_module, expr_pool>>;
TYPED_TEST_SUITE(PudRuntimeReloadIntegrationTest, reloaded_runtimes_t);

// Reloading: the frontier, the refutations and the modules come back exactly as they were left.

TYPED_TEST(PudRuntimeReloadIntegrationTest, AModuleWithoutAxiomsReloadsToAnEmptyFrontier) {
    auto& h = this->h;
    auto& original = h.build({module_spec{.id = 9, .dependencies = {}, .axioms = {}}});
    EXPECT_THAT(h.expect_faithful_reload(original).live, IsEmpty());
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, FactsAloneReloadAsTheirOwnFrontier) {
    auto& h = this->h;
    auto& original = h.build({module_spec{.id = 10, .dependencies = {}, .axioms = {
        rule(h.functor("p", {h.atom("a")}), {}, 0),
        rule(h.functor("p", {h.atom("b")}), {}, 0),
        rule(h.functor("q", {h.atom("a")}), {}, 0),
    }}});
    EXPECT_EQ(h.expect_faithful_reload(original).live.size(), 3u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AxiomsWithNothingDerivedReload) {
    auto& h = this->h;
    auto& original = h.build(h.colors());
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ARefutationFoundOnRegistrationIsNotFoundAgainOnReload) {
    auto& h = this->h;
    auto& original = h.build(h.refutations());
    ASSERT_THAT(original.refuted_on_registration, Not(IsEmpty()));
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AFewUnfoldsReload) {
    auto& h = this->h;
    auto& original = h.build(h.colors());
    ASSERT_EQ(h.unfold_randomly(original, 3, std::nullopt), 3u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AProgramUnfoldedToExhaustionReloads) {
    auto& h = this->h;
    auto& original = h.build(h.colors());
    EXPECT_GT(h.unfold_to_exhaustion(original, std::nullopt), 5u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AFrontierOfOnlySolvedLeavesReloads) {
    auto& h = this->h;
    auto& original = h.build(h.colors());
    h.unfold_to_exhaustion(original, std::nullopt);
    for (pud_global_node_id leaf : original.live)
        EXPECT_THAT(h.goal_ids_of(original, leaf), IsEmpty());
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ADeeplyUnfoldedDatabaseReloads) {
    auto& h = this->h;
    auto& original = h.build(h.arithmetic());
    pud_global_node_id deepest{.module = 2001, .node = 1};
    pud_body_goal_id goal = 0;
    for (int level = 0; level < 12; ++level) {
        const pud_frontier_change change = original.unfold(deepest, goal);
        ASSERT_FALSE(change.offspring.empty()) << "level " << level;
        deepest = change.offspring.back();
        ASSERT_THAT(h.goal_ids_of(original, deepest), Not(IsEmpty())) << "level " << level;
        goal = h.goal_ids_of(original, deepest).front();
    }
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ManyRandomUnfoldsOfArithmeticReload) {
    auto& h = this->h;
    h.rng.seed(1);
    auto& original = h.build(h.arithmetic());
    ASSERT_EQ(h.unfold_randomly(original, 40, std::nullopt), 40u);
    EXPECT_GT(h.count_nodes(original), 60u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, UnfoldingOnlyTheQueriesOfArithmeticReloads) {
    auto& h = this->h;
    h.rng.seed(2);
    auto& original = h.build(h.arithmetic());
    ASSERT_EQ(h.unfold_randomly(original, 30, 2002), 30u);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, RandomUnfoldsOfListsReload) {
    auto& h = this->h;
    h.rng.seed(3);
    auto& original = h.build(h.lists());
    ASSERT_EQ(h.unfold_randomly(original, 30, std::nullopt), 30u);
    EXPECT_GT(h.count_nodes(original), 40u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListQueriesUnfoldedToExhaustionReload) {
    auto& h = this->h;
    h.rng.seed(4);
    auto& original = h.build(h.lists());
    EXPECT_GT(h.unfold_to_exhaustion(original, 3002), 3u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, GraphQueriesUnfoldedToExhaustionReload) {
    auto& h = this->h;
    h.rng.seed(5);
    auto& original = h.build(h.graph());
    EXPECT_GT(h.unfold_to_exhaustion(original, 4002), 5u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AGraphUnfoldedEverywhereReloads) {
    auto& h = this->h;
    h.rng.seed(6);
    auto& original = h.build(h.graph());
    EXPECT_GT(h.unfold_randomly(original, 40, std::nullopt), 20u);
    EXPECT_GT(h.count_nodes(original), 50u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersUnfoldedOnlyInTheBottomModuleReload) {
    auto& h = this->h;
    h.rng.seed(7);
    auto& original = h.build(h.tiers());
    EXPECT_GT(h.unfold_randomly(original, 10, 5002), 0u);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersUnfoldedOnlyInTheTopModuleReload) {
    auto& h = this->h;
    h.rng.seed(8);
    auto& original = h.build(h.tiers());
    EXPECT_GT(h.unfold_to_exhaustion(original, 5003), 1u);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TiersUnfoldedEverywhereReload) {
    auto& h = this->h;
    h.rng.seed(9);
    auto& original = h.build(h.tiers());
    EXPECT_GT(h.unfold_to_exhaustion(original, std::nullopt), 2u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SiblingModulesOverOneBaseReload) {
    auto& h = this->h;
    h.rng.seed(10);
    auto& original = h.build(h.siblings());
    ASSERT_EQ(h.unfold_randomly(original, 40, std::nullopt), 40u);
    EXPECT_GT(h.count_nodes(original), 50u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, CascadingRefutationsReload) {
    auto& h = this->h;
    h.rng.seed(11);
    auto& original = h.build(h.refutations());
    EXPECT_GT(h.unfold_to_exhaustion(original, std::nullopt), 2u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NothingBelowARefutedInteriorNodeComesBackOnReload) {
    auto& h = this->h;
    auto& original = h.build(h.cascade());
    original.unfold(pud_global_node_id{.module = 8001, .node = 2}, 0);
    const pud_module& base = original.module_of(8001);
    ASSERT_TRUE(base.check_refuted(2));
    ASSERT_THAT(base.get_children(2), Not(IsEmpty()));
    const auto& reloaded = h.expect_faithful_reload(original);
    for (pud_node_id child : base.get_children(2))
        EXPECT_FALSE(reloaded.live.contains(pud_global_node_id{.module = 8001, .node = child}));
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ARefutationThatCrossedModulesStaysOnReload) {
    auto& h = this->h;
    auto& original = h.build(h.cascade());
    original.unfold(pud_global_node_id{.module = 8001, .node = 2}, 0);
    ASSERT_TRUE(original.module_of(8002).check_refuted(0));
    const auto& reloaded = h.expect_faithful_reload(original);
    EXPECT_FALSE(reloaded.live.contains(pud_global_node_id{.module = 8002, .node = 0}));
    EXPECT_TRUE(reloaded.live.contains(pud_global_node_id{.module = 8002, .node = 1}));
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AReloadOfAReloadIsFaithfulToo) {
    auto& h = this->h;
    h.rng.seed(12);
    auto& original = h.build(h.lists());
    h.unfold_randomly(original, 25, std::nullopt);
    const auto& first = h.expect_faithful_reload(original);
    const auto& second = h.expect_faithful_reload(first);
    EXPECT_EQ(second.live, original.live);
    h.expect_same_modules(original, second);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ReloadingTheSameModulesManyTimesNeverChangesThem) {
    auto& h = this->h;
    h.rng.seed(13);
    auto& original = h.build(h.graph());
    h.unfold_randomly(original, 30, std::nullopt);
    for (int reload_index = 0; reload_index < 6; ++reload_index)
        h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ModuleIdsAreOnlyNames) {
    auto& h = this->h;
    const pud_module_id large = std::numeric_limits<pud_module_id>::max() - 3;
    auto& original = h.build({
        module_spec{.id = large, .dependencies = {}, .axioms = h.nat_axioms()},
        module_spec{.id = 0, .dependencies = {large}, .axioms = {
            rule(h.functor("number", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1),
        }},
    });
    h.rng.seed(14);
    h.unfold_randomly(original, 20, std::nullopt);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, PeanoMulWithManyRandomUnfoldsReloads) {
    auto& h = this->h;
    h.rng.seed(60);
    auto& original = h.build(h.peano_mul());
    ASSERT_EQ(h.unfold_randomly(original, 70, std::nullopt), 70u);
    EXPECT_GT(h.count_nodes(original), 120u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ListMembersUnfoldedToExhaustionReload) {
    auto& h = this->h;
    h.rng.seed(61);
    auto& original = h.build(h.list_members());
    EXPECT_GT(h.unfold_to_exhaustion(original, 9102), 5u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, MirrorTreesWithDeepStructureReload) {
    auto& h = this->h;
    h.rng.seed(62);
    auto& original = h.build(h.mirror_trees());
    EXPECT_GT(h.unfold_to_exhaustion(original, 9202), 2u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, NestedUnaryChainUnfoldsDeeplyThenReloads) {
    auto& h = this->h;
    auto& original = h.build(h.nested_unary_chain());
    const pud_global_node_id deep_axiom{.module = 9301, .node = 1};
    const pud_frontier_change change = original.unfold(deep_axiom, 0);
    ASSERT_FALSE(change.offspring.empty());
    for (int level = 0; level < 10; ++level) {
        const std::optional<unfold_step> step = h.choose_step(original, 9301, goal_choice::leftmost);
        if (!step.has_value())
            break;
        original.unfold(step->leaf, step->goal);
    }
    EXPECT_GT(h.count_nodes(original), 15u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, TripleAppendStressReloads) {
    auto& h = this->h;
    h.rng.seed(63);
    auto& original = h.build(h.triple_append());
    ASSERT_EQ(h.unfold_randomly(original, 55, std::nullopt), 55u);
    EXPECT_GT(h.count_nodes(original), 80u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, SharedBinaryNestedTermsReload) {
    auto& h = this->h;
    h.rng.seed(64);
    auto& original = h.build(h.shared_binary());
    ASSERT_EQ(h.unfold_randomly(original, 40, std::nullopt), 40u);
    h.expect_consistent(original);
    h.expect_faithful_reload(original);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, EveryStressProgramReloadsAfterHeavyRandomUnfolds) {
    auto& h = this->h;
    for (uint32_t seed = 0; seed < 4 && !this->HasFailure(); ++seed) {
        for (const program_t& program : h.stress_programs()) {
            h.rng.seed(300 + seed);
            auto& original = h.build(program);
            const size_t budget = 40 + h.rng() % 40;
            h.unfold_randomly(original, budget, std::nullopt);
            h.expect_consistent(original);
            h.expect_faithful_reload(original);
            if (this->HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, EveryProgramReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    auto& h = this->h;
    for (uint32_t seed = 0; seed < 6 && !this->HasFailure(); ++seed) {
        for (const program_t& program : h.every_program()) {
            h.rng.seed(seed);
            auto& original = h.build(program);
            h.unfold_randomly(original, h.rng() % 30, std::nullopt);
            h.expect_consistent(original);
            h.expect_faithful_reload(original);
            if (this->HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}

// Resuming: a reloaded runtime unfolds exactly as the original goes on unfolding.

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumingFromAxiomsAloneIsLikeNeverHavingReloaded) {
    auto& h = this->h;
    h.rng.seed(20);
    auto& original = h.build(h.arithmetic());
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_EQ(h.resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedProgramUnfoldsToExhaustionLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(21);
    auto& original = h.build(h.colors());
    h.unfold_randomly(original, 4, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_LT(h.resume_in_lockstep(original, resumed, 2000, std::nullopt, goal_choice::leftmost), 2000u);
    EXPECT_FALSE(h.choose_step(original, std::nullopt, goal_choice::leftmost).has_value());
    EXPECT_FALSE(h.choose_step(resumed, std::nullopt, goal_choice::leftmost).has_value());
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedArithmeticUnfoldsLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(22);
    auto& original = h.build(h.arithmetic());
    h.unfold_randomly(original, 20, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_EQ(h.resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedListsUnfoldLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(23);
    auto& original = h.build(h.lists());
    h.unfold_randomly(original, 15, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_EQ(h.resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedGraphQueriesUnfoldToExhaustionLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(24);
    auto& original = h.build(h.graph());
    h.unfold_randomly(original, 10, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_LT(h.resume_in_lockstep(original, resumed, 2000, 4002, goal_choice::leftmost), 2000u);
    EXPECT_FALSE(h.choose_step(original, 4002, goal_choice::leftmost).has_value());
    EXPECT_FALSE(h.choose_step(resumed, 4002, goal_choice::leftmost).has_value());
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedTiersUnfoldLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(25);
    auto& original = h.build(h.tiers());
    h.unfold_randomly(original, 3, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_LT(h.resume_in_lockstep(original, resumed, 2000, std::nullopt, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedSiblingModulesUnfoldLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(26);
    auto& original = h.build(h.siblings());
    h.unfold_randomly(original, 20, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_EQ(h.resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedRefutationsCascadeLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(27);
    auto& original = h.build(h.refutations());
    h.unfold_randomly(original, 2, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_LT(h.resume_in_lockstep(original, resumed, 2000, std::nullopt, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedUnfoldInADependencyRefutesTheDependentsLeavesLikeTheOriginal) {
    auto& h = this->h;
    auto& original = h.build(h.cascade());
    auto& resumed = h.expect_faithful_reload(original);
    const pud_global_node_id mid{.module = 8001, .node = 2};
    const pud_frontier_change expected = original.unfold(mid, 0);
    const pud_frontier_change actual = resumed.unfold(mid, 0);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    EXPECT_TRUE(resumed.module_of(8002).check_refuted(0));
    h.expect_same_modules(original, resumed);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedUnfoldInADependencyKeepsADependentsQueryResolvable) {
    auto& h = this->h;
    auto& original = h.build({
        module_spec{.id = 1, .dependencies = {}, .axioms = h.nat_axioms()},
        module_spec{.id = 2, .dependencies = {1}, .axioms = {rule(h.atom("q"), {h.functor("nat", {h.peano(3)})}, 0)}},
    });
    auto& resumed = h.expect_faithful_reload(original);
    const pud_global_node_id step{.module = 1, .node = 1};
    original.unfold(step, 0);
    resumed.unfold(step, 0);
    const pud_global_node_id query{.module = 2, .node = 0};
    const pud_frontier_change expected = original.unfold(query, 0);
    const pud_frontier_change actual = resumed.unfold(query, 0);
    ASSERT_EQ(actual.offspring.size(), 1u);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    h.expect_same_modules(original, resumed);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ReloadingAfterEveryUnfoldKeepsUnfoldingLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(28);
    auto& original = h.build(h.arithmetic());
    auto* resumed = &h.expect_faithful_reload(original);
    for (int unfold_index = 0; unfold_index < 20 && !this->HasFailure(); ++unfold_index) {
        ASSERT_TRUE(h.unfold_in_lockstep(original, *resumed, std::nullopt, goal_choice::random)) << "unfold " << unfold_index;
        resumed = &h.expect_faithful_reload(*resumed);
    }
    h.expect_same_modules(original, *resumed);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ReloadingEveryFewUnfoldsKeepsUnfoldingLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(29);
    auto& original = h.build(h.lists());
    auto* resumed = &h.expect_faithful_reload(original);
    for (int generation = 0; generation < 6 && !this->HasFailure(); ++generation) {
        h.resume_in_lockstep(original, *resumed, 6, std::nullopt, goal_choice::random);
        resumed = &h.expect_faithful_reload(*resumed);
    }
    EXPECT_EQ(resumed->live, original.live);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, AResumedRuntimeCanItselfBeReloadedAndResumed) {
    auto& h = this->h;
    h.rng.seed(30);
    auto& original = h.build(h.graph());
    h.unfold_randomly(original, 10, std::nullopt);
    auto& first = h.expect_faithful_reload(original);
    h.resume_in_lockstep(original, first, 15, std::nullopt, goal_choice::random);
    auto& second = h.expect_faithful_reload(first);
    h.resume_in_lockstep(original, second, 15, std::nullopt, goal_choice::random);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedPeanoMulContinuesLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(70);
    auto& original = h.build(h.peano_mul());
    h.unfold_randomly(original, 35, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_EQ(h.resume_in_lockstep(original, resumed, 50, std::nullopt, goal_choice::random), 50u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedTripleAppendContinuesLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(71);
    auto& original = h.build(h.triple_append());
    h.unfold_randomly(original, 25, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_EQ(h.resume_in_lockstep(original, resumed, 45, std::nullopt, goal_choice::random), 45u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, ResumedMirrorTreesUnfoldToExhaustionLikeTheOriginal) {
    auto& h = this->h;
    h.rng.seed(72);
    auto& original = h.build(h.mirror_trees());
    h.unfold_randomly(original, 5, std::nullopt);
    auto& resumed = h.expect_faithful_reload(original);
    EXPECT_LT(h.resume_in_lockstep(original, resumed, 2000, 9202, goal_choice::leftmost), 2000u);
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, EveryStressProgramResumesAfterHeavyUnfolds) {
    auto& h = this->h;
    for (uint32_t seed = 0; seed < 3 && !this->HasFailure(); ++seed) {
        for (const program_t& program : h.stress_programs()) {
            h.rng.seed(400 + seed);
            auto& original = h.build(program);
            h.unfold_randomly(original, 20 + h.rng() % 30, std::nullopt);
            auto& resumed = h.expect_faithful_reload(original);
            h.resume_in_lockstep(original, resumed, 35, std::nullopt, goal_choice::random);
            if (this->HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}

TYPED_TEST(PudRuntimeReloadIntegrationTest, EveryProgramResumesLikeTheOriginalFromAnyPoint) {
    auto& h = this->h;
    for (uint32_t seed = 0; seed < 5 && !this->HasFailure(); ++seed) {
        for (const program_t& program : h.every_program()) {
            h.rng.seed(100 + seed);
            auto& original = h.build(program);
            h.unfold_randomly(original, h.rng() % 25, std::nullopt);
            auto& resumed = h.expect_faithful_reload(original);
            h.resume_in_lockstep(original, resumed, 20, std::nullopt, goal_choice::random);
            if (this->HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}
