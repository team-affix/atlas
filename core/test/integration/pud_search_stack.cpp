#include <deque>
#include <optional>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_refuted_leaves.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"

// Replayer, witness searches, candidate searches and queries over real node stores.
struct PudSearchStackIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using root_heads_t = pud_node_table<const expr*>;
    using substitutions_t = pud_node_table<std::vector<pud_substitution>>;
    using children_t = pud_node_table<std::vector<pud_node_id>>;
    using axioms_t = std::vector<pud_node_id>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;
    using refuted_node_checker_t =
        pud_refuted_node_checker<pud_leaf_nodes, pud_refuted_leaves, pud_node_live_child_counts>;
    using replayer_t = pud_replayer<root_heads_t, substitutions_t, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, pud_leaf_nodes, refuted_node_checker_t, children_t>;
    using mhcs_t =
        pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, pud_leaf_nodes, refuted_node_checker_t, children_t>;
    using mhq_t = pud_mhq<replayer_t, axioms_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_leaf_nodes leaf_nodes;
    pud_refuted_leaves refuted_leaves;
    pud_node_live_child_counts live_child_counts;
    root_heads_t root_heads;
    substitutions_t substitutions;
    children_t children;
    axioms_t axioms;
    unit_queries_t unit_queries;
    refuted_node_checker_t refuted_node_checker{leaf_nodes, refuted_leaves, live_child_counts};
    replayer_t replayer{root_heads, substitutions, exprs, terms, terms, terms};
    mhws_t mhws{replayer, leaf_nodes, refuted_node_checker, children};
    mhcs_t mhcs{replayer, mhws, mhws, mhws, mhws, mhws, leaf_nodes, refuted_node_checker, children};
    mhq_t mhq{replayer, axioms, mhcs, mhcs, mhcs, mhcs, unit_queries};
    pud_ubs_state empty_state;
    pud_node_id next_node = 0;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    pud_node_id axiom(const expr* head) {
        const pud_node_id node = next_node++;
        leaf_nodes.set_leaf(node);
        root_heads.store(node, head);
        axioms.push_back(node);
        return node;
    }

    pud_node_id leaf(std::vector<pud_substitution> added) {
        const pud_node_id node = next_node++;
        leaf_nodes.set_leaf(node);
        substitutions.store(node, std::move(added));
        return node;
    }

    void expand(pud_node_id parent, std::vector<pud_node_id> kids) {
        leaf_nodes.unset_leaf(parent);
        live_child_counts.store(parent, static_cast<uint32_t>(kids.size()));
        children.store(parent, std::move(kids));
    }

    void refute(pud_node_id node, pud_node_id parent) {
        refuted_leaves.mark_refuted(node);
        live_child_counts.decrement(parent);
    }

    std::vector<pud_node_id> roots_of(pud_mhq_head_id query) {
        std::vector<pud_node_id> roots;
        for (pud_mhcs_head_id candidate : mhq.candidates(query))
            roots.push_back(mhcs.path(candidate).front());
        return roots;
    }

    std::vector<std::vector<pud_node_id>> paths_of(pud_mhq_head_id query) {
        std::vector<std::vector<pud_node_id>> paths;
        for (pud_mhcs_head_id candidate : mhq.candidates(query))
            paths.push_back(mhcs.path(candidate));
        return paths;
    }

    pud_ubs_state replay_path(
        const pud_ubs_state& caller_state, const expr* goal, uint32_t frame_offset, const std::vector<pud_node_id>& path) {
        pud_replay replay = replayer.enter(caller_state, goal, frame_offset, path.front()).value();
        for (size_t step = 1; step < path.size(); ++step)
            replay = replayer.descend(replay, path.at(step)).value();
        return replay.ubs_state;
    }

    const expr* resolve(const pud_ubs_state& state, uint32_t index) {
        ubs_bind_map bind_map{state};
        return terms.make_normalizer(bind_map).normalize(framed_expr{var(index), 0});
    }
};

TEST_F(PudSearchStackIntegrationTest, OnlyAxiomsWhoseHeadsResolveTheGoalBecomeCandidates) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    const pud_node_id blue = axiom(functor("color", {atom("blue")}));
    axiom(functor("warm", {atom("red")}));
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red, blue));
}

TEST_F(PudSearchStackIntegrationTest, HeadedNodesThatAreNotAxiomsAreNeverCandidates) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    const pud_node_id headed = next_node++;
    leaf_nodes.set_leaf(headed);
    root_heads.store(headed, functor("color", {atom("blue")}));
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red));
}

TEST_F(PudSearchStackIntegrationTest, EachCandidatePathReplaysItsResolution) {
    axiom(functor("color", {atom("red")}));
    axiom(functor("color", {atom("blue")}));
    const expr* goal = functor("color", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    const std::vector<std::vector<pud_node_id>> paths = paths_of(query.value());
    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(resolve(replay_path(empty_state, goal, 1, paths.at(0)), 0), atom("red"));
    EXPECT_EQ(resolve(replay_path(empty_state, goal, 1, paths.at(1)), 0), atom("blue"));
}

TEST_F(PudSearchStackIntegrationTest, CallersBindingsAreRespected) {
    axiom(functor("color", {atom("red")}));
    const pud_node_id blue = axiom(functor("color", {atom("blue")}));
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_expr{atom("blue"), 0});
    const pud_ubs_state caller = std::move(bind_map).persist();
    const std::optional<pud_mhq_head_id> query = mhq.try_open(caller, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(blue));
}

TEST_F(PudSearchStackIntegrationTest, RootHeadIsLiftedToTheFrameOffset) {
    axiom(functor("nat", {functor("s", {var(0)})}));
    const expr* goal = functor("nat", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, goal, 3);
    ASSERT_TRUE(query.has_value());
    const std::vector<std::vector<pud_node_id>> paths = paths_of(query.value());
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(resolve(replay_path(empty_state, goal, 3, paths.at(0)), 0), functor("s", {var(3)}));
}

TEST_F(PudSearchStackIntegrationTest, NoAxiomResolvesTheGoal) {
    axiom(functor("color", {atom("red")}));
    EXPECT_FALSE(mhq.try_open(empty_state, functor("warm", {var(0)}), 1).has_value());
    EXPECT_TRUE(unit_queries.empty());
}

TEST_F(PudSearchStackIntegrationTest, CallersStateIsLeftUntouched) {
    axiom(functor("color", {atom("red")}));
    mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    EXPECT_EQ(resolve(empty_state, 0), var(0));
}

TEST_F(PudSearchStackIntegrationTest, SingleCandidateQueryIsQueuedAsUnit) {
    axiom(functor("color", {atom("red")}));
    axiom(functor("warm", {atom("red")}));
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(unit_queries, ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, CandidateSpecializesThroughTheOnlyCompatibleChild) {
    const pud_node_id root = axiom(functor("color", {var(0)}));
    const pud_node_id red = leaf({pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_node_id blue = leaf({pud_substitution{.var_idx = 0, .value = atom("blue")}});
    expand(root, {red, blue});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {atom("blue")}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
}

TEST_F(PudSearchStackIntegrationTest, TwoCompatibleChildrenMakeAChoicePoint) {
    const pud_node_id root = axiom(functor("color", {var(0)}));
    const pud_node_id red = leaf({pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_node_id blue = leaf({pud_substitution{.var_idx = 0, .value = atom("blue")}});
    expand(root, {red, blue});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root)));
}

TEST_F(PudSearchStackIntegrationTest, ChoicePointRejustifiesThroughAThirdWitness) {
    const pud_node_id root = axiom(functor("color", {var(0)}));
    const pud_node_id red = leaf({pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_node_id green = leaf({pud_substitution{.var_idx = 0, .value = atom("green")}});
    const pud_node_id blue = leaf({pud_substitution{.var_idx = 0, .value = atom("blue")}});
    expand(root, {red, green, blue});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    refute(red, root);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root)));
}

TEST_F(PudSearchStackIntegrationTest, ChoicePointLosingAWitnessSpecializesToTheSurvivor) {
    const pud_node_id root = axiom(functor("color", {var(0)}));
    const pud_node_id red = leaf({pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_node_id blue = leaf({pud_substitution{.var_idx = 0, .value = atom("blue")}});
    expand(root, {red, blue});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    refute(red, root);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
}

TEST_F(PudSearchStackIntegrationTest, LosingOneOfTwoCandidatesQueuesTheQueryAsUnit) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    const pud_node_id blue = axiom(functor("color", {atom("blue")}));
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    ASSERT_TRUE(unit_queries.empty());
    refuted_leaves.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(blue));
    EXPECT_THAT(unit_queries, ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, LosingTheLastCandidateRefutesTheQuery) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    refuted_leaves.mark_refuted(red);
    EXPECT_THAT(mhq.invalidate_leaf(red), ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, ForkKeepsOnlyCandidatesCompatibleWithTheNewState) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    const pud_node_id blue = axiom(functor("color", {atom("blue")}));
    const expr* goal = functor("color", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_expr{atom("blue"), 0});
    const pud_ubs_state narrowed = std::move(bind_map).persist();
    const std::optional<pud_mhq_head_id> forked = mhq.try_fork(query.value(), narrowed, goal, 1);
    ASSERT_TRUE(forked.has_value());
    EXPECT_THAT(roots_of(forked.value()), ::testing::ElementsAre(blue));
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red, blue));
}
