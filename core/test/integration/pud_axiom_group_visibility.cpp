#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

struct PudAxiomGroupVisibilityIntegrationTest : public ::testing::Test {
    pud_manifest manifest;
    test_functors functors;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }

    const expr* atom(const char* name) {
        return manifest.expr_pool_.make_functor(functors.id(name), {});
    }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), std::move(args));
    }

    pud_axiom_group_id make_group() { return manifest.axiom_groups_.create_group(); }

    std::vector<pud_node_id> unactivated_roots;

    pud_node_id add_rule(const rule& axiom, pud_axiom_group_id group) {
        const pud_node_id root = manifest.node_maker_.make_root(axiom, group);
        unactivated_roots.push_back(root);
        return root;
    }

    void activate_axioms() {
        for (pud_node_id root : unactivated_roots) {
            if (!manifest.root_activator_.activate_root(root))
                continue;
            manifest.leaf_refuter_.refute(root);
        }
        unactivated_roots.clear();
    }

    std::vector<pud_node_id> candidate_axiom_roots(pud_node_id leaf, pud_body_goal_id goal) {
        const pud_mhq_head_id head = manifest.active_states_.get(leaf).queries.at(goal);
        std::vector<pud_node_id> roots;
        for (pud_mhcs_head_id candidate : manifest.mhq_.candidates(head))
            roots.push_back(manifest.mhcs_.path(candidate).front());
        return roots;
    }

    pud_node_id first_live_offspring(const pud_frontier_change& change) {
        for (pud_node_id node : change.offspring) {
            const bool refuted =
                std::ranges::find(change.refuted, node) != change.refuted.end();
            if (!refuted)
                return node;
        }
        EXPECT_FALSE(change.offspring.empty());
        return change.offspring.front();
    }
};

TEST_F(PudAxiomGroupVisibilityIntegrationTest, ADependeeGroupDoesNotSeeRulesFromADependentGroup) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id goals = make_group();
    manifest.axiom_groups_.add_dependency(goals, base);
    const pud_node_id fact = add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id goal_root =
        add_rule(rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1), goals);
    activate_axioms();
    const std::vector<pud_node_id> candidates = candidate_axiom_roots(goal_root, 0);
    EXPECT_EQ(candidates.size(), 1u);
    EXPECT_EQ(candidates.front(), fact);
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(base), ElementsAre(fact));
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(goals), ElementsAre(fact, goal_root));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, AThreeTierChainExposesEveryLowerGroupToTheTop) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id middle = make_group();
    const pud_axiom_group_id top = make_group();
    manifest.axiom_groups_.add_dependency(middle, base);
    manifest.axiom_groups_.add_dependency(top, middle);
    const pud_node_id base_fact = add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id middle_fact = add_rule(rule(functor("r", {atom("b")}), {}, 0), middle);
    const pud_node_id top_goal =
        add_rule(rule(functor("q", {var(0)}), {functor("p", {var(0)}), functor("r", {var(0)})}, 1), top);
    activate_axioms();
    EXPECT_THAT(candidate_axiom_roots(top_goal, 0), ElementsAre(base_fact));
    EXPECT_THAT(candidate_axiom_roots(top_goal, 1), ElementsAre(middle_fact));
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(base), ElementsAre(base_fact));
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(middle), ElementsAre(base_fact, middle_fact));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, SiblingGoalGroupsDoNotSeeEachOthersRules) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id left = make_group();
    const pud_axiom_group_id right = make_group();
    manifest.axiom_groups_.add_dependency(left, base);
    manifest.axiom_groups_.add_dependency(right, base);
    const pud_node_id base_fact = add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id left_only = add_rule(rule(functor("l", {atom("x")}), {}, 0), left);
    const pud_node_id right_only = add_rule(rule(functor("r", {atom("y")}), {}, 0), right);
    const pud_node_id left_goal =
        add_rule(rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1), left);
    activate_axioms();
    EXPECT_THAT(candidate_axiom_roots(left_goal, 0), ElementsAre(base_fact));
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(left), ElementsAre(base_fact, left_only, left_goal));
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(right), ElementsAre(base_fact, right_only));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, RulesInTheSameGroupAreCandidatesForEachOther) {
    const pud_axiom_group_id goals = make_group();
    const pud_node_id lemma = add_rule(rule(functor("p", {atom("a")}), {}, 0), goals);
    const pud_node_id query =
        add_rule(rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1), goals);
    activate_axioms();
    EXPECT_THAT(candidate_axiom_roots(query, 0), ElementsAre(lemma));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, CandidateOrderFollowsDependencyGroupsThenRegistration) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id goals = make_group();
    manifest.axiom_groups_.add_dependency(goals, base);
    const pud_node_id red = add_rule(rule(functor("color", {atom("red")}), {}, 0), base);
    const pud_node_id blue = add_rule(rule(functor("color", {atom("blue")}), {}, 0), base);
    const pud_node_id warm = add_rule(rule(functor("warm", {atom("red")}), {}, 0), goals);
    const pud_node_id query =
        add_rule(rule(functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1), goals);
    activate_axioms();
    EXPECT_THAT(candidate_axiom_roots(query, 0), ElementsAre(red, blue));
    EXPECT_THAT(candidate_axiom_roots(query, 1), ElementsAre(warm));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, ExpandingInADependentGroupUsesBaseGroupAxioms) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id goals = make_group();
    manifest.axiom_groups_.add_dependency(goals, base);
    const pud_node_id base_fact = add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id goals_lemma = add_rule(rule(functor("helper", {atom("a")}), {}, 0), goals);
    const pud_node_id query = add_rule(
        rule(functor("q", {var(0)}), {functor("p", {var(0)}), functor("helper", {var(0)})}, 1), goals);
    activate_axioms();
    const pud_frontier_change change = manifest.expander_.expand(query, 0);
    const pud_node_id child = first_live_offspring(change);
    EXPECT_EQ(manifest.node_axiom_groups_.get(child).value, goals.value);
    (void)base_fact;
    EXPECT_THAT(candidate_axiom_roots(child, 1), ElementsAre(goals_lemma));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, ExpandedChildInOneGoalGroupDoesNotSeeSiblingGroupAxioms) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id left = make_group();
    const pud_axiom_group_id right = make_group();
    manifest.axiom_groups_.add_dependency(left, base);
    manifest.axiom_groups_.add_dependency(right, base);
    add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id left_lemma = add_rule(rule(functor("l", {atom("a")}), {}, 0), left);
    const pud_node_id right_lemma = add_rule(rule(functor("r", {atom("a")}), {}, 0), right);
    const pud_node_id left_query = add_rule(
        rule(functor("q", {var(0)}), {functor("p", {var(0)}), functor("l", {var(0)})}, 1), left);
    activate_axioms();
    const pud_frontier_change change = manifest.expander_.expand(left_query, 0);
    const pud_node_id child = first_live_offspring(change);
    EXPECT_EQ(manifest.node_axiom_groups_.get(child).value, left.value);
    EXPECT_THAT(candidate_axiom_roots(child, 1), ElementsAre(left_lemma));
    EXPECT_THAT(candidate_axiom_roots(child, 1), ::testing::Not(::testing::Contains(right_lemma)));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, TwoStepExpandThroughTieredGroupsKeepsGroupAndDependencies) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id middle = make_group();
    const pud_axiom_group_id top = make_group();
    manifest.axiom_groups_.add_dependency(middle, base);
    manifest.axiom_groups_.add_dependency(top, middle);
    add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id middle_fact = add_rule(rule(functor("m", {atom("a")}), {}, 0), middle);
    const pud_node_id top_query =
        add_rule(rule(functor("q", {var(0)}), {functor("p", {var(0)}), functor("m", {var(0)})}, 1), top);
    activate_axioms();
    const pud_node_id child = first_live_offspring(manifest.expander_.expand(top_query, 0));
    EXPECT_EQ(manifest.node_axiom_groups_.get(child).value, top.value);
    EXPECT_THAT(candidate_axiom_roots(child, 1), ElementsAre(middle_fact));
    const pud_frontier_change second = manifest.expander_.expand(child, 1);
    ASSERT_EQ(second.offspring.size(), 1u);
    const pud_node_id grandchild = second.offspring.front();
    EXPECT_EQ(manifest.node_axiom_groups_.get(grandchild).value, top.value);
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, AnExpandedChildKeepsItsGroupsVisibility) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id goals = make_group();
    manifest.axiom_groups_.add_dependency(goals, base);
    add_rule(rule(functor("color", {atom("red")}), {}, 0), base);
    add_rule(rule(functor("color", {atom("blue")}), {}, 0), base);
    const pud_node_id outer_only = add_rule(rule(functor("warm", {atom("red")}), {}, 0), goals);
    const pud_node_id query =
        add_rule(rule(functor("q", {var(0), var(1)}), {functor("color", {var(0)}), functor("warm", {var(1)})}, 2), goals);
    activate_axioms();
    const pud_frontier_change change = manifest.expander_.expand(query, 0);
    const pud_node_id child = first_live_offspring(change);
    EXPECT_EQ(manifest.node_axiom_groups_.get(child).value, goals.value);
    EXPECT_THAT(candidate_axiom_roots(child, 1), ElementsAre(outer_only));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, TwoIndependentGoalGroupsOnOneBaseStayIsolated) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id first_goals = make_group();
    const pud_axiom_group_id second_goals = make_group();
    manifest.axiom_groups_.add_dependency(first_goals, base);
    manifest.axiom_groups_.add_dependency(second_goals, base);
    const pud_node_id base_fact = add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    const pud_node_id first_lemma = add_rule(rule(functor("f", {atom("x")}), {}, 0), first_goals);
    const pud_node_id second_lemma = add_rule(rule(functor("g", {atom("y")}), {}, 0), second_goals);
    const pud_node_id first_query =
        add_rule(rule(functor("q1", {var(0)}), {functor("p", {var(0)})}, 1), first_goals);
    const pud_node_id second_query =
        add_rule(rule(functor("q2", {var(0)}), {functor("p", {var(0)})}, 1), second_goals);
    activate_axioms();
    EXPECT_THAT(candidate_axiom_roots(first_query, 0), ElementsAre(base_fact));
    EXPECT_THAT(candidate_axiom_roots(second_query, 0), ElementsAre(base_fact));
    const std::vector<pud_node_id> first_visible = manifest.axiom_groups_.axiom_roots(first_goals);
    EXPECT_THAT(first_visible, ::testing::Not(::testing::Contains(second_lemma)));
    EXPECT_THAT(first_visible, ::testing::Contains(first_lemma));
    EXPECT_THAT(first_visible, ::testing::Contains(first_query));
}

TEST_F(PudAxiomGroupVisibilityIntegrationTest, DatabaseGroupLoadsOnlySeeDatabaseRules) {
    const pud_axiom_group_id base = manifest.database_axiom_group_;
    const pud_axiom_group_id goals = make_group();
    manifest.axiom_groups_.add_dependency(goals, base);
    const pud_node_id base_rule = add_rule(rule(functor("p", {atom("a")}), {}, 0), base);
    add_rule(rule(functor("q", {var(0)}), {functor("p", {var(0)})}, 1), goals);
    activate_axioms();
    EXPECT_THAT(manifest.axiom_groups_.axiom_roots(base), ElementsAre(base_rule));
}
