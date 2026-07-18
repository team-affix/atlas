// rp_srt_active_goals: forwards insert/clear/link; set percolates max;
// link sets parent to -inf then percolates before forward.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
#include <stdexcept>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::ByMove;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

constexpr double kNegInf = -std::numeric_limits<double>::infinity();

struct MockInsertActiveGoal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*));
};

struct MockClearActiveGoals {
    MOCK_METHOD(void, clear_active_goals, ());
};

struct MockLinkSrtGoalBatchParent {
    MOCK_METHOD(void, link_srt_goal_batch_parent, (const goal_lineage*));
};

struct MockGetParentGoal {
    MOCK_METHOD(const goal_lineage*, get_parent_goal, (const goal_lineage*), (const));
};

struct MockIterateChildGoals {
    MOCK_METHOD((coroutine<const goal_lineage*, void>), iterate_child_goals,
                (const goal_lineage*), (const));
};

coroutine<const goal_lineage*, void> two_children_sm(const goal_lineage* a,
                                                    const goal_lineage* b) {
    co_yield a;
    co_yield b;
}

}  // namespace

using rp_srt_active_goals_t = rp_srt_active_goals<
    MockInsertActiveGoal, MockClearActiveGoals, srt_active_goals, srt_active_goals,
    MockLinkSrtGoalBatchParent>;

using rp_srt_fully_mocked_t = rp_srt_active_goals<
    MockInsertActiveGoal, MockClearActiveGoals, MockGetParentGoal,
    MockIterateChildGoals, MockLinkSrtGoalBatchParent>;

struct RpSrtActiveGoalsTest : public ::testing::Test {
    StrictMock<MockInsertActiveGoal> insert;
    StrictMock<MockClearActiveGoals> clear;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    srt_active_goals srt;
    rp_srt_active_goals_t rp{insert, clear, srt, srt, link};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 1};
    goal_lineage child1{nullptr, 2};
};

struct RpSrtActiveGoalsFullyMockedTest : public ::testing::Test {
    StrictMock<MockInsertActiveGoal> insert;
    StrictMock<MockClearActiveGoals> clear;
    StrictMock<MockGetParentGoal> get_parent;
    StrictMock<MockIterateChildGoals> iterate_children;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    rp_srt_fully_mocked_t rp{insert, clear, get_parent, iterate_children, link};

    goal_lineage gp{nullptr, 0};
    goal_lineage p{nullptr, 1};
    goal_lineage c{nullptr, 2};
    goal_lineage s{nullptr, 3};
    goal_lineage g{nullptr, 4};
};

TEST_F(RpSrtActiveGoalsTest, InsertForwardsThenDefaultsLeafScoreToZero) {
    EXPECT_CALL(insert, insert_active_goal(&child0));
    rp.insert_active_goal(&child0);
    EXPECT_EQ(rp.get(&child0), 0.0);
}

TEST_F(RpSrtActiveGoalsTest, SetUpdatesLeafAndPercolatesParentMax) {
    EXPECT_CALL(insert, insert_active_goal(&parent));
    rp.insert_active_goal(&parent);
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&child0));
    EXPECT_CALL(insert, insert_active_goal(&child1));
    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    srt.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);
    srt.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&child0), -2.0);
    EXPECT_EQ(rp.get(&child1), -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);
}

TEST_F(RpSrtActiveGoalsTest, PercolateEarlyExitsWhenMaxUnchanged) {
    EXPECT_CALL(insert, insert_active_goal(&parent));
    rp.insert_active_goal(&parent);
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&child0));
    EXPECT_CALL(insert, insert_active_goal(&child1));
    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    srt.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);
    srt.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);

    rp.set_active_goal_value(&child1, -4.0);
    EXPECT_EQ(rp.get(&parent), -2.0);
}

TEST_F(RpSrtActiveGoalsTest, ClearForwardsThenRemovesScores) {
    EXPECT_CALL(insert, insert_active_goal(&child0));
    rp.insert_active_goal(&child0);
    EXPECT_CALL(clear, clear_active_goals());
    rp.clear_active_goals();
    EXPECT_THROW(rp.get(&child0), std::out_of_range);
}

TEST_F(RpSrtActiveGoalsTest, LinkSetsNegInfPercolatesThenForwards) {
    goal_lineage grandparent{nullptr, 3};
    goal_lineage sibling{nullptr, 4};

    EXPECT_CALL(insert, insert_active_goal(&grandparent));
    rp.insert_active_goal(&grandparent);
    srt.insert_active_goal(&grandparent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&parent));
    EXPECT_CALL(insert, insert_active_goal(&sibling));
    rp.insert_active_goal(&parent);
    rp.insert_active_goal(&sibling);
    srt.insert_active_goal(&parent);
    srt.insert_active_goal(&sibling);
    srt.link_srt_goal_batch_parent(&grandparent);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&parent, -2.0);
    rp.set_active_goal_value(&sibling, -5.0);
    EXPECT_EQ(rp.get(&grandparent), -2.0);

    bool linked = false;
    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent))
        .WillOnce([&](const goal_lineage*) {
            EXPECT_EQ(rp.get(&parent),
                      -std::numeric_limits<double>::infinity());
            EXPECT_EQ(rp.get(&grandparent), -5.0);
            linked = true;
        });
    rp.link_srt_goal_batch_parent(&parent);
    EXPECT_TRUE(linked);
    EXPECT_EQ(rp.get(&parent), -std::numeric_limits<double>::infinity());
    EXPECT_EQ(rp.get(&grandparent), -5.0);
}

TEST_F(RpSrtActiveGoalsTest, CallerSetAfterLinkOverwritesNegInfWithChildMax) {
    EXPECT_CALL(insert, insert_active_goal(&parent));
    rp.insert_active_goal(&parent);
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&child0));
    EXPECT_CALL(insert, insert_active_goal(&child1));
    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    srt.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);

    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent))
        .WillOnce([&](const goal_lineage* p) {
            srt.link_srt_goal_batch_parent(p);
        });
    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_EQ(rp.get(&parent), -std::numeric_limits<double>::infinity());

    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);
}

TEST_F(RpSrtActiveGoalsFullyMockedTest, SetWithNullParentDoesNotIterateChildren) {
    EXPECT_CALL(insert, insert_active_goal(&g));
    rp.insert_active_goal(&g);
    EXPECT_CALL(get_parent, get_parent_goal(&g)).WillOnce(Return(nullptr));
    EXPECT_CALL(iterate_children, iterate_child_goals).Times(0);
    rp.set_active_goal_value(&g, -3.0);
    EXPECT_EQ(rp.get(&g), -3.0);
}

TEST_F(RpSrtActiveGoalsFullyMockedTest, PercolateEarlyExitStopsWalkingAncestors) {
    EXPECT_CALL(insert, insert_active_goal(&p));
    EXPECT_CALL(insert, insert_active_goal(&c));
    EXPECT_CALL(insert, insert_active_goal(&s));
    rp.insert_active_goal(&p);
    rp.insert_active_goal(&c);
    rp.insert_active_goal(&s);

    // Seed S=-2 (max with C=0 stays 0 → early exit at P).
    EXPECT_CALL(get_parent, get_parent_goal(&s)).WillOnce(Return(&p));
    EXPECT_CALL(iterate_children, iterate_child_goals(&p))
        .WillOnce(Return(ByMove(two_children_sm(&c, &s))));
    rp.set_active_goal_value(&s, -2.0);

    // set C=-5 → P becomes -2; walk parent of P once.
    EXPECT_CALL(get_parent, get_parent_goal(&c)).WillOnce(Return(&p));
    EXPECT_CALL(iterate_children, iterate_child_goals(&p))
        .WillOnce(Return(ByMove(two_children_sm(&c, &s))));
    EXPECT_CALL(get_parent, get_parent_goal(&p)).WillOnce(Return(nullptr));
    rp.set_active_goal_value(&c, -5.0);
    EXPECT_EQ(rp.get(&p), -2.0);

    // set C to -4; max still -2 via S → early exit; get_parent(P) not called.
    EXPECT_CALL(get_parent, get_parent_goal(&c)).WillOnce(Return(&p));
    EXPECT_CALL(iterate_children, iterate_child_goals(&p))
        .WillOnce(Return(ByMove(two_children_sm(&c, &s))));
    EXPECT_CALL(get_parent, get_parent_goal(&p)).Times(0);
    rp.set_active_goal_value(&c, -4.0);
    EXPECT_EQ(rp.get(&p), -2.0);
}

TEST_F(RpSrtActiveGoalsFullyMockedTest, LinkAssignsNegInfPercolatesThenForwards) {
    EXPECT_CALL(insert, insert_active_goal(&gp));
    EXPECT_CALL(insert, insert_active_goal(&p));
    EXPECT_CALL(insert, insert_active_goal(&s));
    rp.insert_active_goal(&gp);
    rp.insert_active_goal(&p);
    rp.insert_active_goal(&s);

    EXPECT_CALL(get_parent, get_parent_goal(&s)).WillOnce(Return(&gp));
    EXPECT_CALL(iterate_children, iterate_child_goals(&gp))
        .WillOnce(Return(ByMove(two_children_sm(&p, &s))));
    // max(P=0, S=-5)=0 == GP → early exit; no get_parent(GP).
    rp.set_active_goal_value(&s, -5.0);
    EXPECT_EQ(rp.get(&gp), 0.0);

    InSequence seq;
    EXPECT_CALL(get_parent, get_parent_goal(&p)).WillOnce(Return(&gp));
    EXPECT_CALL(iterate_children, iterate_child_goals(&gp))
        .WillOnce(Return(ByMove(two_children_sm(&p, &s))));
    EXPECT_CALL(get_parent, get_parent_goal(&gp)).WillOnce(Return(nullptr));
    EXPECT_CALL(link, link_srt_goal_batch_parent(&p)).WillOnce([&](const goal_lineage*) {
        EXPECT_EQ(rp.get(&p), kNegInf);
        EXPECT_EQ(rp.get(&gp), -5.0);
    });
    rp.link_srt_goal_batch_parent(&p);
    EXPECT_EQ(rp.get(&p), kNegInf);
    EXPECT_EQ(rp.get(&gp), -5.0);
}
