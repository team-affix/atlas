#include <set>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_random_unfold_manifest.hpp"
#include "value_objects/rule.hpp"

using ::testing::IsEmpty;

namespace {

// Builds programs into one manifest.
struct unfold_program_builder {
    pud_random_unfold_manifest& manifest;
    test_functors functors;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        return manifest.axiom_initializer_.initialize_axiom(rule(head, std::move(body), var_count));
    }

    void run_base_descent() {
        for (pud_node_id root : manifest.root_nodes_.get())
            manifest.base_descent_runner_.run(root);
    }

    // Axioms 0-3: color/warm facts and warmcolor, a finite rule over them.
    void add_colors() {
        axiom(functor("color", {atom("red")}), {}, 0);
        axiom(functor("color", {atom("blue")}), {}, 0);
        axiom(functor("warm", {atom("red")}), {}, 0);
        axiom(functor("warmcolor", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    }

    // Axioms 4-5: nat, an infinite recursion.
    void add_nat() {
        axiom(functor("nat", {atom("z")}), {}, 0);
        axiom(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    }
};

std::vector<pud_unfold> unfold_steps(pud_random_unfold_manifest& manifest, size_t steps) {
    std::vector<pud_unfold> unfolds;
    coroutine<pud_unfold, void> unfolding = manifest.unfold_loop_.drive_unfold_loop();
    for (size_t step = 0; step < steps; ++step) {
        unfolding.resume();
        if (!unfolding.has_yield())
            break;
        unfolds.push_back(unfolding.consume_yield());
    }
    return unfolds;
}

}  // namespace

struct PudRandomUnfoldManifestIntegrationTest : public ::testing::Test {
    pud_random_unfold_manifest manifest{0};
    unfold_program_builder program{manifest, {}};

    bool has_descent(pud_node_id node) { return manifest.node_descents_.check_descent(node); }

    // Every live leaf has a stored descent and nothing else does; exactly those with open goals are open.
    void check_open_leaves() {
        for (pud_node_id node = 0; node < manifest.node_id_sequencer_.peek(); ++node) {
            const bool live_leaf = manifest.leaf_nodes_.check_leaf(node)
                && !manifest.refuted_node_checker_.check_refuted(node);
            EXPECT_EQ(has_descent(node), live_leaf) << "node " << node;
            const bool should_be_open = live_leaf
                && !manifest.node_descents_.get(node).body_goal_expr().body_goal_exprs().empty();
            EXPECT_EQ(manifest.open_leaves_.check_leaf_open(node), should_be_open) << "node " << node;
        }
    }
};

TEST_F(PudRandomUnfoldManifestIntegrationTest, RunningTheBaseDescentOpensEveryRuleAndNoFact) {
    program.add_colors();
    program.add_nat();
    program.run_base_descent();
    EXPECT_EQ(manifest.open_leaves_.get(), (std::set<pud_node_id>{3, 5}));
    check_open_leaves();
}

TEST_F(PudRandomUnfoldManifestIntegrationTest, RuleWhoseGoalHasNoCandidatesIsRefutedByTheBaseDescent) {
    const pud_node_id orphan = program.axiom(
        program.functor("p", {program.var(0)}), {program.functor("missing", {program.var(0)})}, 1);
    program.run_base_descent();
    EXPECT_TRUE(manifest.refuted_node_checker_.check_refuted(orphan));
    EXPECT_THAT(manifest.open_leaves_.get(), IsEmpty());
    check_open_leaves();
}

TEST_F(PudRandomUnfoldManifestIntegrationTest, FiniteDatabaseUnfoldsUntilNothingIsOpen) {
    program.add_colors();
    program.run_base_descent();
    const std::vector<pud_unfold> unfolds = unfold_steps(manifest, 100);
    EXPECT_LT(unfolds.size(), 100u);
    EXPECT_THAT(manifest.open_leaves_.get(), IsEmpty());
    check_open_leaves();
}

TEST_F(PudRandomUnfoldManifestIntegrationTest, EveryUnfoldIsOfAnOpenLeafOverOneOfItsOpenGoals) {
    program.add_colors();
    program.add_nat();
    program.run_base_descent();
    coroutine<pud_unfold, void> unfolding = manifest.unfold_loop_.drive_unfold_loop();
    for (int step = 0; step < 40; ++step) {
        const std::set<pud_node_id> open_before = manifest.open_leaves_.get();
        unfolding.resume();
        ASSERT_TRUE(unfolding.has_yield());
        const pud_unfold next = unfolding.consume_yield();
        EXPECT_TRUE(open_before.contains(next.leaf)) << next.leaf;
        EXPECT_FALSE(manifest.leaf_nodes_.check_leaf(next.leaf)) << next.leaf;
        EXPECT_EQ(manifest.call_sites_.get(next.leaf), next.goal);
        check_open_leaves();
    }
}

TEST_F(PudRandomUnfoldManifestIntegrationTest, UnfoldingReachesEveryAxiomTree) {
    program.add_colors();
    program.add_nat();
    program.run_base_descent();
    std::set<pud_node_id> unfolded;
    for (const pud_unfold& next : unfold_steps(manifest, 60))
        unfolded.insert(next.leaf);
    EXPECT_TRUE(unfolded.contains(3)) << "warmcolor tree never unfolded";
    EXPECT_TRUE(unfolded.contains(5)) << "nat tree never unfolded";
}

TEST_F(PudRandomUnfoldManifestIntegrationTest, SameSeedUnfoldsTheSameWay) {
    pud_random_unfold_manifest again{0};
    unfold_program_builder again_program{again, {}};
    for (unfold_program_builder* builder : {&program, &again_program}) {
        builder->add_colors();
        builder->add_nat();
        builder->run_base_descent();
    }
    EXPECT_EQ(unfold_steps(manifest, 30), unfold_steps(again, 30));
}

TEST_F(PudRandomUnfoldManifestIntegrationTest, DecisionsAloneDriveTheLoop) {
    program.add_colors();
    program.run_base_descent();
    const std::set<pud_node_id> open_before = manifest.open_leaves_.get();
    const std::vector<pud_unfold> unfolds = unfold_steps(manifest, 1);
    ASSERT_EQ(unfolds.size(), 1u);
    EXPECT_TRUE(open_before.contains(unfolds.front().leaf));
}
