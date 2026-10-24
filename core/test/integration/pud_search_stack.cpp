#include <deque>
#include <optional>
#include <vector>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_test_node.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/rule.hpp"

// Replayer, witness searches, candidate searches and queries over the real modules. Each test builds
// its modules' axioms up front, registers them, then stores the inferences below them.
struct PudSearchStackIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;
    using modules_t = pud_modules<pud_module>;
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
    std::deque<pud_module> module_data;
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
};

TEST_F(PudSearchStackIntegrationTest, OnlyAxiomsWhoseHeadsResolveTheGoalBecomeCandidates) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
        });
    modules.register_module(module_data.back());
    const pud_global_node_id red = pud_global_node_id{.module = k_module, .node = 0};
    const pud_global_node_id blue = pud_global_node_id{.module = k_module, .node = 1};
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    std::vector<pud_global_node_id> roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(roots, ::testing::ElementsAre(red, blue));
}

TEST_F(PudSearchStackIntegrationTest, AxiomsOfAModuleTheQueryDoesNotDependOnAreNeverCandidates) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    modules.register_module(module_data.back());
    module_data.emplace_back(
        k_other_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("blue")}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id red = pud_global_node_id{.module = k_module, .node = 0};
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    std::vector<pud_global_node_id> roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(roots, ::testing::ElementsAre(red));
}

TEST_F(PudSearchStackIntegrationTest, DependencysAxiomsAreCandidatesBeforeTheModulesOwn) {
    module_data.emplace_back(
        k_other_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    modules.register_module(module_data.back());
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{k_other_module},
        std::vector<rule>{rule(functor("color", {atom("blue")}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id red = pud_global_node_id{.module = k_other_module, .node = 0};
    const pud_global_node_id blue = pud_global_node_id{.module = k_module, .node = 0};
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    std::vector<pud_global_node_id> roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(roots, ::testing::ElementsAre(red, blue));
}

TEST_F(PudSearchStackIntegrationTest, ModulesWithOverlappingNodeIdsNeverMix) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {var(0)}), {}, 0)});
    modules.register_module(module_data.back());
    module_data.emplace_back(
        k_other_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {var(0)}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id root = pud_global_node_id{.module = k_module, .node = 0};
    const pud_global_node_id other_root = pud_global_node_id{.module = k_other_module, .node = 0};
    modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("red")}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id blue = modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("blue")}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id other_blue = modules.store_inference(other_root.module, pud_inference{
        .parent = other_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("blue")}},
        .body_goals = {},
        .var_count = 0,
    });
    ASSERT_EQ(root.node, other_root.node);
    const expr* goal = functor("color", {atom("blue")});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 1);
    const std::optional<pud_mhq_head_id> other_query = mhq.try_open(k_other_module, empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    ASSERT_TRUE(other_query.has_value());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    std::vector<std::vector<pud_global_node_id>> other_paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(other_query.value()))
        other_paths.push_back(mhcs.path(candidate));
    EXPECT_THAT(paths, ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
    EXPECT_THAT(other_paths, ::testing::ElementsAre(::testing::ElementsAre(other_root, other_blue)));
}

TEST_F(PudSearchStackIntegrationTest, EachCandidatePathReplaysItsResolution) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
        });
    modules.register_module(module_data.back());
    const expr* goal = functor("color", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    ASSERT_EQ(paths.size(), 2u);
    pud_replay replay0 = replayer.enter(empty_state, goal, 1, paths.at(0).front()).value();
    for (size_t step = 1; step < paths.at(0).size(); ++step)
        replay0 = replayer.descend(replay0, paths.at(0).at(step)).value();
    ubs_bind_map bind_map0{replay0.ubs_state};
    const expr* resolved0 = terms.make_normalizer(bind_map0).normalize(framed_expr{var(0), 0});
    EXPECT_EQ(resolved0, atom("red"));
    pud_replay replay1 = replayer.enter(empty_state, goal, 1, paths.at(1).front()).value();
    for (size_t step = 1; step < paths.at(1).size(); ++step)
        replay1 = replayer.descend(replay1, paths.at(1).at(step)).value();
    ubs_bind_map bind_map1{replay1.ubs_state};
    const expr* resolved1 = terms.make_normalizer(bind_map1).normalize(framed_expr{var(0), 0});
    EXPECT_EQ(resolved1, atom("blue"));
}

TEST_F(PudSearchStackIntegrationTest, CallersBindingsAreRespected) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
        });
    modules.register_module(module_data.back());
    const pud_global_node_id blue = pud_global_node_id{.module = k_module, .node = 1};
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_expr{atom("blue"), 0});
    const pud_ubs_state caller = std::move(bind_map).persist();
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, caller, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    std::vector<pud_global_node_id> roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(roots, ::testing::ElementsAre(blue));
}

TEST_F(PudSearchStackIntegrationTest, RootHeadIsLiftedToTheFrameOffset) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("nat", {functor("s", {var(0)})}), {}, 0)});
    modules.register_module(module_data.back());
    const expr* goal = functor("nat", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 3);
    ASSERT_TRUE(query.has_value());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    ASSERT_EQ(paths.size(), 1u);
    pud_replay replay = replayer.enter(empty_state, goal, 3, paths.at(0).front()).value();
    for (size_t step = 1; step < paths.at(0).size(); ++step)
        replay = replayer.descend(replay, paths.at(0).at(step)).value();
    ubs_bind_map bind_map{replay.ubs_state};
    const expr* resolved = terms.make_normalizer(bind_map).normalize(framed_expr{var(0), 0});
    EXPECT_EQ(resolved, functor("s", {var(3)}));
}

TEST_F(PudSearchStackIntegrationTest, NoAxiomResolvesTheGoal) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    modules.register_module(module_data.back());
    EXPECT_FALSE(mhq.try_open(k_module, empty_state, functor("warm", {var(0)}), 1).has_value());
    EXPECT_TRUE(unit_queries.empty());
}

TEST_F(PudSearchStackIntegrationTest, CallersStateIsLeftUntouched) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    modules.register_module(module_data.back());
    mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ubs_bind_map bind_map{empty_state};
    const expr* resolved = terms.make_normalizer(bind_map).normalize(framed_expr{var(0), 0});
    EXPECT_EQ(resolved, var(0));
}

TEST_F(PudSearchStackIntegrationTest, SingleCandidateQueryIsQueuedAsUnit) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
        });
    modules.register_module(module_data.back());
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    EXPECT_THAT(unit_queries, ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, CandidateSpecializesThroughTheOnlyCompatibleChild) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {var(0)}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id root = pud_global_node_id{.module = k_module, .node = 0};
    modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("red")}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id blue = modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("blue")}},
        .body_goals = {},
        .var_count = 0,
    });
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {atom("blue")}), 1);
    ASSERT_TRUE(query.has_value());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    EXPECT_THAT(paths, ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
}

TEST_F(PudSearchStackIntegrationTest, TwoCompatibleChildrenMakeAChoicePoint) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {var(0)}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id root = pud_global_node_id{.module = k_module, .node = 0};
    modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("red")}},
        .body_goals = {},
        .var_count = 0,
    });
    modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("blue")}},
        .body_goals = {},
        .var_count = 0,
    });
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    EXPECT_THAT(paths, ::testing::ElementsAre(::testing::ElementsAre(root)));
}

TEST_F(PudSearchStackIntegrationTest, ChoicePointRejustifiesThroughAThirdWitness) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {var(0)}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id root = pud_global_node_id{.module = k_module, .node = 0};
    const pud_global_node_id red = modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("red")}},
        .body_goals = {},
        .var_count = 0,
    });
    modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("green")}},
        .body_goals = {},
        .var_count = 0,
    });
    modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("blue")}},
        .body_goals = {},
        .var_count = 0,
    });
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    modules.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    EXPECT_THAT(paths, ::testing::ElementsAre(::testing::ElementsAre(root)));
}

TEST_F(PudSearchStackIntegrationTest, ChoicePointLosingAWitnessSpecializesToTheSurvivor) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {var(0)}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id root = pud_global_node_id{.module = k_module, .node = 0};
    const pud_global_node_id red = modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("red")}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id blue = modules.store_inference(root.module, pud_inference{
        .parent = root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("blue")}},
        .body_goals = {},
        .var_count = 0,
    });
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    modules.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    std::vector<std::vector<pud_global_node_id>> paths;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        paths.push_back(mhcs.path(candidate));
    EXPECT_THAT(paths, ::testing::ElementsAre(::testing::ElementsAre(root, blue)));
}

TEST_F(PudSearchStackIntegrationTest, LosingOneOfTwoCandidatesQueuesTheQueryAsUnit) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
        });
    modules.register_module(module_data.back());
    const pud_global_node_id red = pud_global_node_id{.module = k_module, .node = 0};
    const pud_global_node_id blue = pud_global_node_id{.module = k_module, .node = 1};
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    ASSERT_TRUE(unit_queries.empty());
    modules.mark_refuted(red);
    EXPECT_TRUE(mhq.invalidate_leaf(red).empty());
    std::vector<pud_global_node_id> roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(roots, ::testing::ElementsAre(blue));
    EXPECT_THAT(unit_queries, ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, LosingTheLastCandidateRefutesTheQuery) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("color", {atom("red")}), {}, 0)});
    modules.register_module(module_data.back());
    const pud_global_node_id red = pud_global_node_id{.module = k_module, .node = 0};
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, functor("color", {var(0)}), 1);
    ASSERT_TRUE(query.has_value());
    modules.mark_refuted(red);
    EXPECT_THAT(mhq.invalidate_leaf(red), ::testing::ElementsAre(query.value()));
}

TEST_F(PudSearchStackIntegrationTest, ForkKeepsOnlyCandidatesCompatibleWithTheNewState) {
    module_data.emplace_back(
        k_module,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
        });
    modules.register_module(module_data.back());
    const pud_global_node_id red = pud_global_node_id{.module = k_module, .node = 0};
    const pud_global_node_id blue = pud_global_node_id{.module = k_module, .node = 1};
    const expr* goal = functor("color", {var(0)});
    const std::optional<pud_mhq_head_id> query = mhq.try_open(k_module, empty_state, goal, 1);
    ASSERT_TRUE(query.has_value());
    ubs_bind_map bind_map{empty_state};
    bind_map.instantiate(bind_map.find(0), framed_expr{atom("blue"), 0});
    const pud_ubs_state narrowed = std::move(bind_map).persist();
    const std::optional<pud_mhq_head_id> forked = mhq.try_fork(query.value(), narrowed, goal, 1);
    ASSERT_TRUE(forked.has_value());
    std::vector<pud_global_node_id> forked_roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(forked.value()))
        forked_roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(forked_roots, ::testing::ElementsAre(blue));
    std::vector<pud_global_node_id> query_roots;
    for (pud_mhcs_head_id candidate : mhq.candidates(query.value()))
        query_roots.push_back(mhcs.path(candidate).front());
    EXPECT_THAT(query_roots, ::testing::ElementsAre(red, blue));
}
