#include <algorithm>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "pud_identity_demoduler.hpp"
#include "pud_runtime.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_candidate_witness_index.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_eager_goal_querier.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_registry.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resumable_candidate_specializer.hpp"
#include "infrastructure/pud_resumable_witness_search.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_witness_search_recorder.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/rule.hpp"

// The eager querier must find exactly the candidates the reactive search stack (MHQ over MHCS over MHWS) admits for
// the same goal and bindings: the same specialized paths and the same witness leaves. Trees are grown by random
// unfolds through the manual runtime, then every open goal of every live leaf is asked of both, each a fresh query
// over the same stored modules.
struct PudEagerGoalQuerierIntegrationTest : public ::testing::Test {
    using bind_map_factory_t = pud_bind_map_factory;
    using copy_demoduler_t = pud_copy_functor_demoduler<expr_pool>;
    using cycle_detector_factory_t = pud_cycle_detector_factory<globalizer, copy_demoduler_t>;
    using unifier_factory_t = pud_unifier_factory<
        globalizer, expr_pool, pud_identity_global_functor_ids, cycle_detector_factory_t::cycle_detector_t>;
    using module_registry_t = pud_module_registry<pud_module>;
    using replayer_t = pud_replayer<
        module_registry_t, module_registry_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t>;
    using mhws_t = pud_mhws<replayer_t, module_registry_t, module_registry_t, module_registry_t>;
    using recorder_t = pud_witness_search_recorder<mhws_t>;
    using mhcs_t = pud_mhcs<
        replayer_t, recorder_t, recorder_t, recorder_t, recorder_t, recorder_t, module_registry_t, module_registry_t,
        module_registry_t>;
    using candidate_witness_index_t = pud_candidate_witness_index<mhcs_t, recorder_t>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;
    using mhq_t = pud_mhq<
        replayer_t, module_registry_t, candidate_witness_index_t, candidate_witness_index_t, candidate_witness_index_t,
        candidate_witness_index_t, unit_queries_t>;
    using witness_search_t = pud_resumable_witness_search<replayer_t, module_registry_t, module_registry_t, module_registry_t>;
    using specializer_t = pud_resumable_candidate_specializer<
        replayer_t, witness_search_t, witness_search_t, module_registry_t, module_registry_t, module_registry_t>;
    using querier_t = pud_eager_goal_querier<replayer_t, module_registry_t, specializer_t>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    copy_demoduler_t copy_demoduler{exprs};
    bind_map_factory_t bind_maps;
    cycle_detector_factory_t cycle_detectors{globalize, copy_demoduler};
    pud_identity_global_functor_ids global_functor_ids;
    unifier_factory_t unifiers{globalize, exprs, global_functor_ids};
    std::deque<pud_module> modules;
    module_registry_t registry;
    replayer_t replayer{registry, registry, exprs, bind_maps, cycle_detectors, unifiers};
    mhws_t mhws{replayer, registry, registry, registry};
    recorder_t recorder{mhws};
    mhcs_t mhcs{replayer, recorder, recorder, recorder, recorder, recorder, registry, registry, registry};
    candidate_witness_index_t candidate_witnesses{mhcs, recorder};
    unit_queries_t unit_queries;
    mhq_t mhq{
        replayer, registry, candidate_witnesses, candidate_witnesses, candidate_witnesses, candidate_witnesses,
        unit_queries};
    witness_search_t witness_search{replayer, registry, registry, registry};
    specializer_t specializer{replayer, witness_search, witness_search, registry, registry, registry};
    querier_t querier{replayer, registry, specializer};
    size_t compared_goals = 0;
    size_t specialized_candidates = 0;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    uint32_t lvc_of(pud_global_node_id node) const {
        const std::optional<pud_global_node_id> parent = registry.get_parent(node);
        if (!parent.has_value())
            return registry.get_axiom(node).var_count;
        return lvc_of(parent.value()) + registry.get_inference(node).var_count;
    }

    // MHCS doesn't expose which of its two witness slots holds which search, so witnesses compare as a set.
    static pud_eager_candidate sorted_witnesses(pud_eager_candidate candidate) {
        std::sort(candidate.witness_leaves.begin(), candidate.witness_leaves.end());
        return candidate;
    }

    std::vector<pud_eager_candidate> reactive_candidates(
        pud_module_id module, const pud_ubs_state& bindings, const expr* goal, uint32_t lvc) {
        std::vector<pud_eager_candidate> found;
        const std::optional<pud_mhq_head_id> head = mhq.try_open(module, bindings, goal, lvc);
        if (!head.has_value())
            return found;
        for (const pud_mhcs_head_id candidate : mhq.candidates(head.value())) {
            pud_eager_candidate admitted{.path = mhcs.path(candidate), .witness_leaves = {}};
            // The index lists every search admission added; those MHWS removed as the candidate moved onto a
            // leaf have no witness, and a candidate left with none is its own witness.
            for (const pud_mhws_head_id witness : candidate_witnesses.find(candidate)->witnesses) {
                const std::optional<pud_global_node_id> leaf = mhws.find_witness(witness);
                if (leaf.has_value())
                    admitted.witness_leaves.push_back(leaf.value());
            }
            if (admitted.witness_leaves.empty())
                admitted.witness_leaves.push_back(admitted.path.back());
            found.push_back(sorted_witnesses(std::move(admitted)));
        }
        return found;
    }

    void compare_live(const pud_runtime<pud_module, expr_pool>& runtime, const std::set<pud_global_node_id>& live) {
        for (const pud_global_node_id leaf : live) {
            pud_unnormalized_rule_information information = runtime.get_rule_information(leaf);
            const pud_ubs_state bindings = std::move(information.bindings).persist();
            const uint32_t lvc = lvc_of(leaf);
            for (const auto& [goal, goal_expr] : information.body) {
                std::vector<pud_eager_candidate> eager = querier.query(leaf.module, bindings, goal_expr, lvc);
                for (pud_eager_candidate& candidate : eager)
                    candidate = sorted_witnesses(std::move(candidate));
                EXPECT_EQ(eager, reactive_candidates(leaf.module, bindings, goal_expr, lvc))
                    << "leaf " << leaf.node << " goal " << goal;
                ++compared_goals;
                for (const pud_eager_candidate& candidate : eager)
                    specialized_candidates += candidate.path.size() > 1 ? 1 : 0;
            }
        }
    }

    // Grows the modules by random unfolds through the manual runtime, comparing every open goal of every live leaf
    // after each one.
    void grow_and_compare(uint32_t seed, size_t unfolds) {
        pud_runtime<pud_module, expr_pool> runtime(exprs);
        std::set<pud_global_node_id> live;
        for (pud_module& module : modules) {
            install_shared_functor_names(module, functors);
            registry.register_module(module);
            const pud_frontier_change change = runtime.register_module(module);
            live.insert(change.offspring.begin(), change.offspring.end());
            for (const pud_global_node_id refuted : change.refuted)
                live.erase(refuted);
        }
        std::mt19937 rng(seed);
        for (size_t step = 0; step < unfolds; ++step) {
            compare_live(runtime, live);
            std::vector<pud_global_node_id> unfoldable;
            for (const pud_global_node_id leaf : live)
                if (!runtime.get_rule_information(leaf).body.empty())
                    unfoldable.push_back(leaf);
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goals;
            for (const auto& [goal, goal_expr] : runtime.get_rule_information(leaf).body)
                goals.push_back(goal);
            const pud_frontier_change change = runtime.unfold(leaf, goals.at(rng() % goals.size()));
            live.erase(leaf);
            live.insert(change.offspring.begin(), change.offspring.end());
            for (const pud_global_node_id refuted : change.refuted)
                live.erase(refuted);
        }
    }
};

TEST_F(PudEagerGoalQuerierIntegrationTest, EagerCandidatesMatchTheReactiveStackOnPeanoArithmetic) {
    modules.emplace_back(2001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(functor("nat", {atom("z")}), {}, 0),
        rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        rule(functor("add", {atom("z"), var(0), var(0)}), {}, 1),
        rule(
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3),
    });
    modules.emplace_back(2002, std::vector<pud_module_id>{2001}, std::vector<rule>{
        rule(
            functor("half", {var(0)}),
            {functor("add", {var(0), var(0), functor("s", {functor("s", {functor("s", {functor("s", {atom("z")})})})})})},
            1),
        rule(functor("number", {var(0)}), {functor("nat", {var(0)})}, 1),
        rule(functor("pair", {var(0), var(1)}), {functor("nat", {var(0)}), functor("add", {var(0), var(0), var(1)})}, 2),
    });
    grow_and_compare(3, 60);
    EXPECT_GT(compared_goals, 20u);
    EXPECT_GT(specialized_candidates, 0u);
}

TEST_F(PudEagerGoalQuerierIntegrationTest, EagerCandidatesMatchTheReactiveStackOnGraphReachability) {
    modules.emplace_back(4001, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(functor("edge", {atom("a"), atom("b")}), {}, 0),
        rule(functor("edge", {atom("b"), atom("c")}), {}, 0),
        rule(functor("edge", {atom("c"), atom("d")}), {}, 0),
        rule(functor("edge", {atom("b"), atom("e")}), {}, 0),
        rule(functor("path", {var(0), var(1)}), {functor("edge", {var(0), var(1)})}, 2),
        rule(
            functor("path", {var(0), var(2)}),
            {functor("edge", {var(0), var(1)}), functor("path", {var(1), var(2)})},
            3),
    });
    modules.emplace_back(4002, std::vector<pud_module_id>{4001}, std::vector<rule>{
        rule(functor("reach", {var(0)}), {functor("path", {atom("a"), var(0)})}, 1),
        rule(functor("loop", {var(0)}), {functor("path", {var(0), var(0)})}, 1),
    });
    grow_and_compare(6, 40);
    EXPECT_GT(compared_goals, 10u);
    EXPECT_GT(specialized_candidates, 0u);
}
