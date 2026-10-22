#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_base_descent.hpp"
#include "value_objects/pud_watch_head_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

namespace {
using body_goal_exprs_t = immer::map<pud_body_goal_id, const expr*>;

struct watch_result_t {
    std::vector<pud_watch_head_id> structure_changes;
    std::vector<pud_watch_head_id> added_head_ids;
    int child_tag;
};

struct SubDescentOps {
    MOCK_METHOD(int, descend_binding, (int, pud_node_id));
    MOCK_METHOD(const pud_ubs_state&, ubs_state, (int));
    MOCK_METHOD(int, descend_lvc, (int, pud_node_id));
    MOCK_METHOD(uint32_t, lvc, (int));
    MOCK_METHOD(pud_body_goal_id, descend_next_body_goal_id, (pud_body_goal_id, pud_node_id));
    MOCK_METHOD(int, descend_body_goal_expr, (int, pud_body_goal_id, pud_node_id, pud_node_id));
    MOCK_METHOD(const body_goal_exprs_t&, body_goal_exprs, (int));
    MOCK_METHOD(
        watch_result_t,
        descend_structure_watch,
        (int, pud_node_id, const std::vector<pud_watch_head_id>&, const std::vector<const expr*>&));
    MOCK_METHOD(
        int,
        descend_body_goal_pq,
        (int,
         pud_body_goal_id,
         pud_node_id,
         pud_node_id,
         const pud_ubs_state&,
         uint32_t,
         const body_goal_exprs_t&,
         const std::vector<pud_watch_head_id>&,
         const std::vector<pud_watch_head_id>&));
    MOCK_METHOD(pud_watch_head_id, find_call_site_watch_head_id, (int, pud_node_id));
};

SubDescentOps* sub_descent_ops = nullptr;

struct MockBindingDescent {
    int tag;
    MockBindingDescent descend(pud_node_id child) const {
        return {sub_descent_ops->descend_binding(tag, child)};
    }
    const pud_ubs_state& ubs_state() const { return sub_descent_ops->ubs_state(tag); }
};

struct MockLvcDescent {
    int tag;
    MockLvcDescent descend(pud_node_id child) const {
        return {sub_descent_ops->descend_lvc(tag, child)};
    }
    uint32_t lvc() const { return sub_descent_ops->lvc(tag); }
};

struct MockNextBodyGoalIdDescent {
    pud_body_goal_id next;
    MockNextBodyGoalIdDescent descend(pud_node_id child) const {
        return {sub_descent_ops->descend_next_body_goal_id(next, child)};
    }
    pud_body_goal_id next_body_goal_id() const { return next; }
};

struct MockBodyGoalExprDescent {
    int tag;
    MockBodyGoalExprDescent descend(pud_body_goal_id next_body_goal_id, pud_node_id parent, pud_node_id child) const {
        return {sub_descent_ops->descend_body_goal_expr(tag, next_body_goal_id, parent, child)};
    }
    const body_goal_exprs_t& body_goal_exprs() const { return sub_descent_ops->body_goal_exprs(tag); }
};

struct MockStructureWatchDescent {
    struct descend_result;
    int tag;
    descend_result descend(
        pud_node_id child,
        const std::vector<pud_watch_head_id>& unwatch,
        const std::vector<const expr*>& watch,
        const pud_ubs_state& child_ubs_state) const;
};

struct MockStructureWatchDescent::descend_result {
    std::vector<pud_watch_head_id> structure_changes;
    std::vector<pud_watch_head_id> added_head_ids;
    MockStructureWatchDescent child_descent;
};

MockStructureWatchDescent::descend_result MockStructureWatchDescent::descend(
    pud_node_id child,
    const std::vector<pud_watch_head_id>& unwatch,
    const std::vector<const expr*>& watch,
    const pud_ubs_state&) const {
    watch_result_t watched = sub_descent_ops->descend_structure_watch(tag, child, unwatch, watch);
    return {watched.structure_changes, watched.added_head_ids, MockStructureWatchDescent{watched.child_tag}};
}

struct MockBodyGoalPqDescent {
    int tag;
    MockBodyGoalPqDescent descend(
        pud_body_goal_id next_body_goal_id,
        pud_node_id parent,
        pud_node_id child,
        const pud_ubs_state& child_ubs_state,
        uint32_t child_frame_offset,
        const body_goal_exprs_t& child_body_goal_exprs,
        const std::vector<pud_watch_head_id>& structure_changes,
        const std::vector<pud_watch_head_id>& added_watch_head_ids) const {
        return {sub_descent_ops->descend_body_goal_pq(
            tag,
            next_body_goal_id,
            parent,
            child,
            child_ubs_state,
            child_frame_offset,
            child_body_goal_exprs,
            structure_changes,
            added_watch_head_ids)};
    }
    pud_watch_head_id find_call_site_watch_head_id(pud_node_id parent) const {
        return sub_descent_ops->find_call_site_watch_head_id(tag, parent);
    }
};

using test_descent_t = pud_base_descent<
    MockBindingDescent,
    MockLvcDescent,
    MockNextBodyGoalIdDescent,
    MockBodyGoalExprDescent,
    MockStructureWatchDescent,
    MockBodyGoalPqDescent>;

const expr* fake_expr(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudBaseDescentTest : public ::testing::Test {
    NiceMock<SubDescentOps> ops;
    pud_node_id parent = 1;
    pud_node_id child = 2;
    pud_node_id sibling = 3;
    pud_ubs_state child_ubs_state;
    body_goal_exprs_t child_body_goal_exprs;
    test_descent_t at_parent{
        MockBindingDescent{10},
        MockLvcDescent{20},
        MockNextBodyGoalIdDescent{4},
        MockBodyGoalExprDescent{40},
        MockStructureWatchDescent{60},
        MockBodyGoalPqDescent{50},
    };

    void SetUp() override {
        sub_descent_ops = &ops;
        child_ubs_state.parents = child_ubs_state.parents.set(0, 0);
        child_body_goal_exprs = child_body_goal_exprs.set(4, fake_expr(0x40)).set(5, fake_expr(0x50));
        ON_CALL(ops, ubs_state(_)).WillByDefault(ReturnRef(child_ubs_state));
        ON_CALL(ops, body_goal_exprs(_)).WillByDefault(ReturnRef(child_body_goal_exprs));
        ON_CALL(ops, descend_next_body_goal_id(_, _)).WillByDefault(Return(4));
    }

    void TearDown() override {
        sub_descent_ops = nullptr;
    }
};

TEST_F(PudBaseDescentTest, ConstructorKeepsEachRootSubDescent) {
    EXPECT_EQ(at_parent.binding().tag, 10);
    EXPECT_EQ(at_parent.lvc().tag, 20);
    EXPECT_EQ(at_parent.body_goal_expr().tag, 40);
    EXPECT_EQ(at_parent.body_goal_pq().tag, 50);
}

TEST_F(PudBaseDescentTest, DescendForwardsTheChildToEverySubDescent) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(ops, descend_lvc(20, child)).WillOnce(Return(21));
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(4));
    EXPECT_CALL(ops, descend_body_goal_expr(40, _, _, child)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_structure_watch(60, child, _, _)).WillOnce(Return(watch_result_t{{}, {}, 61}));
    EXPECT_CALL(ops, descend_body_goal_pq(50, _, _, _, _, _, _, _, _)).WillOnce(Return(51));
    test_descent_t at_child = at_parent.descend(parent, child);
    EXPECT_EQ(at_child.binding().tag, 11);
    EXPECT_EQ(at_child.lvc().tag, 21);
    EXPECT_EQ(at_child.body_goal_expr().tag, 41);
    EXPECT_EQ(at_child.body_goal_pq().tag, 51);
}

TEST_F(PudBaseDescentTest, ExprAndPqGetTheParentsNextBodyGoalId) {
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(7));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, _, _)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 4u, _, _, _, _, _, _, _)).WillOnce(Return(51));
    child_body_goal_exprs = child_body_goal_exprs.set(6, fake_expr(0x60));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, ExprAndPqGetTheParentNode) {
    EXPECT_CALL(ops, descend_body_goal_expr(_, _, parent, child)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_body_goal_pq(_, _, parent, _, _, _, _, _, _)).WillOnce(Return(51));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, WatchUnwatchesTheParentsCallSiteGoal) {
    EXPECT_CALL(ops, find_call_site_watch_head_id(50, parent)).WillOnce(Return(12));
    EXPECT_CALL(ops, descend_structure_watch(_, _, ElementsAre(12u), _)).WillOnce(Return(watch_result_t{}));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, NewGoalsAreWatchedAsStored) {
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(6));
    EXPECT_CALL(ops, descend_body_goal_expr(40, _, _, _)).WillOnce(Return(41));
    EXPECT_CALL(ops, body_goal_exprs(41)).WillRepeatedly(ReturnRef(child_body_goal_exprs));
    EXPECT_CALL(ops, descend_structure_watch(_, _, _, ElementsAre(fake_expr(0x40), fake_expr(0x50))))
        .WillOnce(Return(watch_result_t{}));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, FactChildWatchesNothingNew) {
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(4));
    EXPECT_CALL(ops, descend_structure_watch(_, _, _, IsEmpty())).WillOnce(Return(watch_result_t{}));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, PqGetsTheChildsEnvironment) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(ops, ubs_state(11)).WillRepeatedly(ReturnRef(child_ubs_state));
    EXPECT_CALL(ops, descend_lvc(20, child)).WillOnce(Return(21));
    EXPECT_CALL(ops, lvc(21)).WillOnce(Return(9));
    EXPECT_CALL(ops, descend_body_goal_expr(40, _, _, _)).WillOnce(Return(41));
    EXPECT_CALL(ops, body_goal_exprs(41)).WillRepeatedly(ReturnRef(child_body_goal_exprs));
    EXPECT_CALL(ops, descend_body_goal_pq(_, _, _, _, Ref(child_ubs_state), 9u, Ref(child_body_goal_exprs), _, _))
        .WillOnce(Return(51));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, PqGetsTheStructureWatchResults) {
    EXPECT_CALL(ops, descend_structure_watch(_, _, _, _)).WillOnce(Return(watch_result_t{{1, 2}, {3}, 61}));
    EXPECT_CALL(ops, descend_body_goal_pq(_, _, _, _, _, _, _, ElementsAre(1u, 2u), ElementsAre(3u))).WillOnce(Return(51));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, ChildKeepsTheStructureWatchsChildDescent) {
    EXPECT_CALL(ops, descend_structure_watch(60, child, _, _)).WillOnce(Return(watch_result_t{{}, {}, 61}));
    test_descent_t at_child = at_parent.descend(parent, child);
    EXPECT_CALL(ops, descend_structure_watch(61, sibling, _, _)).WillOnce(Return(watch_result_t{}));
    at_child.descend(child, sibling);
}

TEST_F(PudBaseDescentTest, GrandchildUnwatchesTheChildsCallSiteGoal) {
    EXPECT_CALL(ops, descend_body_goal_pq(50, _, _, _, _, _, _, _, _)).WillOnce(Return(51));
    test_descent_t at_child = at_parent.descend(parent, child);
    EXPECT_CALL(ops, descend_body_goal_pq(51, _, _, _, _, _, _, _, _)).WillOnce(Return(52));
    EXPECT_CALL(ops, find_call_site_watch_head_id(51, child)).WillOnce(Return(13));
    EXPECT_CALL(ops, descend_structure_watch(_, sibling, ElementsAre(13u), _)).WillOnce(Return(watch_result_t{}));
    at_child.descend(child, sibling);
}

TEST_F(PudBaseDescentTest, DescendLeavesParentUntouched) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(ops, descend_structure_watch(60, child, _, _)).WillOnce(Return(watch_result_t{}));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, parent, child)).WillOnce(Return(41));
    at_parent.descend(parent, child);
    EXPECT_EQ(at_parent.binding().tag, 10);
    EXPECT_EQ(at_parent.body_goal_pq().tag, 50);
    EXPECT_CALL(ops, descend_binding(10, sibling)).WillOnce(Return(12));
    EXPECT_CALL(ops, descend_structure_watch(60, sibling, _, _)).WillOnce(Return(watch_result_t{}));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, parent, sibling)).WillOnce(Return(0));
    at_parent.descend(parent, sibling);
}

TEST_F(PudBaseDescentTest, SiblingsDescendFromTheSameParentState) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(ops, descend_binding(10, sibling)).WillOnce(Return(12));
    EXPECT_CALL(ops, descend_body_goal_pq(50, _, _, _, _, _, _, _, _)).WillOnce(Return(51)).WillOnce(Return(52));
    test_descent_t at_child = at_parent.descend(parent, child);
    test_descent_t at_sibling = at_parent.descend(parent, sibling);
    EXPECT_EQ(at_child.binding().tag, 11);
    EXPECT_EQ(at_sibling.binding().tag, 12);
    EXPECT_EQ(at_child.body_goal_pq().tag, 51);
    EXPECT_EQ(at_sibling.body_goal_pq().tag, 52);
}

TEST_F(PudBaseDescentTest, ConstructorDescendsNothing) {
    StrictMock<SubDescentOps> strict_ops;
    sub_descent_ops = &strict_ops;
    test_descent_t descent{
        MockBindingDescent{1},
        MockLvcDescent{2},
        MockNextBodyGoalIdDescent{3},
        MockBodyGoalExprDescent{4},
        MockStructureWatchDescent{6},
        MockBodyGoalPqDescent{5},
    };
    EXPECT_EQ(descent.binding().tag, 1);
    sub_descent_ops = &ops;
}

TEST_F(PudBaseDescentTest, DescendCallsEverySubDescentExactlyOnce) {
    StrictMock<SubDescentOps> strict_ops;
    sub_descent_ops = &strict_ops;
    EXPECT_CALL(strict_ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(strict_ops, descend_lvc(20, child)).WillOnce(Return(21));
    EXPECT_CALL(strict_ops, descend_next_body_goal_id(4, child)).WillOnce(Return(4));
    EXPECT_CALL(strict_ops, descend_body_goal_expr(40, 4u, parent, child)).WillOnce(Return(41));
    EXPECT_CALL(strict_ops, find_call_site_watch_head_id(50, parent)).WillOnce(Return(12));
    EXPECT_CALL(strict_ops, descend_structure_watch(60, child, _, _)).WillOnce(Return(watch_result_t{}));
    EXPECT_CALL(strict_ops, ubs_state(11)).WillOnce(ReturnRef(child_ubs_state));
    EXPECT_CALL(strict_ops, lvc(21)).WillOnce(Return(0));
    EXPECT_CALL(strict_ops, body_goal_exprs(41)).WillOnce(ReturnRef(child_body_goal_exprs));
    EXPECT_CALL(strict_ops, descend_body_goal_pq(50, 4u, parent, _, _, _, _, _, _)).WillOnce(Return(51));
    at_parent.descend(parent, child);
    sub_descent_ops = &ops;
}

TEST_F(PudBaseDescentTest, NextBodyGoalIdIsThreadedThroughEveryLevel) {
    const pud_node_id grandchild = 4;
    const pud_node_id great_grandchild = 5;
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(6));
    EXPECT_CALL(ops, descend_next_body_goal_id(6, grandchild)).WillOnce(Return(6));
    EXPECT_CALL(ops, descend_next_body_goal_id(6, great_grandchild)).WillOnce(Return(6));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, parent, child)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 6u, child, grandchild)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 6u, grandchild, great_grandchild)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 4u, parent, _, _, _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 6u, child, _, _, _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 6u, grandchild, _, _, _, _, _, _)).WillOnce(Return(0));
    at_parent.descend(parent, child).descend(child, grandchild).descend(grandchild, great_grandchild);
}

TEST_F(PudBaseDescentTest, DescentIsAssignable) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    test_descent_t current = at_parent;
    current = current.descend(parent, child);
    EXPECT_EQ(current.binding().tag, 11);
}

TEST_F(PudBaseDescentTest, PqGetsTheChildNode) {
    EXPECT_CALL(ops, descend_body_goal_pq(_, _, parent, child, _, _, _, _, _)).WillOnce(Return(51));
    at_parent.descend(parent, child);
}

}
