// Integration: srt_subgoals_activator driving the real dbuct_srt_active_goals.
//
// A failing activation returns before link_srt_goal_batch_parent and
// flush_srt_goal_batch, so it strands the subgoals it already inserted -- both as
// orphan roots in the tree and in the store's pending batch.
// unit/srt_subgoals_activator.cpp proves the early return against mocks;
// unit/dbuct_srt_active_goals.cpp proves the store's frame undo in isolation.
// Neither shows the two together, which is the invariant dbuct depends on: it
// activates initial goals once, outside the episode loop, so no per-episode clear
// ever runs and backtracking's frame pop is the only thing that can hand the next
// episode a clean store.
//
// Real: srt_subgoals_activator, dbuct_srt_active_goals.
// Mocked (outside the slice): the inner subgoals activator, which pulls in the rule
// database, goal activation and candidate activation.

#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "value_objects/lineage.hpp"

using ::testing::UnorderedElementsAre;

namespace {

struct MockInnerSubgoalsActivator {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*));
};

using srt_subgoals_activator_t = srt_subgoals_activator<
    dbuct_srt_active_goals, dbuct_srt_active_goals, MockInnerSubgoalsActivator>;

std::vector<const goal_lineage*> collect_goals(coroutine<const goal_lineage*, void> sm) {
    std::vector<const goal_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

}  // namespace

struct SrtSubgoalsActivatorFrameUndoIntegrationTest : public ::testing::Test {
    dbuct_srt_active_goals active_goals;
    MockInnerSubgoalsActivator inner;
    srt_subgoals_activator_t activator{active_goals, active_goals, inner};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 2};
    goal_lineage child1{nullptr, 3};
    goal_lineage orphan{nullptr, 4};
    resolution_lineage rl{&parent, 1};
};

TEST_F(SrtSubgoalsActivatorFrameUndoIntegrationTest, FailedActivationLeavesNoTraceAfterFramePop) {
    active_goals.push_frame();
    active_goals.insert_active_goal(&parent);
    active_goals.flush_srt_goal_batch();

    active_goals.push_frame();
    // Stands in for subgoals_activator: activate one subgoal, then fail on the next
    // because it has no candidate rules.
    EXPECT_CALL(inner, activate_subgoals_and_candidates(&rl))
        .WillOnce([this](const resolution_lineage*) {
            active_goals.insert_active_goal(&orphan);
            return false;
        });
    EXPECT_FALSE(activator.activate_subgoals_and_candidates(&rl));
    EXPECT_TRUE(active_goals.is_active_goal(&orphan));

    active_goals.pop_frame();

    EXPECT_FALSE(active_goals.is_active_goal(&orphan));
    EXPECT_TRUE(active_goals.is_active_goal(&parent));
    EXPECT_EQ(active_goals.active_goals_size(), 1u);
    EXPECT_THAT(collect_goals(active_goals.iterate_root_goals()), UnorderedElementsAre(&parent));

    // The pending batch is clean too, so the next episode's first link adopts only
    // the goals that batch inserted. Two children: one would series-reduce the
    // parent away and hide a stale third member.
    active_goals.insert_active_goal(&child0);
    active_goals.insert_active_goal(&child1);
    active_goals.link_srt_goal_batch_parent(&parent);
    active_goals.flush_srt_goal_batch();

    EXPECT_FALSE(active_goals.is_active_goal(&parent));
    EXPECT_EQ(active_goals.active_goals_size(), 2u);
    EXPECT_THAT(collect_goals(active_goals.iterate_child_goals(&parent)),
                UnorderedElementsAre(&child0, &child1));
}
