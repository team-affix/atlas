#include <cstdint>
#include <deque>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "pud_module_serde_fixture.hpp"
#include "reference_pud_frontier.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/rule.hpp"

namespace {

struct PudModuleInsertExtractIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool encode_pool;
    expr_pool decode_pool;
    pud_module_serde_wiring encode_wiring;
    pud_module_serde_decode_wiring<expr_pool, expr_pool> decode_wiring{decode_pool, decode_pool};

    const expr* var(uint32_t index) { return encode_pool.make_var(index); }
    const expr* atom(const char* name) { return encode_pool.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return encode_pool.make_functor(functors.id(name), std::move(args));
    }

    std::string serialize(const pud_module& module) {
        std::ostringstream out;
        encode_wiring.insert_module_.insert_module(out, module);
        return out.str();
    }

    pud_module deserialize(const std::string& blob) {
        std::istringstream in(blob);
        return decode_wiring.extract_module_.extract_module(in);
    }

    pud_module round_trip(const pud_module& original) {
        return deserialize(serialize(original));
    }

    void expect_same(const pud_module& original, const pud_module& loaded) {
        EXPECT_EQ(original.get_module_id(), loaded.get_module_id());
        EXPECT_EQ(original.get_dependencies(), loaded.get_dependencies());
        EXPECT_EQ(original.count_axioms(), loaded.count_axioms());
        EXPECT_EQ(original.count_nodes(), loaded.count_nodes());
        for (pud_node_id node = 0; node < original.count_nodes(); ++node) {
            EXPECT_EQ(original.get_children(node), loaded.get_children(node));
            EXPECT_EQ(original.check_refuted(node), loaded.check_refuted(node));
        }
        EXPECT_EQ(serialize(original), serialize(loaded));
    }
};

}

TEST_F(PudModuleInsertExtractIntegrationTest, EmptyAxiomsModuleRoundTrips) {
    const pud_module original = make_empty_pud_module(1);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, FactsOnlyModuleRoundTrips) {
    const pud_module original = make_facts_pud_module(10, encode_pool, functors);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, ModuleWithDependenciesRoundTrips) {
    pud_module original{3, {9, 1}, {rule(atom("q"), {atom("g")}, 0)}};
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, LinearChainOfInferencesRoundTrips) {
    const pud_module original = make_chain_pud_module(4, encode_pool, functors, 5);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, SiblingOrderRoundTrips) {
    pud_module original{1, {}, {rule(atom("h"), {atom("g")}, 0)}};
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {atom("a")},
        .var_count = 0,
    });
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {atom("b")},
        .var_count = 0,
    });
    const pud_module loaded = round_trip(original);
    expect_same(original, loaded);
    EXPECT_EQ(loaded.get_children(0), original.get_children(0));
}

TEST_F(PudModuleInsertExtractIntegrationTest, BranchingTreeRoundTrips) {
    const pud_module original = make_branchy_pud_module(8, encode_pool, functors);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, InferenceUnderSecondAxiomRoundTrips) {
    pud_module original{1, {}, {rule(atom("a"), {}, 0), rule(atom("b"), {atom("g")}, 0)}};
    original.store_inference(pud_inference{
        .parent = 1,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, SubstitutionsWithVarAndFunctorValuesRoundTrip) {
    pud_module original{1, {}, {rule(functor("h", {var(0)}), {atom("g")}, 1)}};
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions =
            {
                pud_substitution{.var_idx = 0, .value = var(3)},
                pud_substitution{.var_idx = 1, .value = atom("v")},
            },
        .body_goals = {},
        .var_count = 2,
    });
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, MultipleSubstitutionsPerInferenceRoundTrip) {
    pud_module original{1, {}, {rule(atom("h"), {}, 0)}};
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions =
            {
                pud_substitution{.var_idx = 0, .value = atom("a")},
                pud_substitution{.var_idx = 2, .value = atom("b")},
                pud_substitution{.var_idx = 4, .value = atom("c")},
            },
        .body_goals = {},
        .var_count = 5,
    });
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, MultipleBodyGoalsRoundTrip) {
    pud_module original{1, {}, {rule(atom("h"), {atom("g")}, 0)}};
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {atom("a"), atom("b"), atom("c")},
        .var_count = 0,
    });
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, EmptyBodyGoalsAndZeroSubstitutionsRoundTrip) {
    pud_module original{1, {}, {rule(atom("h"), {}, 0)}};
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, NonZeroCallSiteAndVarCountRoundTrip) {
    pud_module original{1, {}, {rule(atom("h"), {atom("g"), atom("k")}, 0)}};
    original.store_inference(pud_inference{
        .parent = 0,
        .call_site = 1,
        .substitutions = {},
        .body_goals = {atom("n")},
        .var_count = 6,
    });
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, DeepNestedExprInAxiomRoundTrips) {
    const expr* deep = functor("f", {functor("g", {functor("h", {atom("z")})})});
    pud_module original{1, {}, {rule(deep, {deep}, 0)}};
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, SharedSubexprInRuleRoundTrips) {
    const expr* shared = atom("g");
    pud_module original{1, {}, {rule(atom("h"), {shared, shared}, 0)}};
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, HighVarIndicesRoundTrip) {
    pud_module original{1, {}, {rule(functor("h", {var(std::numeric_limits<uint32_t>::max())}), {}, 1)}};
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, UnrefutedModuleRoundTrips) {
    const pud_module original = make_branchy_pud_module(2, encode_pool, functors);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, SingleRefutedLeafRoundTrips) {
    pud_module original = make_branchy_pud_module(2, encode_pool, functors);
    original.mark_refuted(4);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, RefutedAxiomRoundTrips) {
    pud_module original = make_facts_pud_module(10, encode_pool, functors);
    original.mark_refuted(1);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, RefutedInteriorNodeRoundTrips) {
    pud_module original = make_branchy_pud_module(2, encode_pool, functors);
    original.mark_refuted(2);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, AllNodesRefutedRoundTrips) {
    pud_module original = make_chain_pud_module(1, encode_pool, functors, 3);
    for (pud_node_id node = 0; node < original.count_nodes(); ++node)
        original.mark_refuted(node);
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, RoundTripUsesDistinctExprPools) {
    const pud_module original = make_facts_pud_module(10, encode_pool, functors);
    const pud_module loaded = round_trip(original);
    expect_same(original, loaded);
    EXPECT_NE(&original.get_axiom(0).head->content, &loaded.get_axiom(0).head->content);
}

TEST_F(PudModuleInsertExtractIntegrationTest, TwoModulesSerializedIndependentlyRoundTrip) {
    const pud_module first = make_facts_pud_module(10, encode_pool, functors);
    const pud_module second = make_chain_pud_module(20, encode_pool, functors, 2);
    expect_same(first, deserialize(serialize(first)));
    expect_same(second, deserialize(serialize(second)));
}

TEST_F(PudModuleInsertExtractIntegrationTest, SerializeDeserializeSerializeProducesIdenticalBytes) {
    const pud_module original = make_branchy_pud_module(8, encode_pool, functors);
    const std::string first = serialize(original);
    const std::string second = serialize(deserialize(first));
    EXPECT_EQ(first, second);
}

TEST_F(PudModuleInsertExtractIntegrationTest, RoundTrippedModuleRegistersSameFrontierAsOriginal) {
    pud_module original{
        1,
        {},
        {
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        },
    };
    original.store_inference(pud_inference{
        .parent = 1,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = atom("z")}},
        .body_goals = {functor("nat", {atom("z")})},
        .var_count = 0,
    });
    pud_module loaded = round_trip(original);
    pud_runtime<pud_module, expr_pool> original_runtime{encode_pool};
    pud_runtime<pud_module, expr_pool> loaded_runtime{decode_pool};
    const pud_frontier_change original_change = original_runtime.register_module(original);
    const pud_frontier_change loaded_change = loaded_runtime.register_module(loaded);
    EXPECT_EQ(original_change.offspring, loaded_change.offspring);
    EXPECT_EQ(original_change.refuted, loaded_change.refuted);
    std::deque<pud_module> original_modules;
    original_modules.push_back(original);
    std::deque<pud_module> loaded_modules;
    loaded_modules.push_back(loaded);
    EXPECT_EQ(reference_pud_frontier(original_modules), reference_pud_frontier(loaded_modules));
}

TEST_F(PudModuleInsertExtractIntegrationTest, RoundTrippedModuleAllowsSameUnfoldAsBefore) {
    pud_module original{
        1,
        {},
        {
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        },
    };
    pud_module loaded = round_trip(original);
    pud_runtime<pud_module, expr_pool> original_runtime{encode_pool};
    pud_runtime<pud_module, expr_pool> loaded_runtime{decode_pool};
    original_runtime.register_module(original);
    loaded_runtime.register_module(loaded);
    const pud_global_node_id step{.module = 1, .node = 1};
    const pud_frontier_change original_unfold = original_runtime.unfold(step, 0);
    const pud_frontier_change loaded_unfold = loaded_runtime.unfold(step, 0);
    EXPECT_EQ(original_unfold.offspring, loaded_unfold.offspring);
    EXPECT_EQ(original_unfold.refuted, loaded_unfold.refuted);
    expect_same(original, loaded);
}

TEST_F(PudModuleInsertExtractIntegrationTest, ManySiblingsOneParentRoundTrip) {
    pud_module original{1, {}, {rule(atom("h"), {atom("g")}, 0)}};
    for (uint32_t sibling = 0; sibling < 12; ++sibling) {
        original.store_inference(pud_inference{
            .parent = 0,
            .call_site = 0,
            .substitutions = {},
            .body_goals = {atom("g")},
            .var_count = 0,
        });
    }
    expect_same(original, round_trip(original));
}

TEST_F(PudModuleInsertExtractIntegrationTest, ManyAxiomsRoundTrip) {
    std::vector<rule> axioms;
    for (uint32_t axiom = 0; axiom < 12; ++axiom)
        axioms.push_back(rule(functor("p", {var(axiom)}), {}, axiom + 1));
    pud_module original{1, {}, std::move(axioms)};
    expect_same(original, round_trip(original));
}
