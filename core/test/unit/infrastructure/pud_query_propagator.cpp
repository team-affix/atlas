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
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Return;
using ::testing::SizeIs;

namespace {

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

const pud_node_id g_dummy_root_id = 0;

struct MockGetAddedSpecializations {
    std::vector<pud_specialization> specs;
    const std::vector<pud_specialization>& get(pud_node_id) const { return specs; }
};

struct MockGetAddedBodyGoals {
    std::vector<const expr*> goals;
    const std::vector<const expr*>& get(pud_node_id) const { return goals; }
};

struct MockGetAddedVarCountLog {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};

struct MockGetAddedVarCount {
    uint32_t get(pud_node_id id) const { return log->get(id); }
    static MockGetAddedVarCountLog* log;
};
MockGetAddedVarCountLog* MockGetAddedVarCount::log = nullptr;

struct MockGetAxiomHead {
    const expr* head;
    const expr* get(pud_node_id) const { return head; }
};

using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;

using test_propagator_t = pud_descender<
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

struct PudQueryPropagatorTest : public ::testing::Test {
    expr_pool pool;
    globalizer globalize;
    expr_pool_ubs_var_factory ubs_var_factory{pool};
    expr_pool_ubs_functor_factory ubs_functor_factory{pool};
    NiceMock<MockNextNodeId> next_node_id;
    NiceMock<MockRefuted> refuted;
    NiceMock<MockGetAddedVarCountLog> var_count_log;
    MockCallSite call_site;
    MockGetAddedSpecializations get_specs;
    MockGetAddedBodyGoals get_body_goals;
    MockGetAddedVarCount get_var_count;
    MockGetAxiomHead get_axiom_head;
    NiceMock<MockStoreAddedSpecializations> store_specs;
    NiceMock<MockStoreAddedBodyGoals> store_goals;
    NiceMock<MockStoreAddedVarCount> store_var_count;
    test_propagator_t propagator{
        next_node_id, ubs_var_factory, ubs_functor_factory, pool, pool, globalize, refuted, call_site,
        get_specs, get_body_goals, get_var_count, get_axiom_head,
        store_specs, store_goals, store_var_count};
    const expr* query_expr;
    const expr* axiom_head;
    const expr* goal_a;
    const expr* goal_b;
    const expr* goal_c;
    const expr* spec_value;
    const expr* atom_a;
    pud_node_id made_id = pud_node_id{99};

    void SetUp() override {
        MockGetAddedVarCount::log = &var_count_log;
        query_expr = pool.make_var(0u);
        axiom_head = pool.make_var(0u);
        get_axiom_head.head = axiom_head;
        goal_a = pool.make_var(10u);
        goal_b = pool.make_var(11u);
        goal_c = pool.make_var(12u);
        spec_value = pool.make_functor(1u, {});
        atom_a = pool.make_functor(2u, {});
        ON_CALL(var_count_log, get(_)).WillByDefault(Return(0u));
        ON_CALL(var_count_log, get(g_dummy_root_id)).WillByDefault(Return(1u));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(next_node_id, next()).WillByDefault(Return(made_id));
    }
};

TEST_F(PudQueryPropagatorTest, OpenQueryFromRootUnifiesQueryWithAxiomHead) {
    EXPECT_TRUE(propagator.open_query(propagator.descent_root(g_dummy_root_id), query_expr, g_dummy_root_id).has_value());
}

TEST_F(PudQueryPropagatorTest, OpenQueryDescentIsAtAxiomRootWithOnlyTheAxiomsVars) {
    auto root = propagator.descent_root(g_dummy_root_id);
    auto query = propagator.open_query(root, query_expr, g_dummy_root_id).value();
    EXPECT_EQ(query.node, g_dummy_root_id);
    EXPECT_EQ(query.frame_offset, root.frame_offset + root.lvc);
    EXPECT_EQ(query.lvc, 1u);
}

TEST_F(PudQueryPropagatorTest, OpenQueryThatFailsToUnifyWithAxiomHeadYieldsNullopt) {
    get_axiom_head.head = pool.make_functor(99u, {});
    const expr* ground_query = pool.make_functor(1u, {});
    EXPECT_FALSE(propagator.open_query(propagator.descent_root(g_dummy_root_id), ground_query, g_dummy_root_id).has_value());
}

TEST_F(PudQueryPropagatorTest, OpenQueryOnlyRecordsCallerRepsThatUnifyTouched) {
    get_axiom_head.head = pool.make_functor(1u, {});
    auto root = propagator.descent_root(g_dummy_root_id);
    auto query = propagator.open_query(root, query_expr, g_dummy_root_id).value();
    EXPECT_CALL(store_specs, store(made_id, SizeIs(1)));
    propagator.close_query(query);
}

TEST_F(PudQueryPropagatorTest, RefutedChildIsNotEntered) {
    pud_node_id child = pud_node_id{5};
    EXPECT_CALL(refuted, check_refuted(child)).WillOnce(Return(true));
    auto root = propagator.descent_root(g_dummy_root_id);
    EXPECT_FALSE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, ChildWithNoSpecializationsIsEntered) {
    pud_node_id child = pud_node_id{5};
    auto root = propagator.descent_root(g_dummy_root_id);
    EXPECT_TRUE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, OneSpecializationSucceeds) {
    pud_node_id child = pud_node_id{5};
    get_specs.specs = {{.var_idx = 0u, .value = spec_value}};
    auto root = propagator.descent_root(g_dummy_root_id);
    EXPECT_TRUE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, SeveralSpecializationsSucceed) {
    pud_node_id child = pud_node_id{5};
    get_specs.specs = {
        {.var_idx = 0u, .value = spec_value},
        {.var_idx = 0u, .value = spec_value}};
    auto root = propagator.descent_root(g_dummy_root_id);
    EXPECT_TRUE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, FailingSpecializationIsNotEntered) {
    pud_node_id child = pud_node_id{5};
    const expr* cyclic = pool.make_functor(3u, {pool.make_var(0u)});
    get_specs.specs = {
        {.var_idx = 0u, .value = spec_value},
        {.var_idx = 0u, .value = cyclic}};
    auto root = propagator.descent_root(g_dummy_root_id);
    EXPECT_FALSE(propagator.descend(root, child).has_value());
}

TEST_F(PudQueryPropagatorTest, BodyGoalCountDoesNotRejectChild) {
    pud_node_id none = pud_node_id{5};
    pud_node_id one = pud_node_id{6};
    pud_node_id several = pud_node_id{7};
    auto root = propagator.descent_root(g_dummy_root_id);
    EXPECT_TRUE(propagator.descend(root, none).has_value());
    get_body_goals.goals = {goal_a};
    EXPECT_TRUE(propagator.descend(root, one).has_value());
    get_body_goals.goals = {goal_a, goal_b, goal_c};
    EXPECT_TRUE(propagator.descend(root, several).has_value());
}

TEST_F(PudQueryPropagatorTest, OpenQueryAfterPropagateUsesCallerFramePlusLiveVars) {
    pud_node_id child = pud_node_id{5};
    ON_CALL(var_count_log, get(child)).WillByDefault(Return(4u));
    auto root = propagator.descent_root(g_dummy_root_id);
    auto at_child = propagator.descend(root, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_TRUE(propagator.open_query(*at_child, query_expr, g_dummy_root_id).has_value());
}

TEST_F(PudQueryPropagatorTest, OpenQueryDescentCanBePropagated) {
    pud_node_id child = pud_node_id{5};
    auto root = propagator.descent_root(g_dummy_root_id);
    auto query = propagator.open_query(root, query_expr, g_dummy_root_id).value();
    EXPECT_TRUE(propagator.descend(query, child).has_value());
}

TEST_F(PudQueryPropagatorTest, CloseNeverPropagatedRootHasNoGoalsOrSpecs) {
    EXPECT_CALL(store_specs, store(made_id, IsEmpty()));
    EXPECT_CALL(store_goals, store(made_id, IsEmpty()));
    EXPECT_CALL(store_var_count, store(made_id, 0u));
    auto root = propagator.descent_root(g_dummy_root_id);
    propagator.close_query(root);
}

TEST_F(PudQueryPropagatorTest, CloseOpenQueryWithoutPropagateHasNoGoalsOrSpecs) {
    EXPECT_CALL(store_specs, store(made_id, IsEmpty()));
    EXPECT_CALL(store_goals, store(made_id, IsEmpty()));
    EXPECT_CALL(store_var_count, store(made_id, 0u));
    auto root = propagator.descent_root(g_dummy_root_id);
    auto query = propagator.open_query(root, query_expr, g_dummy_root_id).value();
    propagator.close_query(query);
}

TEST_F(PudQueryPropagatorTest, CloseOneStepReceivesThatStepsGoalsAndReps) {
    get_axiom_head.head = pool.make_functor(1u, {});
    auto root = propagator.descent_root(g_dummy_root_id);
    auto query = propagator.open_query(root, query_expr, g_dummy_root_id).value();
    get_body_goals.goals = {goal_a};
    pud_node_id step = pud_node_id{5};
    auto at_step = propagator.descend(query, step);
    ASSERT_TRUE(at_step.has_value());
    EXPECT_CALL(store_specs, store(made_id, Not(IsEmpty())));
    EXPECT_CALL(store_goals, store(made_id, Not(IsEmpty())));
    propagator.close_query(*at_step);
}

TEST_F(PudQueryPropagatorTest, CloseVarCountZeroWhenNothingIsNormalizedIntoANewVar) {
    pud_node_id step = pud_node_id{5};
    get_body_goals.goals = {spec_value};
    get_specs.specs.clear();
    EXPECT_CALL(store_var_count, store(made_id, 0u));
    auto root = propagator.descent_root(g_dummy_root_id);
    auto at_step = propagator.descend(root, step);
    ASSERT_TRUE(at_step.has_value());
    EXPECT_EQ(propagator.close_query(*at_step), made_id);
}
