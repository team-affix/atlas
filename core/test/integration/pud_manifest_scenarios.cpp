#include <algorithm>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_datalog_oracle.hpp"
#include "pud_manifest_harness.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudManifestScenarioIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::every_open_leaf};

    std::vector<std::vector<pud_node_id>> candidate_paths(pud_node_id leaf, pud_body_goal_id goal) {
        std::vector<std::vector<pud_node_id>> paths;
        for (pud_mhcs_head_id candidate : h.manifest.mhq_.candidates(h.query_of(leaf, goal).value()))
            paths.push_back(h.manifest.mhcs_.path(candidate));
        std::sort(paths.begin(), paths.end());
        return paths;
    }

    bool unfold_until_expanded(pud_node_id node, size_t max_unfolds) {
        for (size_t unfold = 0; unfold < max_unfolds && !h.expanded(node); ++unfold) {
            if (!h.advance_unfold_loop())
                break;
        }
        return h.expanded(node);
    }
};

TEST_F(PudManifestScenarioIntegrationTest, EmptyDatabaseUnfoldsNothing) {
    h.open_axioms();
    EXPECT_FALSE(h.advance_unfold_loop());
    EXPECT_THAT(h.open_leaves(), IsEmpty());
    EXPECT_THAT(h.proven_heads(), IsEmpty());
}

TEST_F(PudManifestScenarioIntegrationTest, DatabaseOfFactsLeavesNothingOpen) {
    h.add_colors();
    h.open_axioms();
    EXPECT_THAT(h.open_leaves(), IsEmpty());
    EXPECT_FALSE(h.advance_unfold_loop());
    EXPECT_EQ(h.proven_heads(), (std::set<const expr*>{
        h.functor("color", {h.atom("red")}), h.functor("color", {h.atom("blue")}), h.functor("warm", {h.atom("red")})}));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, RuleWithAnEmptyBodyIsAFact) {
    const pud_node_id fact = h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    h.axiom(h.functor("q", {h.var(0)}), {h.functor("p", {h.var(0)})}, 1);
    h.open_axioms();
    EXPECT_FALSE(h.open(fact));
    EXPECT_TRUE(h.live_leaf(fact));
    h.unfold_until_closed(100);
    EXPECT_TRUE(h.proven_heads().contains(h.functor("q", {h.atom("a")})));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, ChainWithAMissingBaseIsRefutedWhicheverRootRunsFirst) {
    for (bool dependent_first : {true, false}) {
        SCOPED_TRACE(dependent_first ? "dependent first" : "dependency first");
        pud_manifest_harness chain{0, pud_harness_scope::every_open_leaf};
        pud_node_id a = 0;
        pud_node_id b = 0;
        if (dependent_first) {
            a = chain.axiom(chain.atom("a"), {chain.atom("b")}, 0);
            b = chain.axiom(chain.atom("b"), {chain.atom("c")}, 0);
        } else {
            b = chain.axiom(chain.atom("b"), {chain.atom("c")}, 0);
            a = chain.axiom(chain.atom("a"), {chain.atom("b")}, 0);
        }
        chain.open_axioms();
        EXPECT_TRUE(chain.refuted(b));
        EXPECT_TRUE(chain.refuted(a));
        EXPECT_THAT(chain.open_leaves(), IsEmpty());
        chain.check_all();
    }
}

TEST_F(PudManifestScenarioIntegrationTest, AxiomAddedAfterTheRunIsACandidateForLaterQueries) {
    h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    h.axiom(h.functor("s", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id user = h.axiom(h.atom("q"), {h.functor("s", {h.atom("green")})}, 0);
    h.open_axioms();
    const pud_node_id late = h.axiom(h.functor("color", {h.atom("green")}), {}, 0);
    h.expand(user, 0);
    ASSERT_EQ(h.children_of(user).size(), 1u);
    const pud_node_id child = h.children_of(user).front();
    ASSERT_FALSE(h.refuted(child)) << "the late color(green) fact must be a candidate for the added goal";
    EXPECT_EQ(candidate_paths(child, 1), (std::vector<std::vector<pud_node_id>>{{late}}));
    h.open_axioms();
    h.unfold_until_closed(50);
    EXPECT_TRUE(h.proven_heads().contains(h.atom("q")));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, ForkedQueryNeverGainsAnAxiomAddedAfterItsParentOpened) {
    h.axiom(h.functor("warm", {h.atom("green")}), {}, 0);
    h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    const pud_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("warm", {h.var(0)}), h.functor("color", {h.var(0)})}, 1);
    h.open_axioms();
    h.axiom(h.functor("color", {h.atom("green")}), {}, 0);
    h.expand(rule, 0);
    ASSERT_EQ(h.children_of(rule).size(), 1u);
    EXPECT_TRUE(h.refuted(h.children_of(rule).front()));
    EXPECT_TRUE(h.refuted(rule));
}

TEST_F(PudManifestScenarioIntegrationTest, RepeatedHeadVariablesOnlyMatchEqualArguments) {
    h.axiom(h.functor("pair", {h.atom("a"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("pair", {h.atom("c"), h.atom("c")}), {}, 0);
    h.axiom(h.functor("same", {h.var(0)}), {h.functor("pair", {h.var(0), h.var(0)})}, 1);
    h.axiom(h.functor("twin", {h.var(0), h.var(0)}), {h.functor("pair", {h.var(0), h.var(0)})}, 1);
    const pud_node_id mismatched = h.axiom(h.atom("mismatched"), {h.functor("twin", {h.atom("a"), h.atom("b")})}, 0);
    h.axiom(h.atom("matched"), {h.functor("twin", {h.atom("c"), h.atom("c")})}, 0);
    h.open_axioms();
    h.unfold_until_closed(200);
    const std::set<const expr*> proven = h.proven_heads();
    EXPECT_TRUE(proven.contains(h.functor("same", {h.atom("c")})));
    EXPECT_FALSE(proven.contains(h.functor("same", {h.atom("a")})));
    EXPECT_TRUE(proven.contains(h.atom("matched")));
    EXPECT_TRUE(h.refuted(mismatched));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, NonGroundFactsAreUsedAsCandidates) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    h.axiom(h.functor("q", {h.var(0)}), {h.functor("eq", {h.var(0), h.atom("a")})}, 1);
    const pud_node_id unequal = h.axiom(h.atom("r"), {h.functor("eq", {h.atom("b"), h.atom("c")})}, 0);
    h.open_axioms();
    h.unfold_until_closed(100);
    EXPECT_TRUE(h.proven_heads().contains(h.functor("q", {h.atom("a")})));
    EXPECT_TRUE(h.refuted(unequal));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, SameNameWithDifferentArityNeverUnifies) {
    h.axiom(h.functor("f", {h.atom("a")}), {}, 0);
    const pud_node_id wrong_arity = h.axiom(h.atom("q"), {h.functor("f", {h.atom("a"), h.atom("b")})}, 0);
    h.axiom(h.atom("r"), {h.functor("f", {h.atom("a")})}, 0);
    h.open_axioms();
    h.unfold_until_closed(100);
    EXPECT_TRUE(h.refuted(wrong_arity));
    EXPECT_TRUE(h.proven_heads().contains(h.atom("r")));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, AtomsAndRepeatedGoalsInOneBody) {
    h.axiom(h.atom("s"), {}, 0);
    h.axiom(h.atom("q"), {h.atom("s"), h.atom("s")}, 0);
    h.open_axioms();
    h.unfold_until_closed(100);
    EXPECT_TRUE(h.proven_heads().contains(h.atom("q")));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, VeryDeepTermsUnifyAndUnfold) {
    h.axiom(h.functor("big", {h.peano(5000)}), {}, 0);
    h.axiom(h.functor("q", {h.var(0)}), {h.functor("big", {h.var(0)})}, 1);
    h.axiom(h.atom("r"), {h.functor("big", {h.peano(5000)})}, 0);
    const pud_node_id off_by_one = h.axiom(h.atom("t"), {h.functor("big", {h.peano(4999)})}, 0);
    h.open_axioms();
    h.unfold_until_closed(100);
    const std::set<const expr*> proven = h.proven_heads();
    EXPECT_TRUE(proven.contains(h.functor("q", {h.peano(5000)})));
    EXPECT_TRUE(proven.contains(h.atom("r")));
    EXPECT_TRUE(h.refuted(off_by_one));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, GoalsJoinedThroughEquatedVariables) {
    for (uint32_t random_seed = 0; random_seed < 8; ++random_seed) {
        SCOPED_TRACE("seed " + std::to_string(random_seed));
        pud_manifest_harness join{random_seed, pud_harness_scope::every_open_leaf};
        join.axiom(join.functor("eq", {join.atom("a"), join.atom("a")}), {}, 0);
        join.axiom(join.functor("eq", {join.atom("b"), join.atom("b")}), {}, 0);
        join.axiom(join.functor("pair", {join.atom("a"), join.atom("b")}), {}, 0);
        join.axiom(join.functor("pair", {join.atom("a"), join.atom("a")}), {}, 0);
        join.axiom(join.functor("q", {join.var(0), join.var(1)}),
            {join.functor("eq", {join.var(0), join.var(1)}), join.functor("pair", {join.var(0), join.var(1)})}, 2);
        join.open_axioms();
        join.unfold_until_closed(200);
        const std::set<const expr*> proven = join.proven_heads();
        EXPECT_TRUE(proven.contains(join.functor("q", {join.atom("a"), join.atom("a")})));
        EXPECT_FALSE(proven.contains(join.functor("q", {join.atom("a"), join.atom("b")})));
        EXPECT_FALSE(proven.contains(join.functor("q", {join.atom("b"), join.atom("b")})));
        join.check_all();
    }
}

TEST_F(PudManifestScenarioIntegrationTest, EveryUnfoldClosesItsLeafAndOpensExactlyTheChildrenWithGoals) {
    h.add_colors();
    h.axiom(h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.add_nat();
    h.open_axioms();
    for (int step = 0; step < 30; ++step) {
        const std::optional<pud_unfold> next = h.next_unfold();
        ASSERT_TRUE(next.has_value());
        const pud_unfold unfolded = next.value();
        EXPECT_FALSE(h.open(unfolded.leaf));
        EXPECT_FALSE(h.leaf(unfolded.leaf));
        EXPECT_EQ(h.manifest.call_sites_.get(unfolded.leaf), unfolded.goal);
        for (pud_node_id child : h.children_of(unfolded.leaf)) {
            if (h.refuted(child))
                continue;
            EXPECT_EQ(h.open(child), !h.open_goals(child).empty()) << "child " << child;
        }
    }
}

TEST_F(PudManifestScenarioIntegrationTest, LoopEndsExactlyWhenNothingIsOpen) {
    h.add_colors();
    h.axiom(h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.open_axioms();
    bool ended = false;
    for (int step = 0; step < 100 && !ended; ++step) {
        const bool anything_open = !h.open_leaves().empty();
        ended = !h.advance_unfold_loop();
        EXPECT_EQ(!ended, anything_open);
    }
    EXPECT_TRUE(ended);
    EXPECT_THAT(h.open_leaves(), IsEmpty());
}

TEST_F(PudManifestScenarioIntegrationTest, NewDriveContinuesOnceNewRootsAreRun) {
    h.add_colors();
    h.open_axioms();
    EXPECT_FALSE(h.advance_unfold_loop());
    h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.open_axioms();
    EXPECT_GT(h.unfold_until_closed(100), 0u);
    EXPECT_TRUE(h.proven_heads().contains(h.functor("q", {h.atom("blue")})));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, OneUnfoldMakesAChildForEveryCandidate) {
    for (int index = 0; index < 50; ++index)
        h.axiom(h.functor("f", {h.atom("k" + std::to_string(index))}), {}, 0);
    const pud_node_id rule = h.axiom(h.functor("g", {h.var(0)}), {h.functor("f", {h.var(0)})}, 1);
    h.open_axioms();
    h.expand(rule, 0);
    EXPECT_EQ(h.children_of(rule).size(), 50u);
    EXPECT_EQ(h.manifest.live_child_counts_.get(rule), 50u);
    EXPECT_EQ(h.proven_heads().size(), 100u);
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, RefutationCascadesIntoTheTreesThatDependedOnIt) {
    h.axiom(h.functor("base", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("check", {h.atom("a")}), {}, 0);
    const pud_node_id mid = h.axiom(h.functor("mid", {h.var(0)}),
        {h.functor("base", {h.var(0)}), h.functor("check", {h.var(0)})}, 1);
    const pud_node_id top = h.axiom(h.atom("top"), {h.functor("mid", {h.atom("c")})}, 0);
    h.open_axioms();
    EXPECT_FALSE(h.refuted(top)) << "mid is still a candidate before it is unfolded";
    h.unfold_until_closed(100);
    EXPECT_TRUE(h.refuted(mid));
    EXPECT_TRUE(h.refuted(top));
    EXPECT_THAT(h.open_leaves(), IsEmpty());
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, MutualRecursionWithoutABaseStaysLive) {
    const pud_node_id p = h.axiom(h.atom("p"), {h.atom("q")}, 0);
    const pud_node_id q = h.axiom(h.atom("q"), {h.atom("p")}, 0);
    h.open_axioms();
    for (int step = 0; step < 40; ++step) {
        ASSERT_TRUE(h.advance_unfold_loop());
        h.check_all();
    }
    EXPECT_FALSE(h.refuted(p));
    EXPECT_FALSE(h.refuted(q));
    EXPECT_FALSE(h.open_leaves().empty());
    EXPECT_THAT(h.proven_heads(), IsEmpty());
}

TEST_F(PudManifestScenarioIntegrationTest, SelfRecursionWithoutABaseGrowsOneNodePerUnfold) {
    const pud_node_id p = h.axiom(h.atom("p"), {h.atom("p")}, 0);
    h.open_axioms();
    for (int step = 1; step <= 40; ++step) {
        ASSERT_TRUE(h.advance_unfold_loop());
        EXPECT_EQ(h.all_nodes().size(), static_cast<size_t>(step + 1));
    }
    EXPECT_FALSE(h.refuted(p));
    EXPECT_EQ(h.open_leaves().size(), 1u);
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, GrownRuleTreeOffersDeeperCandidates) {
    const pud_node_id rule = h.add_nat();
    h.open_axioms();
    h.grow_along_open_goals(rule, 3);
    const pud_node_id user = h.axiom(h.atom("q"), {h.functor("nat", {h.peano(3)})}, 0);
    h.open_axioms();
    size_t deepest = 0;
    for (const std::vector<pud_node_id>& path : candidate_paths(user, 0))
        deepest = std::max(deepest, path.size());
    EXPECT_GT(deepest, 1u);
    h.unfold_until_closed(100);
    EXPECT_TRUE(h.proven_heads().contains(h.atom("q")));
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, SelfReferentialLemmasOnlyProveNumerals) {
    h.add_nat();
    h.open_axioms();
    h.unfold_until_closed(60);
    const std::set<const expr*> proven = h.proven_heads();
    EXPECT_GE(proven.size(), 3u);
    for (const expr* head : proven) {
        const expr* number = std::get<expr::functor>(head->content).args.front();
        while (number != h.atom("z")) {
            const expr::functor* successor = std::get_if<expr::functor>(&number->content);
            ASSERT_TRUE(successor != nullptr && successor->args.size() == 1) << "not a numeral";
            number = successor->args.front();
        }
    }
    h.check_all();
}

TEST_F(PudManifestScenarioIntegrationTest, NoQueryOutlivesAnExhaustedDatabase) {
    for (unsigned seed = 0; seed < 30; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        pud_manifest_harness database{seed, pud_harness_scope::every_open_leaf};
        load_datalog_axioms(database, generate_datalog(seed, false));
        database.open_axioms();
        std::set<pud_mhq_head_id> seen;
        do {
            for (pud_node_id leaf : database.open_leaves()) {
                for (const pud_body_goal& goal : database.open_goals(leaf))
                    seen.insert(database.query_of(leaf, goal.id).value());
            }
        } while (database.advance_unfold_loop());
        ASSERT_THAT(database.open_leaves(), IsEmpty());
        for (pud_mhq_head_id head : seen)
            EXPECT_FALSE(database.alive(head)) << "query " << head;
    }
}

TEST_F(PudManifestScenarioIntegrationTest, SameProgramAndSeedUnfoldIdentically) {
    pud_manifest_harness first{7, pud_harness_scope::every_open_leaf};
    pud_manifest_harness second{7, pud_harness_scope::every_open_leaf};
    for (pud_manifest_harness* each : {&first, &second}) {
        each->add_colors();
        each->add_nat();
        each->add_graph();
        each->open_axioms();
    }
    for (int step = 0; step < 40; ++step) {
        const std::optional<pud_unfold> first_next = first.next_unfold();
        ASSERT_EQ(first_next, second.next_unfold());
        if (!first_next.has_value())
            break;
    }
    EXPECT_EQ(first.all_nodes(), second.all_nodes());
}

TEST_F(PudManifestScenarioIntegrationTest, InterleavedManifestsDoNotInterfere) {
    pud_manifest_harness first{1, pud_harness_scope::every_open_leaf};
    pud_manifest_harness second{2, pud_harness_scope::every_open_leaf};
    load_datalog_axioms(first, generate_datalog(11, false));
    load_datalog_axioms(second, generate_datalog(12, false));
    first.open_axioms();
    second.open_axioms();
    bool first_running = true;
    bool second_running = true;
    for (int step = 0; step < 20000 && (first_running || second_running); ++step) {
        if (first_running)
            first_running = first.advance_unfold_loop();
        if (second_running)
            second_running = second.advance_unfold_loop();
    }
    EXPECT_EQ(first.proven_heads(), datalog_model_exprs(first, generate_datalog(11, false)));
    EXPECT_EQ(second.proven_heads(), datalog_model_exprs(second, generate_datalog(12, false)));
}
