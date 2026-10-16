#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "pud_datalog_oracle.hpp"
#include "pud_manifest_harness.hpp"

namespace {

using check_t = void (pud_manifest_harness::*)();
using build_t = pud_node_id (*)(pud_manifest_harness&);

pud_node_id build_colors_conjunction(pud_manifest_harness& h) {
    h.add_colors();
    return h.start_query(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
}

pud_node_id build_colors_independent(pud_manifest_harness& h) {
    h.add_colors();
    return h.start_query(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
}

pud_node_id build_nat_enumeration(pud_manifest_harness& h) {
    h.add_nat();
    return h.start_query(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
}

pud_node_id build_nat_ground(pud_manifest_harness& h) {
    h.add_nat();
    return h.start_query(h.atom("q"), {h.functor("nat", {h.peano(4)})}, 0);
}

pud_node_id build_append_split(pud_manifest_harness& h) {
    h.add_append();
    return h.start_query(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("a"), h.atom("b"), h.atom("c")})})},
        2);
}

pud_node_id build_member(pud_manifest_harness& h) {
    h.add_member();
    return h.start_query(
        h.functor("q", {h.var(0)}),
        {h.functor("mem", {h.var(0), h.list({h.atom("a"), h.atom("b"), h.atom("c")})})},
        1);
}

pud_node_id build_peano_add(pud_manifest_harness& h) {
    h.add_peano_add();
    return h.start_query(h.functor("q", {h.var(0), h.var(1)}), {h.functor("add", {h.var(0), h.var(1), h.peano(3)})}, 2);
}

pud_node_id build_graph(pud_manifest_harness& h) {
    h.add_graph();
    return h.start_query(h.functor("q", {h.var(0)}), {h.functor("anc", {h.atom("a"), h.var(0)})}, 1);
}

pud_node_id build_cross_goal_merge(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    return h.start_query(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
}

pud_node_id build_self_candidate(pud_manifest_harness& h) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    return h.start_query(h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
}

pud_node_id build_grown_lemma(pud_manifest_harness& h) {
    const pud_node_id rule = h.add_nat();
    h.start_tree(rule);
    h.grow_along_open_goals(rule, 3);
    return h.start_query(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
}

pud_node_id build_occurs_check(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    return h.start_query(h.atom("q"), {h.functor("eq", {h.var(0), h.functor("f", {h.var(0)})}), h.functor("color", {h.var(0)})}, 1);
}

pud_node_id build_query_as_lemma(pud_manifest_harness& h) {
    h.add_colors();
    const pud_node_id first = h.start_query(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.next_solution(first, 2);
    return h.start_query(h.functor("q", {h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
}

pud_node_id build_shared_deep(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.atom("a1"), {}, 0);
    h.axiom(h.atom("a2"), {}, 0);
    h.axiom(h.atom("a3"), {}, 0);
    return h.start_query(
        h.functor("q", {h.var(0)}),
        {h.atom("a1"), h.atom("a2"), h.atom("a3"), h.functor("color", {h.var(0)})},
        1);
}

pud_node_id build_random_nonrecursive(pud_manifest_harness& h) {
    return load_datalog(h, generate_datalog(7, false));
}

pud_node_id build_random_recursive(pud_manifest_harness& h) {
    return load_datalog(h, generate_datalog(11, true));
}

struct scenario {
    std::string name;
    build_t build;
    size_t steps;
};

const std::vector<scenario>& corpus() {
    static const std::vector<scenario> scenarios{
        {"colors_conjunction", build_colors_conjunction, 60},
        {"colors_independent", build_colors_independent, 60},
        {"nat_enumeration", build_nat_enumeration, 120},
        {"nat_ground", build_nat_ground, 60},
        {"append_split", build_append_split, 90},
        {"member", build_member, 90},
        {"peano_add", build_peano_add, 90},
        {"graph", build_graph, 90},
        {"cross_goal_merge", build_cross_goal_merge, 90},
        {"self_candidate", build_self_candidate, 90},
        {"grown_lemma", build_grown_lemma, 90},
        {"occurs_check", build_occurs_check, 30},
        {"query_as_lemma", build_query_as_lemma, 60},
        {"shared_deep", build_shared_deep, 60},
        {"random_nonrecursive", build_random_nonrecursive, 120},
        {"random_recursive", build_random_recursive, 120},
    };
    return scenarios;
}

void sweep(check_t check) {
    for (const scenario& each : corpus()) {
        SCOPED_TRACE(each.name);
        auto harness = std::make_unique<pud_manifest_harness>();
        const pud_node_id query = each.build(*harness);
        std::vector<const expr*> answers;
        ((*harness).*check)();
        for (size_t step = 0; step < each.steps; ++step) {
            if (!harness->step(query, answers))
                break;
            SCOPED_TRACE("step " + std::to_string(step));
            ((*harness).*check)();
            if (::testing::Test::HasFailure())
                return;
        }
    }
}

}  // namespace

TEST(PudManifestInvariantsTest, LiveLeavesAreExactlyTheStoredDescents) {
    sweep(&pud_manifest_harness::check_live_leaves_are_exactly_the_stored_descents);
}

TEST(PudManifestInvariantsTest, LeafFlagMatchesHavingChildren) {
    sweep(&pud_manifest_harness::check_leaf_flag_matches_having_children);
}

TEST(PudManifestInvariantsTest, ParentsAndChildrenAgree) {
    sweep(&pud_manifest_harness::check_parents_and_children_agree);
}

TEST(PudManifestInvariantsTest, LiveChildCountsMatchUnrefutedChildren) {
    sweep(&pud_manifest_harness::check_live_child_counts_match_unrefuted_children);
}

TEST(PudManifestInvariantsTest, RefutationIsClosedUpward) {
    sweep(&pud_manifest_harness::check_refutation_is_closed_upward);
}

TEST(PudManifestInvariantsTest, RefutationIsClosedDownward) {
    sweep(&pud_manifest_harness::check_refutation_is_closed_downward);
}

TEST(PudManifestInvariantsTest, LeafLvcIsTheSumOfVarCountsOnItsPath) {
    sweep(&pud_manifest_harness::check_leaf_lvc_is_the_sum_of_var_counts_on_its_path);
}

TEST(PudManifestInvariantsTest, LeafBindingMatchesItsReplayedSubstitutions) {
    sweep(&pud_manifest_harness::check_leaf_binding_matches_its_replayed_substitutions);
}

TEST(PudManifestInvariantsTest, StoredGoalsAreInTheirNodesRepresentatives) {
    sweep(&pud_manifest_harness::check_stored_goals_are_in_their_nodes_representatives);
}

TEST(PudManifestInvariantsTest, SubstitutionsMeetTheWatchersPreconditions) {
    sweep(&pud_manifest_harness::check_substitutions_meet_the_watchers_preconditions);
}

TEST(PudManifestInvariantsTest, FreshVariablesAreDenseFromTheParentsLvc) {
    sweep(&pud_manifest_harness::check_fresh_variables_are_dense_from_the_parents_lvc);
}

TEST(PudManifestInvariantsTest, EveryOpenGoalHasALiveQuery) {
    sweep(&pud_manifest_harness::check_every_open_goal_has_a_live_query);
}

TEST(PudManifestInvariantsTest, QueryHeadNodeIsAnAncestorOfEveryLeafHoldingIt) {
    sweep(&pud_manifest_harness::check_query_head_node_is_an_ancestor_of_every_leaf_holding_it);
}

TEST(PudManifestInvariantsTest, EveryCandidatePathIsARealRootFirstChain) {
    sweep(&pud_manifest_harness::check_every_candidate_path_is_a_real_root_first_chain);
}

TEST(PudManifestInvariantsTest, EveryCandidateIsAcceptableAtEveryLeafHoldingTheQuery) {
    sweep(&pud_manifest_harness::check_every_candidate_is_acceptable_at_every_leaf_holding_the_query);
}

TEST(PudManifestInvariantsTest, RandomProgramsKeepInvariants) {
    for (unsigned seed = 100; seed < 200; ++seed) {
        for (bool recursive : {false, true}) {
            SCOPED_TRACE("seed " + std::to_string(seed) + (recursive ? " recursive" : " flat"));
            auto harness = std::make_unique<pud_manifest_harness>();
            const pud_node_id query = load_datalog(*harness, generate_datalog(seed, recursive));
            std::vector<const expr*> answers;
            harness->check_all();
            for (size_t step = 0; step < 40; ++step) {
                if (!harness->step(query, answers))
                    break;
                harness->check_all();
                if (::testing::Test::HasFailure())
                    return;
            }
        }
    }
}

TEST(PudManifestInvariantsTest, DeepRecursionStaysConsistent) {
    auto harness = std::make_unique<pud_manifest_harness>();
    const pud_node_id query = build_nat_enumeration(*harness);
    std::vector<const expr*> answers;
    for (size_t step = 0; step < 150; ++step) {
        ASSERT_TRUE(harness->step(query, answers));
        if (step % 5 != 0)
            continue;
        harness->check_all();
        if (::testing::Test::HasFailure())
            return;
    }
    harness->check_all();
    EXPECT_GE(answers.size(), 10u);
    for (size_t index = 0; index < answers.size(); ++index)
        EXPECT_EQ(answers.at(index), harness->functor("q", {harness->peano(static_cast<uint32_t>(index))}));
}
