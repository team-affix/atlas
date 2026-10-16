#include <algorithm>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudManifestIntegrationTest : public ::testing::Test {
    pud_manifest manifest;
    test_functors functors;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), args);
    }

    const expr* atom(const char* name) { return functor(name, {}); }

    const expr* list(std::vector<const expr*> items) {
        const expr* tail = atom("nil");
        for (auto item = items.rbegin(); item != items.rend(); ++item)
            tail = functor("cons", {*item, tail});
        return tail;
    }

    const expr* peano(uint32_t count) {
        return peano_on(count, "z");
    }

    const expr* peano_on(uint32_t count, const char* base) {
        const expr* number = atom(base);
        for (uint32_t step = 0; step < count; ++step)
            number = functor("s", {number});
        return number;
    }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        return manifest.axiom_initializer_.initialize_axiom(rule(head, std::move(body), var_count));
    }

    pud_node_id start_query(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        const pud_node_id query_root = axiom(head, std::move(body), var_count);
        manifest.solver_.start(query_root);
        return query_root;
    }

    const expr* answer(pud_node_id query_root, pud_node_id solution) {
        pud_transient_ubs_state transient_ubs_state{
            manifest.node_descents_.get(solution).binding().ubs_state()};
        auto bind_map = transient_ubs_state.make_bind_map(manifest.expr_pool_, manifest.expr_pool_);
        auto normalizer = manifest.normalizer_factory_.make(bind_map);
        return normalizer.normalize(framed_expr{manifest.node_heads_.get(query_root), 0});
    }

    std::vector<const expr*> enumerate(pud_node_id query_root, size_t max_answers, size_t max_expansions) {
        std::vector<const expr*> answers;
        while (answers.size() < max_answers) {
            const std::optional<pud_node_id> solution = manifest.solver_.solve(query_root, max_expansions);
            if (!solution.has_value())
                break;
            answers.push_back(answer(query_root, solution.value()));
            manifest.node_refuter_.refute(solution.value());
        }
        return answers;
    }

    bool refuted(pud_node_id node) {
        return manifest.refuted_node_checker_.check_refuted(node);
    }

    void add_colors() {
        axiom(functor("color", {atom("red")}), {}, 0);
        axiom(functor("color", {atom("blue")}), {}, 0);
        axiom(functor("warm", {atom("red")}), {}, 0);
    }

    void add_nat() {
        axiom(functor("nat", {atom("z")}), {}, 0);
        axiom(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    }

    void add_append() {
        axiom(functor("app", {atom("nil"), var(0), var(0)}), {}, 1);
        axiom(
            functor("app", {functor("cons", {var(0), var(1)}), var(2), functor("cons", {var(0), var(3)})}),
            {functor("app", {var(1), var(2), var(3)})},
            4);
    }

    void add_member() {
        axiom(functor("mem", {var(0), functor("cons", {var(0), var(1)})}), {}, 2);
        axiom(
            functor("mem", {var(0), functor("cons", {var(1), var(2)})}),
            {functor("mem", {var(0), var(2)})},
            3);
    }

    void add_peano_add() {
        axiom(functor("add", {atom("z"), var(0), var(0)}), {}, 1);
        axiom(
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3);
    }
};

// ---------- facts ----------

TEST_F(PudManifestIntegrationTest, GroundQueryAgainstAFactIsSolved) {
    add_colors();
    const pud_node_id query = start_query(atom("q"), {functor("color", {atom("red")})}, 0);
    EXPECT_THAT(enumerate(query, 10, 50), ElementsAre(atom("q")));
}

TEST_F(PudManifestIntegrationTest, QueryVariableIsBoundByEachMatchingFact) {
    add_colors();
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_THAT(
        enumerate(query, 10, 50),
        UnorderedElementsAre(functor("q", {atom("red")}), functor("q", {atom("blue")})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, QueryWithNoMatchingAxiomIsRefutedAtStart) {
    add_colors();
    const pud_node_id query = start_query(atom("q"), {functor("color", {atom("green")})}, 0);
    EXPECT_TRUE(refuted(query));
    EXPECT_FALSE(manifest.solver_.solve(query, 50).has_value());
}

TEST_F(PudManifestIntegrationTest, QueryWithNoBodyGoalsIsItsOwnSolution) {
    const pud_node_id query = start_query(atom("q"), {}, 0);
    EXPECT_EQ(manifest.solver_.solve(query, 0), query);
}

TEST_F(PudManifestIntegrationTest, ConjunctionKeepsOnlyBindingsThatSatisfyEveryGoal) {
    add_colors();
    const pud_node_id query = start_query(
        functor("q", {var(0)}),
        {functor("color", {var(0)}), functor("warm", {var(0)})},
        1);
    EXPECT_THAT(enumerate(query, 10, 50), ElementsAre(functor("q", {atom("red")})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, ConjunctionWithNoCommonBindingIsRefuted) {
    add_colors();
    axiom(functor("cold", {atom("blue")}), {}, 0);
    const pud_node_id query = start_query(
        atom("q"),
        {functor("warm", {var(0)}), functor("cold", {var(0)})},
        1);
    EXPECT_THAT(enumerate(query, 10, 50), IsEmpty());
    EXPECT_TRUE(refuted(query));
}

// ---------- recursion ----------

TEST_F(PudManifestIntegrationTest, NaturalNumbersAreEnumeratedInOrder) {
    add_nat();
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    EXPECT_THAT(
        enumerate(query, 5, 100),
        ElementsAre(
            functor("q", {peano(0)}),
            functor("q", {peano(1)}),
            functor("q", {peano(2)}),
            functor("q", {peano(3)}),
            functor("q", {peano(4)})));
    EXPECT_FALSE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, GroundNaturalNumberIsChecked) {
    add_nat();
    const pud_node_id query = start_query(atom("q"), {functor("nat", {peano(3)})}, 0);
    EXPECT_THAT(enumerate(query, 10, 100), ElementsAre(atom("q")));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, NonNaturalIsRefutedAfterUnfolding) {
    add_nat();
    const pud_node_id query = start_query(atom("q"), {functor("nat", {functor("s", {atom("bad")})})}, 0);
    EXPECT_THAT(enumerate(query, 10, 100), IsEmpty());
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, AppendBuildsTheConcatenation) {
    add_append();
    const pud_node_id query = start_query(
        functor("q", {var(0)}),
        {functor("app", {list({atom("a")}), list({atom("b"), atom("c")}), var(0)})},
        1);
    EXPECT_THAT(enumerate(query, 10, 100), ElementsAre(functor("q", {list({atom("a"), atom("b"), atom("c")})})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, AppendSplitsAListEveryWay) {
    add_append();
    const pud_node_id query = start_query(
        functor("q", {var(0), var(1)}),
        {functor("app", {var(0), var(1), list({atom("a"), atom("b")})})},
        2);
    EXPECT_THAT(
        enumerate(query, 10, 100),
        UnorderedElementsAre(
            functor("q", {list({}), list({atom("a"), atom("b")})}),
            functor("q", {list({atom("a")}), list({atom("b")})}),
            functor("q", {list({atom("a"), atom("b")}), list({})})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, MemberFindsEveryElementInOrder) {
    add_member();
    const pud_node_id query = start_query(
        functor("q", {var(0)}),
        {functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})})},
        1);
    EXPECT_THAT(
        enumerate(query, 10, 100),
        ElementsAre(functor("q", {atom("a")}), functor("q", {atom("b")}), functor("q", {atom("c")})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, PeanoAdditionIsInverted) {
    add_peano_add();
    const pud_node_id query = start_query(
        functor("q", {var(0), var(1)}),
        {functor("add", {var(0), var(1), peano(2)})},
        2);
    EXPECT_THAT(
        enumerate(query, 10, 100),
        UnorderedElementsAre(
            functor("q", {peano(0), peano(2)}),
            functor("q", {peano(1), peano(1)}),
            functor("q", {peano(2), peano(0)})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, TransitiveClosureTerminatesOnAFiniteGraph) {
    axiom(functor("parent", {atom("a"), atom("b")}), {}, 0);
    axiom(functor("parent", {atom("b"), atom("c")}), {}, 0);
    axiom(functor("anc", {var(0), var(1)}), {functor("parent", {var(0), var(1)})}, 2);
    axiom(
        functor("anc", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)}), functor("anc", {var(2), var(1)})},
        3);
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("anc", {atom("a"), var(0)})}, 1);
    EXPECT_THAT(
        enumerate(query, 10, 200),
        UnorderedElementsAre(functor("q", {atom("b")}), functor("q", {atom("c")})));
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestIntegrationTest, SharedVariablesAcrossGoalsAreRespected) {
    add_append();
    add_member();
    const pud_node_id query = start_query(
        functor("q", {var(0)}),
        {
            functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})}),
            functor("app", {list({atom("x")}), list({var(0)}), list({atom("x"), atom("b")})}),
        },
        1);
    EXPECT_THAT(enumerate(query, 10, 200), ElementsAre(functor("q", {atom("b")})));
    EXPECT_TRUE(refuted(query));
}

// ---------- budget and tree state ----------

TEST_F(PudManifestIntegrationTest, ExhaustedBudgetLeavesTheQueryLive) {
    add_nat();
    const pud_node_id query = start_query(atom("q"), {functor("nat", {peano(5)})}, 0);
    EXPECT_FALSE(manifest.solver_.solve(query, 2).has_value());
    EXPECT_FALSE(refuted(query));
    EXPECT_TRUE(manifest.solver_.solve(query, 100).has_value());
}

TEST_F(PudManifestIntegrationTest, RefutingOneSolutionKeepsTheOthers) {
    add_colors();
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("color", {var(0)})}, 1);
    const std::optional<pud_node_id> first = manifest.solver_.solve(query, 50);
    ASSERT_TRUE(first.has_value());
    manifest.node_refuter_.refute(first.value());
    EXPECT_TRUE(refuted(first.value()));
    EXPECT_FALSE(refuted(query));
    const std::optional<pud_node_id> second = manifest.solver_.solve(query, 50);
    ASSERT_TRUE(second.has_value());
    EXPECT_NE(second.value(), first.value());
}

TEST_F(PudManifestIntegrationTest, TwoQueriesShareTheDatabase) {
    add_colors();
    const pud_node_id warm_query = start_query(functor("w", {var(0)}), {functor("warm", {var(0)})}, 1);
    const pud_node_id color_query = start_query(functor("c", {var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_THAT(enumerate(warm_query, 10, 50), ElementsAre(functor("w", {atom("red")})));
    EXPECT_THAT(
        enumerate(color_query, 10, 50),
        UnorderedElementsAre(functor("c", {atom("red")}), functor("c", {atom("blue")})));
}

TEST_F(PudManifestIntegrationTest, SolutionLeafHasNoOpenGoals) {
    add_nat();
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const std::optional<pud_node_id> solution = manifest.solver_.solve(query, 50);
    ASSERT_TRUE(solution.has_value());
    EXPECT_TRUE(manifest.node_descents_.get(solution.value()).body_goal_expr().body_goal_exprs().empty());
    EXPECT_TRUE(manifest.leaf_nodes_.check_leaf(solution.value()));
}

TEST_F(PudManifestIntegrationTest, ManyNaturalNumbersStayCorrect) {
    add_nat();
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const std::vector<const expr*> answers = enumerate(query, 12, 500);
    ASSERT_EQ(answers.size(), 12u);
    for (uint32_t count = 0; count < 12; ++count)
        EXPECT_EQ(answers.at(count), functor("q", {peano(count)}));
}

// ---------- expanded axiom trees (lemmas) ----------

struct PudManifestLemmaIntegrationTest : public PudManifestIntegrationTest {
    pud_node_id nat_fact = 0;
    pud_node_id nat_rule = 0;

    void add_nat_tracked() {
        nat_fact = axiom(functor("nat", {atom("z")}), {}, 0);
        nat_rule = axiom(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    }

    std::vector<pud_node_id> open_children(pud_node_id node) {
        std::vector<pud_node_id> open;
        for (pud_node_id child : manifest.children_.get(node)) {
            if (refuted(child))
                continue;
            if (manifest.node_descents_.get(child).body_goal_expr().body_goal_exprs().empty())
                continue;
            open.push_back(child);
        }
        return open;
    }

    // Expands the nat rule's own tree `depth` times along its recursive branch.
    pud_node_id grow_nat_rule(uint32_t depth) {
        manifest.solver_.start(nat_rule);
        pud_node_id frontier = nat_rule;
        for (uint32_t level = 0; level < depth; ++level) {
            const auto& open_goals = manifest.node_descents_.get(frontier).body_goal_expr().body_goal_exprs();
            manifest.leaf_expander_.expand(frontier, open_goals.begin()->first);
            const std::vector<pud_node_id> recursive = open_children(frontier);
            EXPECT_EQ(recursive.size(), 1u);
            frontier = recursive.front();
        }
        return frontier;
    }

    bool solvable(const expr* goal) {
        const pud_node_id query = start_query(atom("q"), {goal}, 0);
        return manifest.solver_.solve(query, 200).has_value();
    }
};

TEST_F(PudManifestLemmaIntegrationTest, ExpandingTheRuleTreeGivesAFactAndARecursiveChild) {
    add_nat_tracked();
    manifest.solver_.start(nat_rule);
    manifest.leaf_expander_.expand(nat_rule, 0);
    EXPECT_FALSE(manifest.leaf_nodes_.check_leaf(nat_rule));
    EXPECT_EQ(manifest.children_.get(nat_rule).size(), 2u);
    EXPECT_EQ(open_children(nat_rule).size(), 1u);
    EXPECT_FALSE(refuted(nat_rule));
}

TEST_F(PudManifestLemmaIntegrationTest, GroundNumbersAreStillSolvedOnceTheRuleTreeHasGrown) {
    add_nat_tracked();
    grow_nat_rule(3);
    for (uint32_t count = 0; count <= 6; ++count)
        EXPECT_TRUE(solvable(functor("nat", {peano(count)}))) << count;
}

TEST_F(PudManifestLemmaIntegrationTest, NonNumbersAreStillRefutedOnceTheRuleTreeHasGrown) {
    add_nat_tracked();
    grow_nat_rule(3);
    const pud_node_id query = start_query(atom("q"), {functor("nat", {peano_on(2, "bad")})}, 0);
    EXPECT_FALSE(manifest.solver_.solve(query, 200).has_value());
    EXPECT_TRUE(refuted(query));
}

TEST_F(PudManifestLemmaIntegrationTest, EnumerationIsUnchangedByGrowingTheRuleTree) {
    add_nat_tracked();
    grow_nat_rule(2);
    const pud_node_id query = start_query(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const std::vector<const expr*> answers = enumerate(query, 6, 300);
    ASSERT_EQ(answers.size(), 6u);
    for (const expr* found : answers) {
        bool is_number = false;
        for (uint32_t count = 0; count < 20; ++count)
            is_number = is_number || found == functor("q", {peano(count)});
        EXPECT_TRUE(is_number);
    }
    std::vector<const expr*> distinct = answers;
    std::sort(distinct.begin(), distinct.end());
    EXPECT_EQ(std::unique(distinct.begin(), distinct.end()), distinct.end());
}

TEST_F(PudManifestLemmaIntegrationTest, DeepLemmaResolvesALargeNumberInFewExpansions) {
    add_nat_tracked();
    grow_nat_rule(6);
    const pud_node_id query = start_query(atom("q"), {functor("nat", {peano(6)})}, 0);
    EXPECT_TRUE(manifest.solver_.solve(query, 3).has_value());
}

TEST_F(PudManifestLemmaIntegrationTest, RefutingTheRuleTreesRecursiveBranchKeepsQueriesCorrect) {
    add_nat_tracked();
    const pud_node_id recursive = grow_nat_rule(2);
    manifest.node_refuter_.refute(recursive);
    EXPECT_TRUE(refuted(recursive));
    EXPECT_FALSE(refuted(nat_rule));
    EXPECT_TRUE(solvable(functor("nat", {peano(1)})));
}
