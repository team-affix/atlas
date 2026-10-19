#include <algorithm>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "infrastructure/pud_random_rule_chooser.hpp"
#include "infrastructure/pud_random_subgoal_chooser.hpp"
#include "value_objects/pud_unfold.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudManifestConjectureIntegrationTest : public ::testing::Test {
    pud_manifest manifest;
    test_functors functors;
    std::mt19937 rng{0};
    pud_random_rule_chooser rule_chooser{rng};
    pud_random_subgoal_chooser<pud_manifest::node_descents_t> subgoal_chooser{manifest.node_descents_, rng};
    std::map<pud_node_id, std::set<pud_node_id>> reported_solutions;
    std::set<pud_node_id> started_roots;
    std::map<pud_node_id, const expr*> answer_templates;

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

    pud_node_id start_conjecture(const expr* answer_template, std::vector<const expr*> goals, uint32_t var_count) {
        const pud_node_id root = manifest.conjecture_initializer_.initialize_conjecture(answer_template, goals, var_count);
        answer_templates.emplace(root, answer_template);
        run_base_descent();
        return root;
    }

    void run_base_descent() {
        for (pud_node_id root : manifest.root_nodes_.get()) {
            if (started_roots.contains(root))
                continue;
            manifest.base_descent_runner_.run(root);
            started_roots.insert(root);
        }
    }

    const expr* answer(pud_node_id root, pud_node_id solution) {
        pud_transient_ubs_state transient_ubs_state{
            manifest.node_descents_.get(solution).binding().ubs_state()};
        auto bind_map = transient_ubs_state.make_bind_map(manifest.expr_pool_, manifest.expr_pool_);
        auto normalizer = manifest.normalizer_factory_.make(bind_map);
        return normalizer.normalize(framed_expr{answer_templates.at(root), 0});
    }

    pud_node_id root_of(pud_node_id node) {
        while (!manifest.root_nodes_.check_root(node))
            node = manifest.node_parents_.get(node);
        return node;
    }

    bool unfold_conjectures_once() {
        run_base_descent();
        std::set<pud_node_id> conjecture_leaves;
        for (pud_node_id leaf : manifest.open_leaves_.get()) {
            if (!manifest.axioms_.check_axiom(root_of(leaf)))
                conjecture_leaves.insert(leaf);
        }
        if (conjecture_leaves.empty())
            return false;
        const pud_node_id leaf = rule_chooser.choose_rule(conjecture_leaves);
        manifest.unfolder_.unfold(leaf, subgoal_chooser.choose_subgoal(leaf));
        return true;
    }

    std::optional<pud_node_id> find_unreported_solution(pud_node_id root) {
        const std::set<pud_node_id>& reported = reported_solutions[root];
        std::optional<pud_node_id> found;
        for (pud_node_id leaf : manifest.live_leaf_finder_.find_live_leaves(root)) {
            if (manifest.open_leaves_.check_leaf_open(leaf) || reported.contains(leaf))
                continue;
            if (!found.has_value() || leaf < found.value())
                found = leaf;
        }
        return found;
    }

    std::optional<pud_node_id> next_solution(pud_node_id root, size_t max_steps) {
        for (size_t resumed = 0;; ++resumed) {
            const std::optional<pud_node_id> solution = find_unreported_solution(root);
            if (solution.has_value()) {
                reported_solutions[root].insert(solution.value());
                return solution;
            }
            if (resumed == max_steps || !unfold_conjectures_once())
                return std::nullopt;
        }
    }

    bool exhausted(pud_node_id root) {
        const std::set<pud_node_id>& reported = reported_solutions[root];
        for (pud_node_id leaf : manifest.live_leaf_finder_.find_live_leaves(root)) {
            if (manifest.open_leaves_.check_leaf_open(leaf) || !reported.contains(leaf))
                return false;
        }
        return true;
    }

    std::vector<const expr*> enumerate(pud_node_id root, size_t max_answers, size_t max_steps) {
        std::vector<const expr*> answers;
        while (answers.size() < max_answers) {
            const std::optional<pud_node_id> solution = next_solution(root, max_steps);
            if (!solution.has_value())
                break;
            answers.push_back(answer(root, solution.value()));
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

TEST_F(PudManifestConjectureIntegrationTest, GroundQueryAgainstAFactIsSolved) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {atom("red")})}, 0);
    EXPECT_THAT(enumerate(conjecture, 10, 50), ElementsAre(atom("q")));
}

TEST_F(PudManifestConjectureIntegrationTest, QueryVariableIsBoundByEachMatchingFact) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_THAT(
        enumerate(conjecture, 10, 50),
        UnorderedElementsAre(functor("q", {atom("red")}), functor("q", {atom("blue")})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, QueryWithNoMatchingAxiomIsRefutedAtStart) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {atom("green")})}, 0);
    EXPECT_TRUE(refuted(conjecture));
    EXPECT_FALSE(next_solution(conjecture, 50).has_value());
}

TEST_F(PudManifestConjectureIntegrationTest, QueryWithNoBodyGoalsIsItsOwnSolution) {
    const pud_node_id conjecture = start_conjecture(atom("q"), {}, 0);
    EXPECT_EQ(next_solution(conjecture, 1), conjecture);
}

TEST_F(PudManifestConjectureIntegrationTest, ConjunctionKeepsOnlyBindingsThatSatisfyEveryGoal) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0)}),
        {functor("color", {var(0)}), functor("warm", {var(0)})},
        1);
    EXPECT_THAT(enumerate(conjecture, 10, 50), ElementsAre(functor("q", {atom("red")})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, ConjunctionWithNoCommonBindingIsRefuted) {
    add_colors();
    axiom(functor("cold", {atom("blue")}), {}, 0);
    const pud_node_id conjecture = start_conjecture(
        atom("q"),
        {functor("warm", {var(0)}), functor("cold", {var(0)})},
        1);
    EXPECT_THAT(enumerate(conjecture, 10, 50), IsEmpty());
    EXPECT_TRUE(refuted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, NaturalNumbersAreEnumeratedInOrder) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    EXPECT_THAT(
        enumerate(conjecture, 5, 100),
        ElementsAre(
            functor("q", {peano(0)}),
            functor("q", {peano(1)}),
            functor("q", {peano(2)}),
            functor("q", {peano(3)}),
            functor("q", {peano(4)})));
    EXPECT_FALSE(refuted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, GroundNaturalNumberIsChecked) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("nat", {peano(3)})}, 0);
    EXPECT_THAT(enumerate(conjecture, 10, 100), ElementsAre(atom("q")));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, NonNaturalIsRefutedAfterUnfolding) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("nat", {functor("s", {atom("bad")})})}, 0);
    EXPECT_THAT(enumerate(conjecture, 10, 100), IsEmpty());
    EXPECT_TRUE(refuted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, AppendBuildsTheConcatenation) {
    add_append();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0)}),
        {functor("app", {list({atom("a")}), list({atom("b"), atom("c")}), var(0)})},
        1);
    EXPECT_THAT(enumerate(conjecture, 10, 100), ElementsAre(functor("q", {list({atom("a"), atom("b"), atom("c")})})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, AppendSplitsAListEveryWay) {
    add_append();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0), var(1)}),
        {functor("app", {var(0), var(1), list({atom("a"), atom("b")})})},
        2);
    EXPECT_THAT(
        enumerate(conjecture, 10, 100),
        UnorderedElementsAre(
            functor("q", {list({}), list({atom("a"), atom("b")})}),
            functor("q", {list({atom("a")}), list({atom("b")})}),
            functor("q", {list({atom("a"), atom("b")}), list({})})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, MemberFindsEveryElementInOrder) {
    add_member();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0)}),
        {functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})})},
        1);
    EXPECT_THAT(
        enumerate(conjecture, 10, 100),
        ElementsAre(functor("q", {atom("a")}), functor("q", {atom("b")}), functor("q", {atom("c")})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, PeanoAdditionIsInverted) {
    add_peano_add();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0), var(1)}),
        {functor("add", {var(0), var(1), peano(2)})},
        2);
    EXPECT_THAT(
        enumerate(conjecture, 10, 100),
        UnorderedElementsAre(
            functor("q", {peano(0), peano(2)}),
            functor("q", {peano(1), peano(1)}),
            functor("q", {peano(2), peano(0)})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, TransitiveClosureTerminatesOnAFiniteGraph) {
    axiom(functor("parent", {atom("a"), atom("b")}), {}, 0);
    axiom(functor("parent", {atom("b"), atom("c")}), {}, 0);
    axiom(functor("anc", {var(0), var(1)}), {functor("parent", {var(0), var(1)})}, 2);
    axiom(
        functor("anc", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)}), functor("anc", {var(2), var(1)})},
        3);
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("anc", {atom("a"), var(0)})}, 1);
    EXPECT_THAT(
        enumerate(conjecture, 10, 200),
        UnorderedElementsAre(functor("q", {atom("b")}), functor("q", {atom("c")})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, SharedVariablesAcrossGoalsAreRespected) {
    add_append();
    add_member();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0)}),
        {
            functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})}),
            functor("app", {list({atom("x")}), list({var(0)}), list({atom("x"), atom("b")})}),
        },
        1);
    EXPECT_THAT(enumerate(conjecture, 10, 200), ElementsAre(functor("q", {atom("b")})));
    EXPECT_TRUE(exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, ExhaustedBudgetLeavesTheQueryLive) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("nat", {peano(5)})}, 0);
    EXPECT_FALSE(next_solution(conjecture, 2).has_value());
    EXPECT_FALSE(refuted(conjecture));
    EXPECT_TRUE(next_solution(conjecture, 100).has_value());
}

TEST_F(PudManifestConjectureIntegrationTest, RefutingOneSolutionKeepsTheOthers) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("color", {var(0)})}, 1);
    const std::optional<pud_node_id> first = next_solution(conjecture, 50);
    ASSERT_TRUE(first.has_value());
    manifest.node_refuter_.refute(first.value());
    EXPECT_TRUE(refuted(first.value()));
    EXPECT_FALSE(refuted(conjecture));
    const std::optional<pud_node_id> second = next_solution(conjecture, 50);
    ASSERT_TRUE(second.has_value());
    EXPECT_NE(second.value(), first.value());
}

TEST_F(PudManifestConjectureIntegrationTest, TwoQueriesShareTheDatabase) {
    add_colors();
    const pud_node_id warm_conjecture = start_conjecture(functor("w", {var(0)}), {functor("warm", {var(0)})}, 1);
    const pud_node_id color_conjecture = start_conjecture(functor("c", {var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_THAT(enumerate(warm_conjecture, 10, 50), ElementsAre(functor("w", {atom("red")})));
    EXPECT_THAT(
        enumerate(color_conjecture, 10, 50),
        UnorderedElementsAre(functor("c", {atom("red")}), functor("c", {atom("blue")})));
}

TEST_F(PudManifestConjectureIntegrationTest, SolutionLeafHasNoOpenGoals) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const std::optional<pud_node_id> solution = next_solution(conjecture, 50);
    ASSERT_TRUE(solution.has_value());
    EXPECT_TRUE(manifest.node_descents_.get(solution.value()).body_goal_expr().body_goal_exprs().empty());
    EXPECT_TRUE(manifest.leaf_nodes_.check_leaf(solution.value()));
}

TEST_F(PudManifestConjectureIntegrationTest, ManyNaturalNumbersStayCorrect) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const std::vector<const expr*> answers = enumerate(conjecture, 12, 500);
    ASSERT_EQ(answers.size(), 12u);
    for (uint32_t count = 0; count < 12; ++count)
        EXPECT_EQ(answers.at(count), functor("q", {peano(count)}));
}

struct PudManifestConjectureLemmaIntegrationTest : public PudManifestConjectureIntegrationTest {
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

    pud_node_id grow_nat_rule(uint32_t depth) {
        run_base_descent();
        pud_node_id frontier = nat_rule;
        for (uint32_t level = 0; level < depth; ++level) {
            const auto& open_goals = manifest.node_descents_.get(frontier).body_goal_expr().body_goal_exprs();
            manifest.unfolder_.unfold(frontier, open_goals.begin()->first);
            const std::vector<pud_node_id> recursive = open_children(frontier);
            EXPECT_EQ(recursive.size(), 1u);
            frontier = recursive.front();
        }
        return frontier;
    }

    bool solvable(const expr* goal) {
        const pud_node_id conjecture = start_conjecture(atom("q"), {goal}, 0);
        return next_solution(conjecture, 200).has_value();
    }
};

TEST_F(PudManifestConjectureLemmaIntegrationTest, ExpandingTheRuleTreeGivesAFactAndARecursiveChild) {
    add_nat_tracked();
    run_base_descent();
    manifest.unfolder_.unfold(nat_rule, 0);
    EXPECT_FALSE(manifest.leaf_nodes_.check_leaf(nat_rule));
    EXPECT_EQ(manifest.children_.get(nat_rule).size(), 2u);
    EXPECT_EQ(open_children(nat_rule).size(), 1u);
    EXPECT_FALSE(refuted(nat_rule));
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, GroundNumbersAreStillSolvedOnceTheRuleTreeHasGrown) {
    add_nat_tracked();
    grow_nat_rule(3);
    for (uint32_t count = 0; count <= 6; ++count)
        EXPECT_TRUE(solvable(functor("nat", {peano(count)}))) << count;
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, NonNumbersAreStillRefutedOnceTheRuleTreeHasGrown) {
    add_nat_tracked();
    grow_nat_rule(3);
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("nat", {peano_on(2, "bad")})}, 0);
    EXPECT_FALSE(next_solution(conjecture, 200).has_value());
    EXPECT_TRUE(refuted(conjecture));
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, EnumerationIsUnchangedByGrowingTheRuleTree) {
    add_nat_tracked();
    grow_nat_rule(2);
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const std::vector<const expr*> answers = enumerate(conjecture, 6, 300);
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

TEST_F(PudManifestConjectureLemmaIntegrationTest, DeepLemmaResolvesALargeNumberInFewExpansions) {
    add_nat_tracked();
    grow_nat_rule(6);
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("nat", {peano(6)})}, 0);
    EXPECT_TRUE(next_solution(conjecture, 3).has_value());
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, RefutingTheRuleTreesRecursiveBranchKeepsQueriesCorrect) {
    add_nat_tracked();
    const pud_node_id recursive = grow_nat_rule(2);
    manifest.node_refuter_.refute(recursive);
    EXPECT_TRUE(refuted(recursive));
    EXPECT_FALSE(refuted(nat_rule));
    EXPECT_TRUE(solvable(functor("nat", {peano(1)})));
}

struct PudManifestConjectureQueryLifetimeIntegrationTest : public PudManifestConjectureIntegrationTest {
    pud_mhq_head_id query_of(pud_node_id leaf, pud_body_goal_id goal) {
        return manifest.node_descents_.get(leaf).body_goal_pq().find_query_head(goal).value();
    }

    bool alive(pud_mhq_head_id head) { return manifest.mhq_.check_head(head); }

    std::vector<pud_node_id> children_of(pud_node_id node) {
        return manifest.children_.get(node);
    }
};

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, RootsQueriesLiveWhileTheRootIsALeaf) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)}), functor("warm", {var(1)})}, 2);
    EXPECT_TRUE(alive(query_of(conjecture, 0)));
    EXPECT_TRUE(alive(query_of(conjecture, 1)));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, ExpandedLeafKeepsNoStoredDescent) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)})}, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    EXPECT_THROW(manifest.node_descents_.get(conjecture), std::out_of_range);
    for (pud_node_id child : children_of(conjecture))
        EXPECT_NO_THROW(manifest.node_descents_.get(child));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, ExpandedGoalsQueryIsRemovedOnceNoLeafMentionsIt) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)})}, 1);
    const pud_mhq_head_id expanded = query_of(conjecture, 0);
    manifest.unfolder_.unfold(conjecture, 0);
    EXPECT_FALSE(alive(expanded));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, UnchangedGoalsQueryIsSharedByEveryChild) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)}), functor("warm", {var(1)})}, 2);
    const pud_mhq_head_id shared = query_of(conjecture, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    ASSERT_EQ(children_of(conjecture).size(), 2u);
    for (pud_node_id child : children_of(conjecture))
        EXPECT_EQ(query_of(child, 1), shared);
    EXPECT_TRUE(alive(shared));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, SharedQuerySurvivesUntilItsLastLeafIsRefuted) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)}), functor("warm", {var(1)})}, 2);
    const pud_mhq_head_id shared = query_of(conjecture, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    const std::vector<pud_node_id> children = children_of(conjecture);
    ASSERT_EQ(children.size(), 2u);
    manifest.node_refuter_.refute(children.at(0));
    EXPECT_TRUE(alive(shared));
    manifest.node_refuter_.refute(children.at(1));
    EXPECT_FALSE(alive(shared));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, SharedQuerySurvivesUntilItsLastLeafIsExpandedPastIt) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)}), functor("warm", {var(1)})}, 2);
    const pud_mhq_head_id shared = query_of(conjecture, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    const std::vector<pud_node_id> children = children_of(conjecture);
    ASSERT_EQ(children.size(), 2u);
    manifest.unfolder_.unfold(children.at(0), 1);
    EXPECT_TRUE(alive(shared));
    manifest.unfolder_.unfold(children.at(1), 1);
    EXPECT_FALSE(alive(shared));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, ForkedQueryReplacesTheParentsQueryForEveryChild) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    const pud_mhq_head_id parents_query = query_of(conjecture, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    EXPECT_FALSE(alive(parents_query));
    for (pud_node_id child : children_of(conjecture)) {
        if (refuted(child))
            continue;
        const pud_mhq_head_id forked = query_of(child, 1);
        EXPECT_NE(forked, parents_query);
        EXPECT_TRUE(alive(forked));
    }
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, RefutedChildsForkedQueryIsRemoved) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(
        functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    const std::vector<pud_node_id> children = children_of(conjecture);
    ASSERT_EQ(children.size(), 2u);
    pud_node_id live_child = children.at(0);
    for (pud_node_id child : children) {
        if (!refuted(child))
            live_child = child;
    }
    const pud_mhq_head_id forked = query_of(live_child, 1);
    manifest.node_refuter_.refute(live_child);
    EXPECT_FALSE(alive(forked));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, RefutingTheRootRemovesEveryQuery) {
    add_colors();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("color", {var(0)}), functor("warm", {var(1)})}, 2);
    const pud_mhq_head_id colors = query_of(conjecture, 0);
    const pud_mhq_head_id warmth = query_of(conjecture, 1);
    manifest.node_refuter_.refute(conjecture);
    EXPECT_FALSE(alive(colors));
    EXPECT_FALSE(alive(warmth));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, NewGoalsQueryLivesWithTheChildThatAddedIt) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(atom("q"), {functor("nat", {var(0)})}, 1);
    manifest.unfolder_.unfold(conjecture, 0);
    pud_node_id recursive = 0;
    for (pud_node_id child : children_of(conjecture)) {
        if (!manifest.node_descents_.get(child).body_goal_expr().body_goal_exprs().empty())
            recursive = child;
    }
    ASSERT_NE(recursive, 0u);
    const pud_mhq_head_id added = query_of(recursive, 1);
    EXPECT_EQ(manifest.query_head_nodes_.get(added), recursive);
    EXPECT_TRUE(alive(added));
    manifest.node_refuter_.refute(recursive);
    EXPECT_FALSE(alive(added));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, SolvingLeavesOnlyTheFrontiersQueriesAlive) {
    add_nat();
    const pud_node_id conjecture = start_conjecture(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1);
    const pud_mhq_head_id root_query = query_of(conjecture, 0);
    ASSERT_TRUE(next_solution(conjecture, 20).has_value());
    EXPECT_FALSE(alive(root_query));
    for (pud_node_id leaf : manifest.live_leaf_finder_.find_live_leaves(conjecture)) {
        for (const auto& [goal, goal_expr] : manifest.node_descents_.get(leaf).body_goal_expr().body_goal_exprs())
            EXPECT_TRUE(alive(query_of(leaf, goal)));
    }
}
