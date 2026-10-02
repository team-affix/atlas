#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_query_propagator.hpp"
#include "infrastructure/pud_specializer.hpp"
#include "value_objects/om_interval.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

uint64_t rank_storage[8] = {1, 2, 3, 4, 5, 6, 7, 8};

om_interval interval_of(uint64_t* open, uint64_t* close) {
    return om_interval{om_label{open}, om_label{close}};
}

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

struct MockAllocateRoot {
    MOCK_METHOD(om_interval, allocate_root, ());
};

struct MockAllocateChild {
    MOCK_METHOD(om_interval, allocate_child_of, (om_interval));
};

struct MockRecord {
    void record(om_interval, uint32_t, framed_expr) {}
};

struct MockQueryFp {
    std::optional<framed_expr> query(om_label, uint32_t) { return std::nullopt; }
};

struct UnifyLog {
    MOCK_METHOD((coroutine<uint32_t, bool>), unify, (framed_expr, framed_expr));
};

struct MockBindMap {
    template<typename G, typename R, typename Q>
    MockBindMap(G&, R&, Q&, om_interval) {}
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

using propagator_t = pud_query_propagator<
    MockBindMap,
    MockUnifier,
    pud_specializer<MockMakeVar, MockUnifier>,
    MockNormalizer,
    MockParent,
    MockMakeNode,
    MockAllocateRoot,
    MockAllocateChild,
    MockMakeVar,
    MockGlobalize,
    MockRecord,
    MockQueryFp,
    MockRefuted>;

} // namespace

struct PudPropagateSpecializeIntegrationTest : public ::testing::Test {
    NiceMock<MockParent> parent;
    NiceMock<MockMakeNode> make_node;
    NiceMock<MockAllocateRoot> allocate_root;
    NiceMock<MockAllocateChild> allocate_child;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockGlobalize> globalize;
    MockRecord record;
    MockQueryFp query_fp;
    NiceMock<MockRefuted> refuted;
    NiceMock<UnifyLog> unify_log;
    propagator_t propagator{
        parent, make_node, allocate_root, allocate_child, make_var, globalize, record, query_fp, refuted};
    om_interval root_interval = interval_of(&rank_storage[0], &rank_storage[1]);
    om_interval child_interval = interval_of(&rank_storage[2], &rank_storage[3]);
    expr var_expr{expr::var{0}};
    expr spec_value{expr::var{3}};
    pud_node made{};

    void SetUp() override {
        MockUnifier::log = &unify_log;
        ON_CALL(allocate_root, allocate_root()).WillByDefault(Return(root_interval));
        ON_CALL(allocate_child, allocate_child_of(_)).WillByDefault(Return(child_interval));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
        ON_CALL(parent, get(_)).WillByDefault(Return(nullptr));
        ON_CALL(make_var, make_var(_)).WillByDefault(Return(&var_expr));
        ON_CALL(make_node, make(_, _, _)).WillByDefault(Return(&made));
    }
};

TEST_F(PudPropagateSpecializeIntegrationTest, RepAtTheFrameOffsetIsNotATouchedCallerRep) {
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 0, .value = &spec_value});
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({0}, true))));
    auto root = propagator.root();
    auto at_child = propagator.propagate(root, &child);
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
    auto root = propagator.root();
    EXPECT_FALSE(propagator.propagate(root, &child).has_value());
}

TEST_F(PudPropagateSpecializeIntegrationTest, CloseOfNeverPropagatedHandleHasNoBodyGoals) {
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t vars) {
        EXPECT_TRUE(specs.empty());
        EXPECT_TRUE(goals.empty());
        EXPECT_EQ(vars, 0u);
        return &made;
    });
    EXPECT_EQ(propagator.close_query(propagator.root()), &made);
}

TEST_F(PudPropagateSpecializeIntegrationTest, RepBelowTheOffsetIsTheChildsTouchedCallerRep) {
    pud_node with_live_vars{};
    with_live_vars.added_var_count = 5;
    pud_node child{};
    child.added_specializations.push_back(pud_specialization{.var_idx = 1, .value = &spec_value});
    EXPECT_CALL(unify_log, unify(_, _)).WillOnce(Return(::testing::ByMove(scripted_unify({3, 5}, true))));
    auto root = propagator.root();
    auto at_live = propagator.propagate(root, &with_live_vars);
    ASSERT_TRUE(at_live.has_value());
    auto opened = propagator.open_query(*at_live, &spec_value);
    auto at_child = propagator.propagate(opened, &child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_CALL(make_node, make(_, _, _)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*>, uint32_t) {
        EXPECT_EQ(specs.size(), 1u);
        if (!specs.empty())
            EXPECT_EQ(specs[0].var_idx, 3u);
        return &made;
    });
    propagator.close_query(*at_child);
}
