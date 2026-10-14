#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/ubs_transient_bind_map_data.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};

struct MockRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};

struct MockNextNodeId {
    MOCK_METHOD(pud_node_id, next, ());
};

struct MockStoreAddedSpecializations {
    MOCK_METHOD(void, store, (pud_node_id, (std::vector<pud_specialization>)));
};

struct MockStoreAddedBodyGoals {
    MOCK_METHOD(void, store, (pud_node_id, (std::vector<const expr*>)));
};

struct MockStoreAddedVarCount {
    MOCK_METHOD(void, store, (pud_node_id, uint32_t));
};

struct MockCallSite {
    size_t get(pud_node_id) const { return std::numeric_limits<size_t>::max(); }
};

const pud_node_id g_spec_dummy_root_id = pud_node_id{0};

struct MockGetAddedSpecializations {
    MOCK_METHOD((const std::vector<pud_specialization>&), get, (pud_node_id));
};

struct MockGetAddedBodyGoals {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

struct MockGetAddedVarCount {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};

struct MockGetAxiomHead {
    MOCK_METHOD(const expr*, get, (pud_node_id));
};

using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;

using propagator_t = pud_descender<
    bind_map_t,
    expr_pool_ubs_var_factory,
    expr_pool_ubs_functor_factory,
    MockNextNodeId,
    expr_pool,
    expr_pool,
    globalizer,
    MockRefuted,
    MockCallSite,
    MockGetAddedSpecializations,
    MockGetAddedBodyGoals,
    MockGetAddedVarCount,
    MockGetAxiomHead,
    MockStoreAddedSpecializations,
    MockStoreAddedBodyGoals,
    MockStoreAddedVarCount>;

} // namespace

struct PudPropagateSpecializeIntegrationTest : public ::testing::Test {
    expr_pool pool;
    globalizer globalize;
    expr_pool_ubs_var_factory ubs_var_factory{pool};
    expr_pool_ubs_functor_factory ubs_functor_factory{pool};
    NiceMock<MockNextNodeId> next_node_id;
    NiceMock<MockRefuted> refuted;
    MockCallSite call_site;
    NiceMock<MockGetAddedSpecializations> get_added_specs;
    NiceMock<MockGetAddedBodyGoals> get_added_body_goals;
    NiceMock<MockGetAddedVarCount> get_added_var_count;
    NiceMock<MockGetAxiomHead> get_axiom_head;
    NiceMock<MockStoreAddedSpecializations> store_specs;
    NiceMock<MockStoreAddedBodyGoals> store_goals;
    NiceMock<MockStoreAddedVarCount> store_var_count;

    propagator_t propagator{
        next_node_id, ubs_var_factory, ubs_functor_factory, pool, pool, globalize, refuted, call_site,
        get_added_specs, get_added_body_goals, get_added_var_count, get_axiom_head,
        store_specs, store_goals, store_var_count};

    const expr* var_expr;
    const expr* spec_value;
    const expr* atom_head;
    pud_node_id made_id = pud_node_id{100};

    static const std::vector<pud_specialization> empty_spec_vec;
    static const std::vector<const expr*> empty_goal_vec;

    const pud_node_id child_id = pud_node_id{1};
    const pud_node_id with_live_vars_id = pud_node_id{2};

    std::vector<pud_specialization> child_spec_var0_only{
        pud_specialization{.var_idx = 0u, .value = nullptr}};
    std::vector<pud_specialization> child_spec_var1_only{
        pud_specialization{.var_idx = 1u, .value = nullptr}};

    void SetUp() override {
        var_expr = pool.make_var(0u);
        spec_value = pool.make_functor(10u, {});
        atom_head = pool.make_functor(11u, {});
        child_spec_var0_only[0].value = spec_value;
        child_spec_var1_only[0].value = spec_value;
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(next_node_id, next()).WillByDefault(Return(made_id));
        ON_CALL(get_added_var_count, get(_)).WillByDefault(Return(0u));
        ON_CALL(get_added_specs, get(_)).WillByDefault(ReturnRef(empty_spec_vec));
        ON_CALL(get_added_body_goals, get(_)).WillByDefault(ReturnRef(empty_goal_vec));
        ON_CALL(get_axiom_head, get(g_spec_dummy_root_id)).WillByDefault(Return(var_expr));
    }
};

const std::vector<pud_specialization> PudPropagateSpecializeIntegrationTest::empty_spec_vec;
const std::vector<const expr*> PudPropagateSpecializeIntegrationTest::empty_goal_vec;

TEST_F(PudPropagateSpecializeIntegrationTest, RepAtTheFrameOffsetIsNotATouchedCallerRep) {
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var0_only));
    auto root = propagator.descent_root(g_spec_dummy_root_id);
    auto at_child = propagator.descend(root, child_id);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(store_specs, store(made_id, IsEmpty()));
    EXPECT_CALL(store_goals, store(made_id, IsEmpty()));
    propagator.close_query(*at_child);
}

TEST_F(PudPropagateSpecializeIntegrationTest, UnifyFailureIsNotEntered) {
    const expr* cyclic = pool.make_functor(1u, {pool.make_var(0u)});
    child_spec_var0_only[0].value = cyclic;
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var0_only));
    auto root = propagator.descent_root(g_spec_dummy_root_id);
    EXPECT_FALSE(propagator.descend(root, child_id).has_value());
}

TEST_F(PudPropagateSpecializeIntegrationTest, CloseOfNeverPropagatedDescentHasNoBodyGoals) {
    EXPECT_CALL(store_specs, store(made_id, IsEmpty()));
    EXPECT_CALL(store_goals, store(made_id, IsEmpty()));
    EXPECT_CALL(store_var_count, store(made_id, 0u));
    EXPECT_EQ(propagator.close_query(propagator.descent_root(g_spec_dummy_root_id)), made_id);
}

TEST_F(PudPropagateSpecializeIntegrationTest, RepBelowTheOffsetIsTheChildsTouchedCallerRep) {
    ON_CALL(get_added_var_count, get(with_live_vars_id)).WillByDefault(Return(5u));
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var1_only));
    ON_CALL(get_axiom_head, get(g_spec_dummy_root_id)).WillByDefault(Return(atom_head));
    auto root = propagator.descent_root(g_spec_dummy_root_id);
    auto at_live = propagator.descend(root, with_live_vars_id);
    ASSERT_TRUE(at_live.has_value());
    const expr* caller_var = pool.make_var(3u);
    auto opened = propagator.open_query(*at_live, caller_var, g_spec_dummy_root_id);
    ASSERT_TRUE(opened.has_value());
    auto at_child = propagator.descend(*opened, child_id);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(store_specs, store(made_id, ElementsAre(Field(&pud_specialization::var_idx, 3u))));
    propagator.close_query(*at_child);
}
