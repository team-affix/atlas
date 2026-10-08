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

namespace {

coroutine<uint32_t, bool> scripted_unify(std::vector<uint32_t> reps, bool ok) {
    for (uint32_t rep : reps)
        co_yield rep;
    co_return ok;
}

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};

struct MockParent {
    MOCK_METHOD(const pud_node*, get, (const pud_node*));
};

struct MockRefuted {
    MOCK_METHOD(bool, check_refuted, (const pud_node*));
};

struct MockMakeNode {
    MOCK_METHOD(const pud_node*, make, (std::vector<pud_specialization>, std::vector<const expr*>, uint32_t));
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
    size_t get(const pud_node*) const { return std::numeric_limits<size_t>::max(); }
};

pud_node g_spec_dummy_root{};

struct MockIterateRoots {
    coroutine<const pud_node*, void> iterate_roots() {
        co_yield &g_spec_dummy_root;
    }
};

using propagator_t = pud_descender<
    MockBindMap,
    MockUnifier,
    pud_specializer<MockMakeVar, MockUnifier>,
    MockNormalizer,
    MockParent,
    MockMakeNode,
    MockMakeVar,
    MockGlobalize,
    MockRefuted,
    MockCallSite,
    MockIterateRoots>;

} // namespace

struct PudPropagateSpecializeIntegrationTest : public ::testing::Test {
    NiceMock<MockParent>   parent;
    NiceMock<MockMakeNode> make_node;
    NiceMock<MockMakeVar>  make_var;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockRefuted>  refuted;
    NiceMock<UnifyLog>     unify_log;
    MockCallSite           call_site;
    MockIterateRoots       iter_roots;
    propagator_t propagator{parent, make_node, make_var, globalize, refuted, call_site, iter_roots};
    expr var_expr{expr::var{0}};
    expr spec_value{expr::var{3}};
    pud_node made{};

    void SetUp() override {
        g_spec_dummy_root = pud_node{};
        MockUnifier::log = &unify_log;
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(parent, get(_)).WillByDefault(Return(&g_spec_dummy_root));
        ON_CALL(make_var, make_var(_)).WillByDefault(Return(&var_expr));
        ON_CALL(make_node, make(_, _, _)).WillByDefault(Return(&made));
    }
};

TEST_F(PudPropagateSpecializeIntegrationTest, RepAtTheFrameOffsetIsNotATouchedCallerRep) {
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 0, .value = &spec_value});
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({0}, true))));
    auto root = propagator.descent_roots()[0];
    auto at_child = propagator.descend(root, &child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        return &made;
    });
    propagator.close_query(*at_child);
}

TEST_F(PudPropagateSpecializeIntegrationTest, UnifyFailureIsNotEntered) {
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 0, .value = &spec_value});
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({}, false))));
    auto root = propagator.descent_roots()[0];
    EXPECT_FALSE(propagator.descend(root, &child).has_value());
}

TEST_F(PudPropagateSpecializeIntegrationTest, CloseOfNeverPropagatedHandleHasNoBodyGoals) {
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t vars) {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        EXPECT_EQ(vars, 0u);
        return &made;
    });
    EXPECT_EQ(propagator.close_query(propagator.descent_roots()[0]), &made);
}

TEST_F(PudPropagateSpecializeIntegrationTest, RepBelowTheOffsetIsTheChildsTouchedCallerRep) {
    pud_node with_live_vars{};
    with_live_vars.added_var_count = 5;
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    // root.lvc=1, with_live_vars.added_var_count=5 → at_live.lvc=6 → query_frame_offset=6
    // scripted_unify yields rep 3 (< 6, is caller rep) and rep 6 (= 6, is not a caller rep)
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({3, 6}, true))));
    auto root = propagator.descent_roots()[0];
    auto at_live = propagator.descend(root, &with_live_vars);
    ASSERT_TRUE(at_live.has_value());
    auto opened = propagator.open_query(*at_live, &spec_value)[0];
    auto at_child = propagator.descend(opened, &child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*>, uint32_t) {
        EXPECT_EQ(specs.size(), 1u);
        if (!specs.empty())
            EXPECT_EQ(specs[0].var_idx, 3u);
        return &made;
    });
    propagator.close_query(*at_child);
}
