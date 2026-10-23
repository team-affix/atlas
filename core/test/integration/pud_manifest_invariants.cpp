#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "pud_datalog_oracle.hpp"
#include "pud_manifest_harness.hpp"

namespace {

using verify_t = void (pud_manifest_harness::*)();
using build_database_t = void (*)(pud_manifest_harness&);
using build_conjecture_t = pud_node_id (*)(pud_manifest_harness&);

void build_colors(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
}

void build_nat(pud_manifest_harness& h) {
    h.add_nat();
}

void build_append(pud_manifest_harness& h) {
    h.add_append();
}

void build_member(pud_manifest_harness& h) {
    h.add_member();
}

void build_peano_add(pud_manifest_harness& h) {
    h.add_peano_add();
}

void build_graph(pud_manifest_harness& h) {
    h.add_graph();
}

void build_chain(pud_manifest_harness& h) {
    for (int link = 0; link < 20; ++link)
        h.axiom(h.atom("p" + std::to_string(link)), {h.atom("p" + std::to_string(link + 1))}, 0);
    h.axiom(h.atom("p20"), {}, 0);
}

void build_fan_out(pud_manifest_harness& h) {
    for (int index = 0; index < 30; ++index)
        h.axiom(h.functor("f", {h.atom("k" + std::to_string(index))}), {}, 0);
    h.axiom(h.functor("g", {h.var(0)}), {h.functor("f", {h.var(0)})}, 1);
}

void build_join(pud_manifest_harness& h) {
    h.axiom(h.functor("eq", {h.atom("a"), h.atom("a")}), {}, 0);
    h.axiom(h.functor("eq", {h.atom("b"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("a")}), {}, 0);
    h.axiom(h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("pair", {h.var(0), h.var(1)})}, 2);
}

void build_mutual_recursion(pud_manifest_harness& h) {
    h.axiom(h.atom("p"), {h.atom("q")}, 0);
    h.axiom(h.atom("q"), {h.atom("p")}, 0);
}

void build_refuted_chain(pud_manifest_harness& h) {
    h.axiom(h.atom("a"), {h.atom("b")}, 0);
    h.axiom(h.atom("b"), {h.atom("c")}, 0);
    h.axiom(h.functor("mid", {h.var(0)}), {h.functor("base", {h.var(0)}), h.functor("check", {h.var(0)})}, 1);
    h.axiom(h.functor("base", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("check", {h.atom("a")}), {}, 0);
    h.axiom(h.atom("top"), {h.functor("mid", {h.atom("c")})}, 0);
}

pud_node_id build_colors_conjunction(pud_manifest_harness& h) {
    h.add_colors();
    return h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
}

pud_node_id build_colors_independent(pud_manifest_harness& h) {
    h.add_colors();
    return h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
}

pud_node_id build_nat_enumeration(pud_manifest_harness& h) {
    h.add_nat();
    return h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
}

pud_node_id build_nat_ground(pud_manifest_harness& h) {
    h.add_nat();
    return h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(4)})}, 0);
}

pud_node_id build_append_split(pud_manifest_harness& h) {
    h.add_append();
    return h.start_conjecture(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("a"), h.atom("b"), h.atom("c")})})},
        2);
}

pud_node_id build_member_conjecture(pud_manifest_harness& h) {
    h.add_member();
    return h.start_conjecture(
        h.functor("q", {h.var(0)}),
        {h.functor("mem", {h.var(0), h.list({h.atom("a"), h.atom("b"), h.atom("c")})})},
        1);
}

pud_node_id build_peano_add_conjecture(pud_manifest_harness& h) {
    h.add_peano_add();
    return h.start_conjecture(h.functor("q", {h.var(0), h.var(1)}), {h.functor("add", {h.var(0), h.var(1), h.peano(3)})}, 2);
}

pud_node_id build_graph_conjecture(pud_manifest_harness& h) {
    h.add_graph();
    return h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("anc", {h.atom("a"), h.var(0)})}, 1);
}

pud_node_id build_cross_goal_merge(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    return h.start_conjecture(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
}

pud_node_id build_self_candidate(pud_manifest_harness& h) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    return h.start_conjecture(h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
}

pud_node_id build_grown_lemma(pud_manifest_harness& h) {
    const pud_node_id rule = h.add_nat();
    h.open_axioms();
    h.grow_along_open_goals(rule, 3);
    return h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
}

pud_node_id build_occurs_check(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    return h.start_conjecture(h.atom("q"), {h.functor("eq", {h.var(0), h.functor("f", {h.var(0)})}), h.functor("color", {h.var(0)})}, 1);
}

pud_node_id build_query_as_lemma(pud_manifest_harness& h) {
    h.add_colors();
    const pud_node_id first = h.start_conjecture(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.next_solution(first, 2);
    return h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
}

pud_node_id build_shared_deep(pud_manifest_harness& h) {
    h.add_colors();
    h.axiom(h.atom("a1"), {}, 0);
    h.axiom(h.atom("a2"), {}, 0);
    h.axiom(h.atom("a3"), {}, 0);
    return h.start_conjecture(
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

// A database alone, expanded at every open leaf, or a database with a conjecture the solver solves.
struct scenario {
    std::string name;
    build_database_t build_database;
    build_conjecture_t build_conjecture;
    size_t steps;
};

const std::vector<scenario>& corpus() {
    static const std::vector<scenario> scenarios{
        {"colors", &build_colors, nullptr, 30},
        {"nat", &build_nat, nullptr, 30},
        {"append", &build_append, nullptr, 30},
        {"member", &build_member, nullptr, 30},
        {"peano_add", &build_peano_add, nullptr, 30},
        {"graph", &build_graph, nullptr, 30},
        {"chain", &build_chain, nullptr, 40},
        {"fan_out", &build_fan_out, nullptr, 40},
        {"join", &build_join, nullptr, 30},
        {"mutual_recursion", &build_mutual_recursion, nullptr, 30},
        {"refuted_chain", &build_refuted_chain, nullptr, 30},
        {"colors_conjunction", nullptr, &build_colors_conjunction, 60},
        {"colors_independent", nullptr, &build_colors_independent, 60},
        {"nat_enumeration", nullptr, &build_nat_enumeration, 120},
        {"nat_ground", nullptr, &build_nat_ground, 60},
        {"append_split", nullptr, &build_append_split, 90},
        {"member", nullptr, &build_member_conjecture, 90},
        {"peano_add", nullptr, &build_peano_add_conjecture, 90},
        {"graph", nullptr, &build_graph_conjecture, 90},
        {"cross_goal_merge", nullptr, &build_cross_goal_merge, 90},
        {"self_candidate", nullptr, &build_self_candidate, 90},
        {"grown_lemma", nullptr, &build_grown_lemma, 90},
        {"occurs_check", nullptr, &build_occurs_check, 30},
        {"query_as_lemma", nullptr, &build_query_as_lemma, 60},
        {"shared_deep", nullptr, &build_shared_deep, 60},
        {"random_nonrecursive", nullptr, &build_random_nonrecursive, 120},
        {"random_recursive", nullptr, &build_random_recursive, 120},
    };
    return scenarios;
}

void sweep(verify_t check) {
    for (uint32_t random_seed = 0; random_seed < 4; ++random_seed) {
        for (const scenario& each : corpus()) {
            SCOPED_TRACE(each.name + " seed " + std::to_string(random_seed));
            const pud_harness_scope scope =
                each.build_database ? pud_harness_scope::every_open_leaf : pud_harness_scope::conjecture_leaves;
            auto harness = std::make_unique<pud_manifest_harness>(random_seed, scope);
            if (each.build_database)
                each.build_database(*harness);
            else
                each.build_conjecture(*harness);
            harness->open_axioms();
            ((*harness).*check)();
            for (size_t step = 0; step < each.steps && harness->advance_unfold_loop(); ++step) {
                SCOPED_TRACE("step " + std::to_string(step));
                ((*harness).*check)();
                if (::testing::Test::HasFailure())
                    return;
            }
        }
    }
}

}

TEST(PudManifestInvariantsIntegrationTest, ParentsAndChildrenAgree) {
    sweep(&pud_manifest_harness::verify_parents_and_children_agree);
}

TEST(PudManifestInvariantsIntegrationTest, LiveChildCountsMatchUnrefutedChildren) {
    sweep(&pud_manifest_harness::verify_live_child_counts_match_unrefuted_children);
}

TEST(PudManifestInvariantsIntegrationTest, GoalIdsAscendAndExtendTheParents) {
    sweep(&pud_manifest_harness::verify_goal_ids_ascend_and_extend_the_parents);
}

TEST(PudManifestInvariantsIntegrationTest, LeafBindingMatchesItsReplayedSubstitutions) {
    sweep(&pud_manifest_harness::verify_leaf_binding_matches_its_replayed_substitutions);
}

TEST(PudManifestInvariantsIntegrationTest, StoredGoalsUseTheirNodesRepresentatives) {
    sweep(&pud_manifest_harness::verify_stored_goals_use_their_nodes_representatives);
}

TEST(PudManifestInvariantsIntegrationTest, SubstitutionsMeetTheWatchersPreconditions) {
    sweep(&pud_manifest_harness::verify_substitutions_meet_the_watchers_preconditions);
}

TEST(PudManifestInvariantsIntegrationTest, FreshVariablesAreDenseFromTheParentsLvc) {
    sweep(&pud_manifest_harness::verify_fresh_variables_are_dense_from_the_parents_lvc);
}

TEST(PudManifestInvariantsIntegrationTest, EveryOpenGoalHasAQueryItsLeafHolds) {
    sweep(&pud_manifest_harness::verify_every_open_goal_has_a_query_its_leaf_holds);
}

TEST(PudManifestInvariantsIntegrationTest, EveryCandidatePathIsARealRootFirstChain) {
    sweep(&pud_manifest_harness::verify_every_candidate_path_is_a_real_root_first_chain);
}

TEST(PudManifestInvariantsIntegrationTest, EveryCandidateIsAcceptableAtEveryLeafHoldingTheQuery) {
    sweep(&pud_manifest_harness::verify_every_candidate_is_acceptable_at_every_leaf_holding_the_query);
}

TEST(PudManifestInvariantsIntegrationTest, RandomRecursiveDatabasesKeepEveryInvariant) {
    for (unsigned seed = 100; seed < 200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = std::make_unique<pud_manifest_harness>(seed, pud_harness_scope::every_open_leaf);
        load_datalog_axioms(*harness, generate_datalog(seed, true));
        harness->open_axioms();
        harness->verify_all();
        for (size_t unfold = 0; unfold < 25 && harness->most_open_goals() <= 64 && harness->advance_unfold_loop(); ++unfold)
            harness->verify_all();
    }
}

TEST(PudManifestInvariantsIntegrationTest, RandomFlatDatabasesKeepEveryInvariantUntilNothingIsOpen) {
    for (unsigned seed = 300; seed < 360; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = std::make_unique<pud_manifest_harness>(seed, pud_harness_scope::every_open_leaf);
        load_datalog_axioms(*harness, generate_datalog(seed, false));
        harness->open_axioms();
        harness->verify_all();
        size_t unfolds = 0;
        while (unfolds < 5000 && harness->advance_unfold_loop()) {
            ++unfolds;
            if (unfolds % 10 == 0)
                harness->verify_all();
        }
        EXPECT_TRUE(harness->database_frontier.empty()) << "a flat program must unfold until nothing is open";
        harness->verify_all();
    }
}

TEST(PudManifestInvariantsIntegrationTest, RandomConjecturesKeepEveryInvariant) {
    for (unsigned seed = 100; seed < 200; ++seed) {
        for (bool recursive : {false, true}) {
            SCOPED_TRACE("seed " + std::to_string(seed) + (recursive ? " recursive" : " flat"));
            auto harness = std::make_unique<pud_manifest_harness>(seed, pud_harness_scope::conjecture_leaves);
            const pud_node_id query = load_datalog(*harness, generate_datalog(seed, recursive));
            std::vector<const expr*> answers;
            harness->verify_all();
            for (size_t step = 0; step < 40 && harness->most_open_goals() <= 64 && harness->step(query, answers); ++step) {
                harness->verify_all();
                if (::testing::Test::HasFailure())
                    return;
            }
        }
    }
}

TEST(PudManifestInvariantsIntegrationTest, DeepRecursionStaysConsistentAndEnumeratesInOrder) {
    auto harness = std::make_unique<pud_manifest_harness>(0, pud_harness_scope::conjecture_leaves);
    const pud_node_id query = build_nat_enumeration(*harness);
    std::vector<const expr*> answers;
    for (size_t step = 0; step < 150; ++step) {
        ASSERT_TRUE(harness->step(query, answers));
        if (step % 5 != 0)
            continue;
        harness->verify_all();
        if (::testing::Test::HasFailure())
            return;
    }
    harness->verify_all();
    EXPECT_GE(answers.size(), 10u);
    for (size_t index = 0; index < answers.size(); ++index)
        EXPECT_EQ(answers.at(index), harness->functor("q", {harness->peano(static_cast<uint32_t>(index))}));
}
