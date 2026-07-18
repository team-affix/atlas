// dbuct_rp_srt_active_goals: insert/set/get/percolate/frames; link −∞ + forward;
// pop_frame undoes score mutations.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
#include <stdexcept>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_rp_srt_active_goals.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
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

using dbuct_rp_srt_t = dbuct_rp_srt_active_goals<
    dbuct_srt_active_goals, dbuct_srt_active_goals, dbuct_srt_active_goals,
    dbuct_srt_active_goals>;

using dbuct_rp_mocked_link_t = dbuct_rp_srt_active_goals<
    dbuct_srt_active_goals, dbuct_srt_active_goals, dbuct_srt_active_goals,
    MockLinkSrtGoalBatchParent>;

using dbuct_rp_fully_mocked_t = dbuct_rp_srt_active_goals<
    MockInsertActiveGoal, MockGetParentGoal, MockIterateChildGoals,
    MockLinkSrtGoalBatchParent>;

struct DbuctRpSrtActiveGoalsTest : public ::testing::Test {
    dbuct_srt_active_goals srt;
    dbuct_rp_srt_t rp{srt, srt, srt, srt};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 1};
    goal_lineage child1{nullptr, 2};
};

struct DbuctRpSrtMockLinkTest : public ::testing::Test {
    dbuct_srt_active_goals srt;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    dbuct_rp_mocked_link_t rp{srt, srt, srt, link};

    goal_lineage grandparent{nullptr, 0};
    goal_lineage parent{nullptr, 1};
    goal_lineage sibling{nullptr, 2};
};

struct DbuctRpSrtFullyMockedTest : public ::testing::Test {
    StrictMock<MockInsertActiveGoal> insert;
    StrictMock<MockGetParentGoal> get_parent;
    StrictMock<MockIterateChildGoals> iterate_children;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    dbuct_rp_fully_mocked_t rp{insert, get_parent, iterate_children, link};

    goal_lineage gp{nullptr, 0};
    goal_lineage p{nullptr, 1};
    goal_lineage c{nullptr, 2};
    goal_lineage s{nullptr, 3};
    goal_lineage g{nullptr, 4};
};

TEST_F(DbuctRpSrtActiveGoalsTest, InsertDefaultsLeafScoreToZero) {
    rp.insert_active_goal(&child0);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&child0), 0.0);
    EXPECT_TRUE(srt.is_active_goal(&child0));
}

TEST_F(DbuctRpSrtActiveGoalsTest, GetUnknownThrows) {
    EXPECT_THROW(rp.get(&child0), std::out_of_range);
}

TEST_F(DbuctRpSrtActiveGoalsTest, SetUpdatesLeafAndPercolatesParentMax) {
    rp.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&child0), -2.0);
    EXPECT_EQ(rp.get(&child1), -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);
}

TEST_F(DbuctRpSrtActiveGoalsTest, PercolateEarlyExitsWhenMaxUnchanged) {
    rp.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);

    rp.set_active_goal_value(&child1, -4.0);
    EXPECT_EQ(rp.get(&parent), -2.0);
}

TEST_F(DbuctRpSrtActiveGoalsTest, LinkWithNoChildrenLeavesParentNegInf) {
    rp.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&parent), 0.0);

    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&parent), kNegInf);
}

TEST_F(DbuctRpSrtActiveGoalsTest, CallerSetAfterLinkOverwritesNegInfWithChildMax) {
    rp.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_EQ(rp.get(&parent), kNegInf);

    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);
}

TEST_F(DbuctRpSrtActiveGoalsTest, PopFrameUndoesInsert) {
    rp.push_frame();
    rp.insert_active_goal(&child0);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&child0), 0.0);

    rp.pop_frame();
    EXPECT_THROW(rp.get(&child0), std::out_of_range);
}

TEST_F(DbuctRpSrtActiveGoalsTest, PopFrameUndoesLinkNegInf) {
    rp.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&parent), 0.0);

    rp.push_frame();
    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&parent), kNegInf);

    rp.pop_frame();
    EXPECT_EQ(rp.get(&parent), 0.0);
}

TEST_F(DbuctRpSrtActiveGoalsTest, NestedFrameLinkThenAssignsUndoInOrder) {
    rp.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&child0);
    rp.insert_active_goal(&child1);
    rp.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();
    EXPECT_EQ(rp.get(&parent), kNegInf);

    rp.push_frame();
    rp.set_active_goal_value(&child0, -2.0);
    rp.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(rp.get(&parent), -2.0);

    rp.push_frame();
    rp.set_active_goal_value(&child0, -1.0);
    EXPECT_EQ(rp.get(&parent), -1.0);

    rp.pop_frame();
    EXPECT_EQ(rp.get(&child0), -2.0);
    EXPECT_EQ(rp.get(&parent), -2.0);

    rp.pop_frame();
    EXPECT_EQ(rp.get(&child0), 0.0);
    EXPECT_EQ(rp.get(&child1), 0.0);
    EXPECT_EQ(rp.get(&parent), kNegInf);
}

TEST_F(DbuctRpSrtMockLinkTest, LinkSetsNegInfPercolatesThenForwards) {
    rp.insert_active_goal(&grandparent);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&parent);
    rp.insert_active_goal(&sibling);
    // Build SRT topology without going through RP link (mock owns link slot).
    srt.link_srt_goal_batch_parent(&grandparent);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&parent, -2.0);
    rp.set_active_goal_value(&sibling, -5.0);
    EXPECT_EQ(rp.get(&grandparent), -2.0);

    bool linked = false;
    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent))
        .WillOnce([&](const goal_lineage*) {
            EXPECT_EQ(rp.get(&parent), kNegInf);
            EXPECT_EQ(rp.get(&grandparent), -5.0);
            linked = true;
        });
    rp.link_srt_goal_batch_parent(&parent);
    EXPECT_TRUE(linked);
    EXPECT_EQ(rp.get(&parent), kNegInf);
    EXPECT_EQ(rp.get(&grandparent), -5.0);
}

TEST_F(DbuctRpSrtFullyMockedTest, SetWithNullParentDoesNotIterateChildren) {
    EXPECT_CALL(insert, insert_active_goal(&g));
    rp.insert_active_goal(&g);
    EXPECT_CALL(get_parent, get_parent_goal(&g)).WillOnce(Return(nullptr));
    EXPECT_CALL(iterate_children, iterate_child_goals).Times(0);
    rp.set_active_goal_value(&g, -3.0);
    EXPECT_EQ(rp.get(&g), -3.0);
}

TEST_F(DbuctRpSrtFullyMockedTest, PercolateEarlyExitStopsWalkingAncestors) {
    EXPECT_CALL(insert, insert_active_goal(&p));
    EXPECT_CALL(insert, insert_active_goal(&c));
    EXPECT_CALL(insert, insert_active_goal(&s));
    rp.insert_active_goal(&p);
    rp.insert_active_goal(&c);
    rp.insert_active_goal(&s);

    EXPECT_CALL(get_parent, get_parent_goal(&s)).WillOnce(Return(&p));
    EXPECT_CALL(iterate_children, iterate_child_goals(&p))
        .WillOnce(Return(ByMove(two_children_sm(&c, &s))));
    rp.set_active_goal_value(&s, -2.0);

    EXPECT_CALL(get_parent, get_parent_goal(&c)).WillOnce(Return(&p));
    EXPECT_CALL(iterate_children, iterate_child_goals(&p))
        .WillOnce(Return(ByMove(two_children_sm(&c, &s))));
    EXPECT_CALL(get_parent, get_parent_goal(&p)).WillOnce(Return(nullptr));
    rp.set_active_goal_value(&c, -5.0);
    EXPECT_EQ(rp.get(&p), -2.0);

    EXPECT_CALL(get_parent, get_parent_goal(&c)).WillOnce(Return(&p));
    EXPECT_CALL(iterate_children, iterate_child_goals(&p))
        .WillOnce(Return(ByMove(two_children_sm(&c, &s))));
    EXPECT_CALL(get_parent, get_parent_goal(&p)).Times(0);
    rp.set_active_goal_value(&c, -4.0);
    EXPECT_EQ(rp.get(&p), -2.0);
}

TEST_F(DbuctRpSrtFullyMockedTest, LinkAssignsNegInfPercolatesThenForwards) {
    EXPECT_CALL(insert, insert_active_goal(&gp));
    EXPECT_CALL(insert, insert_active_goal(&p));
    EXPECT_CALL(insert, insert_active_goal(&s));
    rp.insert_active_goal(&gp);
    rp.insert_active_goal(&p);
    rp.insert_active_goal(&s);

    EXPECT_CALL(get_parent, get_parent_goal(&s)).WillOnce(Return(&gp));
    EXPECT_CALL(iterate_children, iterate_child_goals(&gp))
        .WillOnce(Return(ByMove(two_children_sm(&p, &s))));
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

TEST_F(DbuctRpSrtFullyMockedTest, LinkWithNoChildrenKeepsNegInfAfterForward) {
    EXPECT_CALL(insert, insert_active_goal(&p));
    rp.insert_active_goal(&p);

    InSequence seq;
    EXPECT_CALL(get_parent, get_parent_goal(&p)).WillOnce(Return(nullptr));
    EXPECT_CALL(link, link_srt_goal_batch_parent(&p));
    rp.link_srt_goal_batch_parent(&p);
    EXPECT_EQ(rp.get(&p), kNegInf);
}

TEST_F(DbuctRpSrtFullyMockedTest, PopFrameRestoresPreLinkScore) {
    EXPECT_CALL(insert, insert_active_goal(&p));
    rp.insert_active_goal(&p);
    EXPECT_EQ(rp.get(&p), 0.0);

    rp.push_frame();
    EXPECT_CALL(get_parent, get_parent_goal(&p)).WillOnce(Return(nullptr));
    EXPECT_CALL(link, link_srt_goal_batch_parent(&p));
    rp.link_srt_goal_batch_parent(&p);
    EXPECT_EQ(rp.get(&p), kNegInf);

    rp.pop_frame();
    EXPECT_EQ(rp.get(&p), 0.0);
}
