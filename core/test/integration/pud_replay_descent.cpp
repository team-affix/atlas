#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_replay_bind_factories.hpp"
#include "infrastructure/pud_replay_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_specialization.hpp"

struct PudReplayDescentIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;
    using bind_factories_t = pud_replay_bind_factories<bind_map_t, globalizer, expr_pool_ubs_functor_factory>;
    using binding_descent_t = pud_binding_descent<
        bind_map_t,
        pud_node_added_specializations,
        expr_pool_ubs_var_factory,
        expr_pool_ubs_functor_factory>;
    using replay_descent_t = pud_replay_descent<
        bind_map_t,
        bind_factories_t::normalizer_t,
        bind_factories_t::unifier_t,
        expr_pool_ubs_var_factory,
        expr_pool_ubs_functor_factory,
        expr_pool,
        bind_factories_t,
        bind_factories_t,
        pud_node_added_specializations>;

    test_functors                  functors;
    expr_pool                      exprs;
    globalizer                     globalize;
    expr_pool_ubs_var_factory      ubs_var_factory_{exprs};
    expr_pool_ubs_functor_factory  ubs_functor_factory_{exprs};
    bind_factories_t               bind_factories_{globalize, ubs_functor_factory_};
    pud_node_added_specializations node_specs_;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    std::optional<replay_descent_t> open(
        const pud_ubs_state& caller_state,
        const expr* query_expr,
        const expr* axiom_head,
        uint32_t frame_offset) {
        pud_transient_ubs_state transient{caller_state};
        bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
        bind_factories_t::normalizer_t normalizer = bind_factories_.make_normalizer(bind_map);
        bind_factories_t::unifier_t unifier = bind_factories_.make_unifier(bind_map, normalizer);
        if (!unifier.unify(framed_expr{query_expr, 0u}, framed_expr{axiom_head, frame_offset}))
            return std::nullopt;
        return replay_descent_t{
            ubs_var_factory_,
            ubs_functor_factory_,
            exprs,
            bind_factories_,
            bind_factories_,
            node_specs_,
            transient.to_persistent_state(),
            frame_offset};
    }

    const expr* resolve_global(const replay_descent_t& descent, uint32_t global_var_index) {
        pud_transient_ubs_state transient{descent.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
        return bind_map.resolve(global_var_index);
    }

    const expr* resolve_at_frame(const replay_descent_t& descent, uint32_t frame_offset, uint32_t local_var_index) {
        return resolve_global(descent, globalize.globalize(frame_offset, local_var_index));
    }

    void expect_unbound_global(const replay_descent_t& descent, uint32_t global_var_index) {
        const expr* resolved = resolve_global(descent, global_var_index);
        const expr::var* rep_var = std::get_if<expr::var>(&resolved->content);
        ASSERT_NE(rep_var, nullptr);
        EXPECT_EQ(rep_var->index, global_var_index);
    }
};

TEST_F(PudReplayDescentIntegrationTest, SpecsReplayLiftedToFrameOffset) {
    const pud_node_id child = 1;
    const uint32_t frame_offset = 3;
    node_specs_.store(child, {
        pud_specialization{.var_idx = 0, .value = functor("a", {})},
        pud_specialization{.var_idx = 1, .value = functor("b", {})},
    });
    const expr* head = functor("main", {var(0), var(1), var(2)});
    std::optional<replay_descent_t> opened = open({}, head, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 1), functor("b", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 1), functor("b", {}));
}

TEST_F(PudReplayDescentIntegrationTest, DifferentFrameOffsetsStayIndependent) {
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    const expr* head = var(0);
    std::optional<replay_descent_t> at_one = open({}, var(0), head, 1u);
    std::optional<replay_descent_t> at_five = open({}, var(0), head, 5u);
    ASSERT_TRUE(at_one.has_value());
    ASSERT_TRUE(at_five.has_value());
    std::optional<replay_descent_t> child_one = at_one->descend(child);
    std::optional<replay_descent_t> child_five = at_five->descend(child);
    ASSERT_TRUE(child_one.has_value());
    ASSERT_TRUE(child_five.has_value());
    EXPECT_EQ(resolve_global(*child_one, 1u), functor("a", {}));
    EXPECT_EQ(resolve_global(*child_five, 5u), functor("a", {}));
    expect_unbound_global(*child_one, 5u);
    expect_unbound_global(*child_five, 1u);
}

TEST_F(PudReplayDescentIntegrationTest, OpeningFlowFromBindingDescentState) {
    const pud_node_id binding_child = 11;
    const pud_node_id agree_child = 1;
    const pud_node_id contradict_child = 2;
    const uint32_t frame_offset = 1u;

    node_specs_.store(binding_child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    binding_descent_t caller_binding =
        binding_descent_t{node_specs_, ubs_var_factory_, ubs_functor_factory_}.descend(binding_child);

    std::optional<replay_descent_t> opened = open(caller_binding.ubs_state(), var(0), var(0), frame_offset);
    ASSERT_TRUE(opened.has_value());

    node_specs_.store(agree_child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    EXPECT_TRUE(opened->descend(agree_child).has_value());

    node_specs_.store(contradict_child, {pud_specialization{.var_idx = 0, .value = functor("b", {})}});
    EXPECT_FALSE(opened->descend(contradict_child).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, ContradictsInheritedCallerBindingRefutes) {
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("b", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("a", {}), var(0), 1);
    ASSERT_TRUE(opened.has_value());
    EXPECT_FALSE(opened->descend(child).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, ContradictsEarlierReplayedSpecRefutes) {
    const pud_node_id good_child = 1;
    const pud_node_id bad_child = 2;
    node_specs_.store(good_child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    node_specs_.store(bad_child, {pud_specialization{.var_idx = 0, .value = functor("x", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("main", {}), functor("main", {}), 0u);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_good = opened->descend(good_child);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_FALSE(at_good->descend(bad_child).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, ManySpecsReplay) {
    const uint32_t spec_count = 200;
    std::vector<pud_specialization> specs;
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        specs.push_back(pud_specialization{.var_idx = spec_index, .value = functor(label.c_str(), {})});
    }
    const pud_node_id child = 1;
    node_specs_.store(child, std::move(specs));
    std::optional<replay_descent_t> opened = open({}, functor("main", {}), functor("main", {}), 0u);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        EXPECT_EQ(resolve_at_frame(*at_child, 0u, spec_index), functor(label.c_str(), {}));
    }
}

TEST_F(PudReplayDescentIntegrationTest, ForkedDescentsFromSharedPrefixStayIndependent) {
    const pud_node_id bob_child = 1;
    const pud_node_id x_child = 2;
    const uint32_t frame_offset = 3;
    node_specs_.store(bob_child, {
        pud_specialization{.var_idx = 0, .value = functor("tom", {})},
        pud_specialization{.var_idx = 1, .value = functor("bob", {})},
    });
    node_specs_.store(x_child, {
        pud_specialization{.var_idx = 0, .value = functor("tom", {})},
        pud_specialization{.var_idx = 1, .value = functor("x", {})},
    });
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    std::optional<replay_descent_t> opened = open({}, query, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_bob = opened->descend(bob_child);
    std::optional<replay_descent_t> at_x = opened->descend(x_child);
    ASSERT_TRUE(at_bob.has_value());
    ASSERT_TRUE(at_x.has_value());
    EXPECT_EQ(resolve_at_frame(*at_bob, frame_offset, 1), functor("bob", {}));
    EXPECT_EQ(resolve_at_frame(*at_x, frame_offset, 1), functor("x", {}));
}

TEST_F(PudReplayDescentIntegrationTest, CallerVarThroughHeadBindingSeesCalleeFunctor) {
    const pud_node_id child = 1;
    const uint32_t frame_offset = 3;
    node_specs_.store(child, {
        pud_specialization{.var_idx = 0, .value = functor("tom", {})},
        pud_specialization{.var_idx = 1, .value = functor("bob", {})},
    });
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    std::optional<replay_descent_t> opened = open({}, query, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 2), functor("bob", {}));
}

TEST_F(PudReplayDescentIntegrationTest, ParentUnchangedByChild) {
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    std::optional<replay_descent_t> opened = open({}, var(0), var(0), 2u);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 2u, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*opened, 2u, 0), var(0));
}
