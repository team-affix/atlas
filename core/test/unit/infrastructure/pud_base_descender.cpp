#include <immer/map.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_base_descender.hpp"

namespace {

using ::testing::_;
using ::testing::Field;
using ::testing::Return;
using ::testing::SizeIs;

struct MockBindingDescend {
    MOCK_METHOD(pud_binding_descent, root_descent, (pud_node_id));
    MOCK_METHOD(pud_binding_descent, descend, (const pud_binding_descent&, pud_node_id));
};

struct MockLvcDescend {
    MOCK_METHOD(pud_lvc_descent, root_descent, (pud_node_id));
    MOCK_METHOD(pud_lvc_descent, descend, (const pud_lvc_descent&, pud_node_id));
};

struct MockNextBodyGoalIdDescend {
    MOCK_METHOD(pud_next_body_goal_id_descent, root_descent, (pud_node_id));
    MOCK_METHOD(pud_next_body_goal_id_descent, descend,
        (const pud_next_body_goal_id_descent&, pud_node_id));
};

struct MockBodyGoalExprDescend {
    MOCK_METHOD(pud_body_goal_expr_descent, root_descent, (pud_node_id));
    MOCK_METHOD(pud_body_goal_expr_descent, descend,
        (const pud_body_goal_expr_descent&, body_goal_id, pud_node_id, pud_node_id));
};

struct MockBodyGoalPqDescend {
    MOCK_METHOD(pud_body_goal_pq_descent, root_descent, (pud_node_id));
    MOCK_METHOD(pud_body_goal_pq_descent, descend,
        (const pud_body_goal_pq_descent&, body_goal_id, pud_node_id, pud_node_id));
};

struct PudBaseDescenderTest : public ::testing::Test {
    ::testing::NiceMock<MockBindingDescend> binding;
    ::testing::NiceMock<MockLvcDescend> lvc;
    ::testing::NiceMock<MockNextBodyGoalIdDescend> next_body_goal_id;
    ::testing::NiceMock<MockBodyGoalExprDescend> body_goal_expr;
    ::testing::NiceMock<MockBodyGoalPqDescend> body_goal_pq;
    pud_base_descender<
        MockBindingDescend, MockBindingDescend,
        MockLvcDescend, MockLvcDescend,
        MockNextBodyGoalIdDescend, MockNextBodyGoalIdDescend,
        MockBodyGoalExprDescend, MockBodyGoalExprDescend,
        MockBodyGoalPqDescend, MockBodyGoalPqDescend> descender{
        binding, binding,
        lvc, lvc,
        next_body_goal_id, next_body_goal_id,
        body_goal_expr, body_goal_expr,
        body_goal_pq, body_goal_pq};

    pud_binding_descent binding_descent(uint32_t bound_var) {
        return pud_binding_descent{
            .ubs_state = pud_ubs_state{.parents = immer::map<uint32_t, uint32_t>{}.set(bound_var, 0)}};
    }

    pud_body_goal_expr_descent body_goal_expr_descent(body_goal_id goal_count) {
        immer::map<body_goal_id, const expr*> body_goal_exprs;
        for (body_goal_id id = 0; id < goal_count; ++id)
            body_goal_exprs = body_goal_exprs.set(id, nullptr);
        return pud_body_goal_expr_descent{.body_goal_exprs = body_goal_exprs};
    }

    pud_body_goal_pq_descent body_goal_pq_descent(body_goal_id goal_count) {
        immer::map<body_goal_id, immer::box<persistent_query>> body_goal_pqs;
        for (body_goal_id id = 0; id < goal_count; ++id)
            body_goal_pqs = body_goal_pqs.set(id, immer::box<persistent_query>{});
        return pud_body_goal_pq_descent{.body_goal_pqs = body_goal_pqs};
    }
};

TEST_F(PudBaseDescenderTest, RootDescentCombinesEachRootDescent) {
    const pud_node_id root = 7;
    EXPECT_CALL(binding, root_descent(root)).WillOnce(Return(binding_descent(1)));
    EXPECT_CALL(lvc, root_descent(root)).WillOnce(Return(pud_lvc_descent{.lvc = 3}));
    EXPECT_CALL(next_body_goal_id, root_descent(root))
        .WillOnce(Return(pud_next_body_goal_id_descent{.next_body_goal_id = 2}));
    EXPECT_CALL(body_goal_expr, root_descent(root)).WillOnce(Return(body_goal_expr_descent(2)));
    EXPECT_CALL(body_goal_pq, root_descent(root)).WillOnce(Return(body_goal_pq_descent(2)));

    pud_base_descent descent = descender.root_descent(root);
    EXPECT_EQ(descent.binding.ubs_state, binding_descent(1).ubs_state);
    EXPECT_EQ(descent.lvc.lvc, 3u);
    EXPECT_EQ(descent.next_body_goal_id.next_body_goal_id, 2u);
    EXPECT_EQ(descent.body_goal_expr.body_goal_exprs.size(), 2u);
    EXPECT_EQ(descent.body_goal_pq.body_goal_pqs.size(), 2u);
}

TEST_F(PudBaseDescenderTest, DescendForwardsEachSubDescentWithNodesItNeeds) {
    const pud_node_id parent = 1;
    const pud_node_id child = 2;
    const pud_base_descent at_parent{
        .binding = binding_descent(1),
        .lvc = pud_lvc_descent{.lvc = 3},
        .next_body_goal_id = pud_next_body_goal_id_descent{.next_body_goal_id = 4},
        .body_goal_expr = body_goal_expr_descent(1),
        .body_goal_pq = body_goal_pq_descent(1),
    };

    EXPECT_CALL(binding, descend(Field(&pud_binding_descent::ubs_state, binding_descent(1).ubs_state), child))
        .WillOnce(Return(binding_descent(5)));
    EXPECT_CALL(lvc, descend(Field(&pud_lvc_descent::lvc, 3u), child))
        .WillOnce(Return(pud_lvc_descent{.lvc = 6}));
    EXPECT_CALL(next_body_goal_id,
                descend(Field(&pud_next_body_goal_id_descent::next_body_goal_id, 4u), child))
        .WillOnce(Return(pud_next_body_goal_id_descent{.next_body_goal_id = 7}));
    EXPECT_CALL(body_goal_expr,
                descend(Field(&pud_body_goal_expr_descent::body_goal_exprs, SizeIs(1)), 4u, parent, child))
        .WillOnce(Return(body_goal_expr_descent(3)));
    EXPECT_CALL(body_goal_pq,
                descend(Field(&pud_body_goal_pq_descent::body_goal_pqs, SizeIs(1)), 4u, parent, child))
        .WillOnce(Return(body_goal_pq_descent(3)));

    pud_base_descent at_child = descender.descend(at_parent, parent, child);
    EXPECT_EQ(at_child.binding.ubs_state, binding_descent(5).ubs_state);
    EXPECT_EQ(at_child.lvc.lvc, 6u);
    EXPECT_EQ(at_child.next_body_goal_id.next_body_goal_id, 7u);
    EXPECT_EQ(at_child.body_goal_expr.body_goal_exprs.size(), 3u);
    EXPECT_EQ(at_child.body_goal_pq.body_goal_pqs.size(), 3u);
}

}
