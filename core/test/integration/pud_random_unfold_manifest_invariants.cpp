#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "pud_datalog_oracle.hpp"
#include "pud_manifest_harness.hpp"

namespace {

using check_t = void (pud_random_unfold_manifest_harness::*)();
using build_t = void (*)(pud_random_unfold_manifest_harness&);

void build_colors(pud_random_unfold_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
}

void build_nat(pud_random_unfold_manifest_harness& h) {
    h.add_nat();
}

void build_append(pud_random_unfold_manifest_harness& h) {
    h.add_append();
}

void build_member(pud_random_unfold_manifest_harness& h) {
    h.add_member();
}

void build_peano_add(pud_random_unfold_manifest_harness& h) {
    h.add_peano_add();
}

void build_graph(pud_random_unfold_manifest_harness& h) {
    h.add_graph();
}

// p0 :- p1, p1 :- p2, ..., p19 :- p20, and the fact p20.
void build_chain(pud_random_unfold_manifest_harness& h) {
    for (int link = 0; link < 20; ++link)
        h.axiom(h.atom("p" + std::to_string(link)), {h.atom("p" + std::to_string(link + 1))}, 0);
    h.axiom(h.atom("p20"), {}, 0);
}

// Thirty facts f(k_i) and a rule over them, so one unfold makes thirty children.
void build_fan_out(pud_random_unfold_manifest_harness& h) {
    for (int index = 0; index < 30; ++index)
        h.axiom(h.functor("f", {h.atom("k" + std::to_string(index))}), {}, 0);
    h.axiom(h.functor("g", {h.var(0)}), {h.functor("f", {h.var(0)})}, 1);
}

// A join whose second goal only matches once the first has equated both variables.
void build_join(pud_random_unfold_manifest_harness& h) {
    h.axiom(h.functor("eq", {h.atom("a"), h.atom("a")}), {}, 0);
    h.axiom(h.functor("eq", {h.atom("b"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("a")}), {}, 0);
    h.axiom(h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("pair", {h.var(0), h.var(1)})}, 2);
}

// p :- q and q :- p with no base: both stay live forever.
void build_mutual_recursion(pud_random_unfold_manifest_harness& h) {
    h.axiom(h.atom("p"), {h.atom("q")}, 0);
    h.axiom(h.atom("q"), {h.atom("p")}, 0);
}

// A rule whose goal never matches, and rules that depend on it.
void build_refuted_chain(pud_random_unfold_manifest_harness& h) {
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

// Every program runs under several random seeds, so the choosers take different paths; the check
// runs after the base descent and after every unfold.
void sweep(check_t check) {
    for (uint32_t random_seed = 0; random_seed < 4; ++random_seed) {
        for (const program& each : corpus()) {
            SCOPED_TRACE(each.name + " seed " + std::to_string(random_seed));
            auto harness = std::make_unique<pud_random_unfold_manifest_harness>(random_seed);
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

}  // namespace

TEST(PudRandomUnfoldManifestInvariantsTest, LiveLeavesAreExactlyTheStoredDescents) {
    sweep(&pud_random_unfold_manifest_harness::check_live_leaves_are_exactly_the_stored_descents);
}

TEST(PudRandomUnfoldManifestInvariantsTest, OpenLeavesAreExactlyTheLiveLeavesWithOpenGoals) {
    sweep(&pud_random_unfold_manifest_harness::check_open_leaves_are_exactly_the_live_leaves_with_open_goals);
}

TEST(PudRandomUnfoldManifestInvariantsTest, LeafFlagMatchesHavingChildren) {
    sweep(&pud_random_unfold_manifest_harness::check_leaf_flag_matches_having_children);
}

TEST(PudRandomUnfoldManifestInvariantsTest, ParentsAndChildrenAgree) {
    sweep(&pud_random_unfold_manifest_harness::check_parents_and_children_agree);
}

TEST(PudRandomUnfoldManifestInvariantsTest, LiveChildCountsMatchUnrefutedChildren) {
    sweep(&pud_random_unfold_manifest_harness::check_live_child_counts_match_unrefuted_children);
}

TEST(PudRandomUnfoldManifestInvariantsTest, RefutationIsClosedUpward) {
    sweep(&pud_random_unfold_manifest_harness::check_refutation_is_closed_upward);
}

TEST(PudRandomUnfoldManifestInvariantsTest, RefutationIsClosedDownward) {
    sweep(&pud_random_unfold_manifest_harness::check_refutation_is_closed_downward);
}

TEST(PudRandomUnfoldManifestInvariantsTest, LeafLvcIsTheSumOfVarCountsOnItsPath) {
    sweep(&pud_random_unfold_manifest_harness::check_leaf_lvc_is_the_sum_of_var_counts_on_its_path);
}

TEST(PudRandomUnfoldManifestInvariantsTest, LeafBindingMatchesItsReplayedSubstitutions) {
    sweep(&pud_random_unfold_manifest_harness::check_leaf_binding_matches_its_replayed_substitutions);
}

TEST(PudRandomUnfoldManifestInvariantsTest, StoredGoalsAreInTheirNodesRepresentatives) {
    sweep(&pud_random_unfold_manifest_harness::check_stored_goals_are_in_their_nodes_representatives);
}

TEST(PudRandomUnfoldManifestInvariantsTest, SubstitutionsMeetTheWatchersPreconditions) {
    sweep(&pud_random_unfold_manifest_harness::check_substitutions_meet_the_watchers_preconditions);
}

TEST(PudRandomUnfoldManifestInvariantsTest, FreshVariablesAreDenseFromTheParentsLvc) {
    sweep(&pud_random_unfold_manifest_harness::check_fresh_variables_are_dense_from_the_parents_lvc);
}

TEST(PudRandomUnfoldManifestInvariantsTest, EveryOpenGoalHasALiveQuery) {
    sweep(&pud_random_unfold_manifest_harness::check_every_open_goal_has_a_live_query);
}

TEST(PudRandomUnfoldManifestInvariantsTest, QueryHeadNodeIsAnAncestorOfEveryLeafHoldingIt) {
    sweep(&pud_random_unfold_manifest_harness::check_query_head_node_is_an_ancestor_of_every_leaf_holding_it);
}

TEST(PudRandomUnfoldManifestInvariantsTest, EveryCandidatePathIsARealRootFirstChain) {
    sweep(&pud_random_unfold_manifest_harness::check_every_candidate_path_is_a_real_root_first_chain);
}

TEST(PudRandomUnfoldManifestInvariantsTest, EveryCandidateIsAcceptableAtEveryLeafHoldingTheQuery) {
    sweep(&pud_random_unfold_manifest_harness::check_every_candidate_is_acceptable_at_every_leaf_holding_the_query);
}

TEST(PudRandomUnfoldManifestInvariantsTest, RandomRecursiveProgramsKeepEveryInvariant) {
    for (unsigned seed = 100; seed < 200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = std::make_unique<pud_random_unfold_manifest_harness>(seed);
        load_datalog_axioms(*harness, generate_datalog(seed, true));
        harness->run_base_descent();
        harness->check_all();
        for (size_t unfold = 0; unfold < 25 && harness->most_open_goals() <= 64 && harness->advance_unfold_loop(); ++unfold)
            harness->check_all();
    }
}

TEST(PudRandomUnfoldManifestInvariantsTest, RandomFlatProgramsKeepEveryInvariantUntilNothingIsOpen) {
    for (unsigned seed = 300; seed < 360; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = std::make_unique<pud_random_unfold_manifest_harness>(seed);
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
