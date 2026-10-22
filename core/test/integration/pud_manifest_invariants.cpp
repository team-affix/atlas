#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "pud_datalog_oracle.hpp"
#include "pud_manifest_harness.hpp"

namespace {
using check_t = void (pud_manifest_harness<pud_manifest>::*)();
using build_t = void (*)(pud_manifest_harness<pud_manifest>&);

void build_colors(pud_manifest_harness<pud_manifest>& h) {
    h.add_colors();
    h.axiom(h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
}

void build_nat(pud_manifest_harness<pud_manifest>& h) {
    h.add_nat();
}

void build_append(pud_manifest_harness<pud_manifest>& h) {
    h.add_append();
}

void build_member(pud_manifest_harness<pud_manifest>& h) {
    h.add_member();
}

void build_peano_add(pud_manifest_harness<pud_manifest>& h) {
    h.add_peano_add();
}

void build_graph(pud_manifest_harness<pud_manifest>& h) {
    h.add_graph();
}

void build_chain(pud_manifest_harness<pud_manifest>& h) {
    for (int link = 0; link < 20; ++link)
        h.axiom(h.atom("p" + std::to_string(link)), {h.atom("p" + std::to_string(link + 1))}, 0);
    h.axiom(h.atom("p20"), {}, 0);
}

void build_fan_out(pud_manifest_harness<pud_manifest>& h) {
    for (int index = 0; index < 30; ++index)
        h.axiom(h.functor("f", {h.atom("k" + std::to_string(index))}), {}, 0);
    h.axiom(h.functor("g", {h.var(0)}), {h.functor("f", {h.var(0)})}, 1);
}

void build_join(pud_manifest_harness<pud_manifest>& h) {
    h.axiom(h.functor("eq", {h.atom("a"), h.atom("a")}), {}, 0);
    h.axiom(h.functor("eq", {h.atom("b"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("a")}), {}, 0);
    h.axiom(h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("pair", {h.var(0), h.var(1)})}, 2);
}

void build_mutual_recursion(pud_manifest_harness<pud_manifest>& h) {
    h.axiom(h.atom("p"), {h.atom("q")}, 0);
    h.axiom(h.atom("q"), {h.atom("p")}, 0);
}

void build_refuted_chain(pud_manifest_harness<pud_manifest>& h) {
    h.axiom(h.atom("a"), {h.atom("b")}, 0);
    h.axiom(h.atom("b"), {h.atom("c")}, 0);
    h.axiom(h.functor("mid", {h.var(0)}), {h.functor("base", {h.var(0)}), h.functor("check", {h.var(0)})}, 1);
    h.axiom(h.functor("base", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("check", {h.atom("a")}), {}, 0);
    h.axiom(h.atom("top"), {h.functor("mid", {h.atom("c")})}, 0);
}

struct program {
    std::string name;
    build_t build;
    size_t unfolds;
};

std::vector<program> corpus() {
    return {
        {"colors", &build_colors, 30},
        {"nat", &build_nat, 30},
        {"append", &build_append, 30},
        {"member", &build_member, 30},
        {"peano_add", &build_peano_add, 30},
        {"graph", &build_graph, 30},
        {"chain", &build_chain, 40},
        {"fan_out", &build_fan_out, 40},
        {"join", &build_join, 30},
        {"mutual_recursion", &build_mutual_recursion, 30},
        {"refuted_chain", &build_refuted_chain, 30},
    };
}

void sweep(check_t check) {
    for (uint32_t random_seed = 0; random_seed < 4; ++random_seed) {
        for (const program& each : corpus()) {
            SCOPED_TRACE(each.name + " seed " + std::to_string(random_seed));
            auto harness = std::make_unique<pud_manifest_harness<pud_manifest>>(random_seed, pud_harness_scope::every_open_leaf);
            each.build(*harness);
            harness->run_base_descent();
            (harness.get()->*check)();
            for (size_t unfold = 0; unfold < each.unfolds; ++unfold) {
                if (!harness->advance_unfold_loop())
                    break;
                (harness.get()->*check)();
            }
        }
    }
}

}

TEST(PudManifestInvariantsTest, LiveLeavesAreExactlyTheStoredDescents) {
    sweep(&pud_manifest_harness<pud_manifest>::check_live_leaves_are_exactly_the_stored_descents);
}

TEST(PudManifestInvariantsTest, OpenLeavesAreExactlyTheLiveLeavesWithOpenGoals) {
    sweep(&pud_manifest_harness<pud_manifest>::check_open_leaves_are_exactly_the_live_leaves_with_open_goals);
}

TEST(PudManifestInvariantsTest, LeafFlagMatchesHavingChildren) {
    sweep(&pud_manifest_harness<pud_manifest>::check_leaf_flag_matches_having_children);
}

TEST(PudManifestInvariantsTest, ParentsAndChildrenAgree) {
    sweep(&pud_manifest_harness<pud_manifest>::check_parents_and_children_agree);
}

TEST(PudManifestInvariantsTest, LiveChildCountsMatchUnrefutedChildren) {
    sweep(&pud_manifest_harness<pud_manifest>::check_live_child_counts_match_unrefuted_children);
}

TEST(PudManifestInvariantsTest, RefutationIsClosedUpward) {
    sweep(&pud_manifest_harness<pud_manifest>::check_refutation_is_closed_upward);
}

TEST(PudManifestInvariantsTest, RefutationIsClosedDownward) {
    sweep(&pud_manifest_harness<pud_manifest>::check_refutation_is_closed_downward);
}

TEST(PudManifestInvariantsTest, LeafLvcIsTheSumOfVarCountsOnItsPath) {
    sweep(&pud_manifest_harness<pud_manifest>::check_leaf_lvc_is_the_sum_of_var_counts_on_its_path);
}

TEST(PudManifestInvariantsTest, LeafBindingMatchesItsReplayedSubstitutions) {
    sweep(&pud_manifest_harness<pud_manifest>::check_leaf_binding_matches_its_replayed_substitutions);
}

TEST(PudManifestInvariantsTest, StoredGoalsUseTheirNodesRepresentatives) {
    sweep(&pud_manifest_harness<pud_manifest>::check_stored_goals_use_their_nodes_representatives);
}

TEST(PudManifestInvariantsTest, SubstitutionsMeetTheWatchersPreconditions) {
    sweep(&pud_manifest_harness<pud_manifest>::check_substitutions_meet_the_watchers_preconditions);
}

TEST(PudManifestInvariantsTest, FreshVariablesAreDenseFromTheParentsLvc) {
    sweep(&pud_manifest_harness<pud_manifest>::check_fresh_variables_are_dense_from_the_parents_lvc);
}

TEST(PudManifestInvariantsTest, EveryOpenGoalHasALiveQuery) {
    sweep(&pud_manifest_harness<pud_manifest>::check_every_open_goal_has_a_live_query);
}

TEST(PudManifestInvariantsTest, QueryHeadNodeIsAnAncestorOfEveryLeafHoldingIt) {
    sweep(&pud_manifest_harness<pud_manifest>::check_query_head_node_is_an_ancestor_of_every_leaf_holding_it);
}

TEST(PudManifestInvariantsTest, EveryCandidatePathIsARealRootFirstChain) {
    sweep(&pud_manifest_harness<pud_manifest>::check_every_candidate_path_is_a_real_root_first_chain);
}

TEST(PudManifestInvariantsTest, EveryCandidateIsAcceptableAtEveryLeafHoldingTheQuery) {
    sweep(&pud_manifest_harness<pud_manifest>::check_every_candidate_is_acceptable_at_every_leaf_holding_the_query);
}

TEST(PudManifestInvariantsTest, QueryHoldersMatchTheStoredDescents) {
    sweep(&pud_manifest_harness<pud_manifest>::check_query_holders_match_the_stored_descents);
}

TEST(PudManifestInvariantsTest, RandomRecursiveProgramsKeepEveryInvariant) {
    for (unsigned seed = 100; seed < 200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = std::make_unique<pud_manifest_harness<pud_manifest>>(seed, pud_harness_scope::every_open_leaf);
        load_datalog_axioms(*harness, generate_datalog(seed, true));
        harness->run_base_descent();
        harness->check_all();
        for (size_t unfold = 0; unfold < 25 && harness->most_open_goals() <= 64 && harness->advance_unfold_loop(); ++unfold)
            harness->check_all();
    }
}

TEST(PudManifestInvariantsTest, RandomFlatProgramsKeepEveryInvariantUntilNothingIsOpen) {
    for (unsigned seed = 300; seed < 360; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = std::make_unique<pud_manifest_harness<pud_manifest>>(seed, pud_harness_scope::every_open_leaf);
        load_datalog_axioms(*harness, generate_datalog(seed, false));
        harness->run_base_descent();
        harness->check_all();
        size_t unfolds = 0;
        while (unfolds < 5000 && harness->advance_unfold_loop()) {
            ++unfolds;
            if (unfolds % 10 == 0)
                harness->check_all();
        }
        EXPECT_TRUE(harness->manifest.open_leaves_.get().empty()) << "a flat program must unfold until nothing is open";
        harness->check_all();
    }
}
