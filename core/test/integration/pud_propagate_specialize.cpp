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

struct MockMakeNode {
    MOCK_METHOD(pud_node_id, make, (std::vector<pud_specialization>, std::vector<const expr*>, uint32_t));
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

struct MockIterateRoots {
    coroutine<pud_node_id, void> iterate_roots() {
        co_yield g_spec_dummy_root_id;
    }
};

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
    MockMakeNode,
    MockMakeVar,
    MockGlobalize,
    MockRefuted,
    MockCallSite,
    MockIterateRoots,
    MockGetAddedSpecializations,
    MockGetAddedBodyGoals,
    MockGetAddedVarCount,
    MockGetAxiomHead>;

} // namespace

struct PudPropagateSpecializeIntegrationTest : public ::testing::Test {
    NiceMock<MockMakeNode>              make_node;
    NiceMock<MockMakeVar>               make_var;
    NiceMock<MockGlobalize>             globalize;
    NiceMock<MockRefuted>               refuted;
    NiceMock<UnifyLog>                  unify_log;
    MockCallSite                        call_site;
    MockIterateRoots                    iter_roots;
    NiceMock<MockGetAddedSpecializations> get_added_specs;
    NiceMock<MockGetAddedBodyGoals>     get_added_body_goals;
    NiceMock<MockGetAddedVarCount>      get_added_var_count;
    NiceMock<MockGetAxiomHead>          get_axiom_head;

    propagator_t propagator{make_node, make_var, globalize, refuted, call_site, iter_roots,
                             get_added_specs, get_added_body_goals, get_added_var_count, get_axiom_head};

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
        ON_CALL(make_node,            make(_, _, _)).WillByDefault(Return(made_id));
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
    auto root     = propagator.descent_roots()[0];
    auto at_child = propagator.descend(root, child_id);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs,
                                                        std::vector<const expr*> goals,
                                                        uint32_t) {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        return made_id;
    });
    propagator.close_query(*at_child);
}

TEST_F(PudPropagateSpecializeIntegrationTest, UnifyFailureIsNotEntered) {
    ON_CALL(get_added_specs, get(child_id)).WillByDefault(ReturnRef(child_spec_var0_only));
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({}, false))));
    auto root = propagator.descent_roots()[0];
    EXPECT_FALSE(propagator.descend(root, child_id).has_value());
}

TEST_F(PudPropagateSpecializeIntegrationTest, CloseOfNeverPropagatedHandleHasNoBodyGoals) {
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs,
                                                        std::vector<const expr*> goals,
                                                        uint32_t vars) {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        EXPECT_EQ(vars, 0u);
        return made_id;
    });
    EXPECT_EQ(propagator.close_query(propagator.descent_roots()[0]), made_id);
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
    auto root         = propagator.descent_roots()[0];
    auto at_live      = propagator.descend(root, with_live_vars_id);
    ASSERT_TRUE(at_live.has_value());
    auto opened       = propagator.open_query(*at_live, &spec_value)[0];
    auto at_child     = propagator.descend(opened, child_id);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs,
                                                        std::vector<const expr*>,
                                                        uint32_t) {
        EXPECT_EQ(specs.size(), 1u);
        if (!specs.empty())
            EXPECT_EQ(specs[0].var_idx, 3u);
        return made_id;
    });
    propagator.close_query(*at_child);
}
