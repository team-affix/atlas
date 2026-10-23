#include <deque>
#include <optional>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_test_node.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_data.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"

// Replayer, witness searches, candidate searches and queries over the real modules. Each test builds
// its modules' axioms up front, registers them, then stores the inferences below them.
struct PudSearchStackIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;
    using modules_t = pud_modules<pud_data>;
    using replayer_t = pud_replayer<modules_t, modules_t, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, modules_t, modules_t, modules_t>;
    using mhcs_t =
        pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, modules_t, modules_t, modules_t>;
    using mhq_t = pud_mhq<replayer_t, modules_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    static constexpr pud_module_id k_module = 0;
    static constexpr pud_module_id k_other_module = 1;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    std::deque<pud_data> module_data;
    modules_t modules;
    unit_queries_t unit_queries;
    replayer_t replayer{modules, modules, exprs, terms, terms, terms};
    mhws_t mhws{replayer, modules, modules, modules};
    mhcs_t mhcs{replayer, mhws, mhws, mhws, mhws, mhws, modules, modules, modules};
    mhq_t mhq{replayer, modules, mhcs, mhcs, mhcs, mhcs, unit_queries};
    pud_ubs_state empty_state;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    // Registers a module whose axioms are facts with these heads, in order: axiom i is local node i.
    void make_module(pud_module_id module, std::vector<pud_module_id> dependencies, std::vector<const expr*> heads) {
        std::vector<rule> axioms;
        for (const expr* head : heads)
            axioms.push_back(rule(head, {}, 0));
        module_data.emplace_back(module, std::move(dependencies), std::move(axioms));
        modules.register_module(module_data.back());
    }

    pud_global_node_id node(pud_module_id module, pud_node_id local) {
        return pud_global_node_id{.module = module, .node = local};
    }

    pud_global_node_id leaf(pud_global_node_id parent, std::vector<pud_substitution> added) {
        return modules.store_inference(parent.module, pud_node_delta{
            .parent = parent.node,
            .call_site = 0,
            .substitutions = std::move(added),
            .body_goals = {},
            .var_count = 0,
        });
    }

    std::vector<pud_global_node_id> roots_of(pud_mhq_head_id query) {
        std::vector<pud_global_node_id> roots;
        for (pud_mhcs_head_id candidate : mhq.candidates(query))
            roots.push_back(mhcs.path(candidate).front());
        return roots;
    }

    std::vector<std::vector<pud_global_node_id>> paths_of(pud_mhq_head_id query) {
        std::vector<std::vector<pud_global_node_id>> paths;
        for (pud_mhcs_head_id candidate : mhq.candidates(query))
            paths.push_back(mhcs.path(candidate));
        return paths;
    }

    pud_ubs_state replay_path(
        const pud_ubs_state& caller_state,
        const expr* goal,
        uint32_t frame_offset,
        const std::vector<pud_global_node_id>& path) {
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
    make_module(k_module, {}, {
        functor("color", {atom("red")}),
        functor("color", {atom("blue")}),
        functor("warm", {atom("red")}),
    });
    const pud_global_node_id red = node(k_module, 0);
    const pud_global_node_id blue = node(k_module, 1);
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red, blue));
}

TEST_F(PudSearchStackIntegrationTest, AxiomsOfAModuleTheQueryDoesNotDependOnAreNeverCandidates) {
    make_module(k_module, {}, {functor("color", {atom("red")})});
    make_module(k_other_module, {}, {functor("color", {atom("blue")})});
    const pud_global_node_id red = node(k_module, 0);
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red));
}

TEST_F(PudSearchStackIntegrationTest, DependencysAxiomsAreCandidatesBeforeTheModulesOwn) {
    make_module(k_other_module, {}, {functor("color", {atom("red")})});
    make_module(k_module, {k_other_module}, {functor("color", {atom("blue")})});
    const pud_global_node_id red = node(k_other_module, 0);
    const pud_global_node_id blue = node(k_module, 0);
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red, blue));
}

TEST_F(PudSearchStackIntegrationTest, ModulesWithOverlappingNodeIdsNeverMix) {
    make_module(k_module, {}, {functor("color", {var(0)})});
    make_module(k_other_module, {}, {functor("color", {var(0)})});
    const pud_global_node_id root = node(k_module, 0);
    const pud_global_node_id other_root = node(k_other_module, 0);
    leaf(root, {pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_global_node_id blue = leaf(root, {pud_substitution{.var_idx = 0, .value = atom("blue")}});
    const pud_global_node_id other_blue = leaf(other_root, {pud_substitution{.var_idx = 0, .value = atom("blue")}});
    ASSERT_EQ(root.node, other_root.node);
    const expr* goal = functor("color", {atom("blue")});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 1);
    const std::optional<pud_mhq_head_id> other_query = mhq.try_open(k_other_module, empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    ASSERT_TRUE(other_query.has_value());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
    EXPECT_THAT(paths_of(other_query.value()), ::testing::ElementsAre(::testing::ElementsAre(other_root, other_blue)));
}

TEST_F(PudSearchStackIntegrationTest, EachCandidatePathReplaysItsResolution) {
    make_module(k_module, {}, {functor("color", {atom("red")}), functor("color", {atom("blue")})});
    const expr* goal = functor("color", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    const std::vector<std::vector<pud_global_node_id>> paths = paths_of(query.value());
    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(resolve(replay_path(empty_state, goal, 1, paths.at(0)), 0), atom("red"));
    EXPECT_EQ(resolve(replay_path(empty_state, goal, 1, paths.at(1)), 0), atom("blue"));
}

TEST_F(PudSearchStackIntegrationTest, CallersBindingsAreRespected) {
    make_module(k_module, {}, {functor("color", {atom("red")}), functor("color", {atom("blue")})});
    const pud_global_node_id blue = node(k_module, 1);
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_expr{atom("blue"), 0});
    const pud_ubs_state caller = std::move(bind_map).persist();
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, caller, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(blue));
}

TEST_F(PudSearchStackIntegrationTest, RootHeadIsLiftedToTheFrameOffset) {
    make_module(k_module, {}, {functor("nat", {functor("s", {var(0)})})});
    const expr* goal = functor("nat", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 3);
    ASSERT_TRUE(query.has_value());
    const std::vector<std::vector<pud_global_node_id>> paths = paths_of(query.value());
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(resolve(replay_path(empty_state, goal, 3, paths.at(0)), 0), functor("s", {var(3)}));
}

TEST_F(PudSearchStackIntegrationTest, NoAxiomResolvesTheGoal) {
    make_module(k_module, {}, {functor("color", {atom("red")})});
    EXPECT_FALSE(mhq.try_open(k_module, empty_state, functor("warm", {var(0)}), 1).has_value());
    EXPECT_TRUE(unit_queries.empty());
}

TEST_F(PudSearchStackIntegrationTest, CallersStateIsLeftUntouched) {
    make_module(k_module, {}, {functor("color", {atom("red")})});
    mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    EXPECT_EQ(resolve(empty_state, 0), var(0));
}

TEST_F(PudSearchStackIntegrationTest, SingleCandidateQueryIsQueuedAsUnit) {
    make_module(k_module, {}, {functor("color", {atom("red")}), functor("warm", {atom("red")})});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(unit_queries, ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, CandidateSpecializesThroughTheOnlyCompatibleChild) {
    make_module(k_module, {}, {functor("color", {var(0)})});
    const pud_global_node_id root = node(k_module, 0);
    leaf(root, {pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_global_node_id blue = leaf(root, {pud_substitution{.var_idx = 0, .value = atom("blue")}});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {atom("blue")}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
}

TEST_F(PudSearchStackIntegrationTest, TwoCompatibleChildrenMakeAChoicePoint) {
    make_module(k_module, {}, {functor("color", {var(0)})});
    const pud_global_node_id root = node(k_module, 0);
    leaf(root, {pud_substitution{.var_idx = 0, .value = atom("red")}});
    leaf(root, {pud_substitution{.var_idx = 0, .value = atom("blue")}});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root)));
}

TEST_F(PudSearchStackIntegrationTest, ChoicePointRejustifiesThroughAThirdWitness) {
    make_module(k_module, {}, {functor("color", {var(0)})});
    const pud_global_node_id root = node(k_module, 0);
    const pud_global_node_id red = leaf(root, {pud_substitution{.var_idx = 0, .value = atom("red")}});
    leaf(root, {pud_substitution{.var_idx = 0, .value = atom("green")}});
    leaf(root, {pud_substitution{.var_idx = 0, .value = atom("blue")}});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    modules.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root)));
}

TEST_F(PudSearchStackIntegrationTest, ChoicePointLosingAWitnessSpecializesToTheSurvivor) {
    make_module(k_module, {}, {functor("color", {var(0)})});
    const pud_global_node_id root = node(k_module, 0);
    const pud_global_node_id red = leaf(root, {pud_substitution{.var_idx = 0, .value = atom("red")}});
    const pud_global_node_id blue = leaf(root, {pud_substitution{.var_idx = 0, .value = atom("blue")}});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    modules.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    EXPECT_THAT(paths_of(query.value()), ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
}

TEST_F(PudSearchStackIntegrationTest, LosingOneOfTwoCandidatesQueuesTheQueryAsUnit) {
    make_module(k_module, {}, {functor("color", {atom("red")}), functor("color", {atom("blue")})});
    const pud_global_node_id red = node(k_module, 0);
    const pud_global_node_id blue = node(k_module, 1);
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    ASSERT_TRUE(unit_queries.empty());
    modules.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(blue));
    EXPECT_THAT(unit_queries, ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, LosingTheLastCandidateRefutesTheQuery) {
    make_module(k_module, {}, {functor("color", {atom("red")})});
    const pud_global_node_id red = node(k_module, 0);
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    modules.mark_refuted(red);
    EXPECT_THAT(mhq.invalidate_leaf(red), ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, ForkKeepsOnlyCandidatesCompatibleWithTheNewState) {
    make_module(k_module, {}, {functor("color", {atom("red")}), functor("color", {atom("blue")})});
    const pud_global_node_id red = node(k_module, 0);
    const pud_global_node_id blue = node(k_module, 1);
    const expr* goal = functor("color", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_expr{atom("blue"), 0});
    const pud_ubs_state narrowed = std::move(bind_map).persist();
    const std::optional<pud_mhq_head_id> forked = mhq.try_fork(query.value(), narrowed, goal, 1);
    ASSERT_TRUE(forked.has_value());
    EXPECT_THAT(roots_of(forked.value()), ::testing::ElementsAre(blue));
    EXPECT_THAT(roots_of(query.value()), ::testing::ElementsAre(red, blue));
}
