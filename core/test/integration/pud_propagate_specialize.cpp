#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/pud_specializer.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

coroutine<uint32_t, bool> scripted_unify(std::vector<uint32_t> reps, bool ok) {
    for (uint32_t rep : reps)
        co_yield rep;
    co_return ok;
}

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

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct UnifyLog {
    MOCK_METHOD((coroutine<uint32_t, bool>), unify, (framed_expr, framed_expr));
};

struct MockBindMap {
    template<typename G>
    MockBindMap(G&, immer::map<uint32_t, framed_expr>::transient_type&) {}
    void bind(uint32_t, framed_expr) {}
    framed_expr whnf(framed_expr value) { return value; }
};

struct MockUnifier {
    MockUnifier(MockGlobalize&, MockBindMap*) {}
    coroutine<uint32_t, bool> unify(framed_expr lhs, framed_expr rhs) {
        return log->unify(lhs, rhs);
    }
    static UnifyLog* log;
};

UnifyLog* MockUnifier::log = nullptr;

struct MockNormalizer {
    template<typename G, typename MF, typename MV, typename BM>
    MockNormalizer(G&, MF&, MV&, BM&) {}
    const expr* normalize(framed_expr, uint32_t, std::unordered_map<uint32_t, uint32_t>&) {
        return nullptr;
    }
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

using propagator_t = pud_descender<
    MockBindMap,
    MockUnifier,
    pud_specializer<MockMakeVar, MockUnifier>,
    MockNormalizer,
    MockNextNodeId,
    MockMakeVar,
    MockGlobalize,
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
    NiceMock<MockNextNodeId>            next_node_id;
    NiceMock<MockMakeVar>               make_var;
    NiceMock<MockGlobalize>             globalize;
    NiceMock<MockRefuted>               refuted;
    NiceMock<UnifyLog>                  unify_log;
    MockCallSite                        call_site;
    NiceMock<MockGetAddedSpecializations> get_added_specs;
    NiceMock<MockGetAddedBodyGoals>     get_added_body_goals;
    NiceMock<MockGetAddedVarCount>      get_added_var_count;
    NiceMock<MockGetAxiomHead>          get_axiom_head;
    NiceMock<MockStoreAddedSpecializations> store_specs;
    NiceMock<MockStoreAddedBodyGoals>       store_goals;
    NiceMock<MockStoreAddedVarCount>        store_var_count;

    propagator_t propagator{next_node_id, make_var, globalize, refuted, call_site,
                             get_added_specs, get_added_body_goals, get_added_var_count, get_axiom_head,
                             store_specs, store_goals, store_var_count};

    expr var_expr{expr::var{0}};
    expr spec_value{expr::var{3}};
    pud_node_id made_id = pud_node_id{100};

    static const std::vector<pud_specialization> empty_spec_vec;
    static const std::vector<const expr*>        empty_goal_vec;

    const pud_node_id child_id          = pud_node_id{1};
    const pud_node_id with_live_vars_id = pud_node_id{2};

    std::vector<pud_specialization> child_spec_var0_only{
        pud_specialization{.var_idx = 0, .value = &spec_value}};
    std::vector<pud_specialization> child_spec_var1_only{
        pud_specialization{.var_idx = 1, .value = &spec_value}};

    void SetUp() override {
        MockUnifier::log = &unify_log;
        ON_CALL(refuted,              check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(make_var,             make_var(_)).WillByDefault(Return(&var_expr));
        ON_CALL(next_node_id,         next()).WillByDefault(Return(made_id));
        ON_CALL(get_added_var_count,  get(_)).WillByDefault(Return(0u));
        ON_CALL(get_added_specs,      get(_)).WillByDefault(ReturnRef(empty_spec_vec));
        ON_CALL(get_added_body_goals, get(_)).WillByDefault(ReturnRef(empty_goal_vec));
        ON_CALL(get_axiom_head,       get(g_spec_dummy_root_id)).WillByDefault(Return(&var_expr));
    }
};

const std::vector<pud_specialization> PudPropagateSpecializeIntegrationTest::empty_spec_vec;
const std::vector<const expr*>        PudPropagateSpecializeIntegrationTest::empty_goal_vec;

TEST_F(PudPropagateSpecializeIntegrationTest, RepAtTheFrameOffsetIsNotATouchedCallerRep) {
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var0_only));
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({0}, true))));
    auto root     = propagator.descent_root(g_spec_dummy_root_id);
    auto at_child = propagator.descend(root, child_id);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(store_specs, store(made_id, IsEmpty()));
    EXPECT_CALL(store_goals, store(made_id, IsEmpty()));
    propagator.close_query(*at_child);
}

TEST_F(PudPropagateSpecializeIntegrationTest, UnifyFailureIsNotEntered) {
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var0_only));
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({}, false))));
    auto root = propagator.descent_root(g_spec_dummy_root_id);
    EXPECT_FALSE(propagator.descend(root, child_id).has_value());
}

TEST_F(PudPropagateSpecializeIntegrationTest, CloseOfNeverPropagatedHandleHasNoBodyGoals) {
    EXPECT_CALL(store_specs,     store(made_id, IsEmpty()));
    EXPECT_CALL(store_goals,     store(made_id, IsEmpty()));
    EXPECT_CALL(store_var_count, store(made_id, 0u));
    EXPECT_EQ(propagator.close_query(propagator.descent_root(g_spec_dummy_root_id)), made_id);
}

TEST_F(PudPropagateSpecializeIntegrationTest, RepBelowTheOffsetIsTheChildsTouchedCallerRep) {
    ON_CALL(get_added_var_count, get(with_live_vars_id)).WillByDefault(Return(5u));
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var1_only));
    // root.lvc=0, with_live_vars.added_var_count=5 → at_live.lvc=5 → query_frame_offset=5
    // First call (open_query): scripted_unify yields rep 3 (< 5, caller rep) and rep 5 (not caller rep).
    // Second call (descend to child_id with spec {var_idx=1}): no reps, just succeeds.
    EXPECT_CALL(unify_log, unify(_, _))
        .WillOnce(Return(::testing::ByMove(scripted_unify({3, 5}, true))))
        .WillOnce(Return(::testing::ByMove(scripted_unify({}, true))));
    auto root         = propagator.descent_root(g_spec_dummy_root_id);
    auto at_live      = propagator.descend(root, with_live_vars_id);
    ASSERT_TRUE(at_live.has_value());
    auto opened       = propagator.open_query(*at_live, &spec_value, g_spec_dummy_root_id).value();
    auto at_child     = propagator.descend(opened, child_id);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(store_specs, store(made_id,
        ElementsAre(Field(&pud_specialization::var_idx, 3u))));
    propagator.close_query(*at_child);
}
