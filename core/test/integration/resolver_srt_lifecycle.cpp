// Integration: a real resolver driving the real SRT activation AND deactivation
// stack in one step.
//
// The seam is untested from both sides today. srt_resolver_order_invariance.cpp
// runs a real resolver but mocks its whole deactivation tail (goal deactivator,
// candidate deactivator, chosen-candidate map), so it only ever checks tree
// shape. unit/resolver.cpp mocks the SRT side. Nothing proves that after one
// resolve the goal is consistently gone from the active frontier, the SRT, the
// goal-expr map, the candidate-rule map and the candidate frame offsets --
// which is exactly where a ghost goal (present in one structure, absent from
// another) would hide and stall solution detection.
//
// Real: resolver, srt_subgoals_activator, subgoals_activator, goal_activator,
// srt_active_goals, srt_goal_deactivator, goal_candidates_deactivator,
// candidate_deactivator, goal_exprs, goal_candidate_rules,
// candidate_frame_offsets, chosen_goal_candidates, lineage_pool.
// Mocked (outside the slice): the rule database and candidate activation, which
// pulls in the querier and unification.

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/candidate_frame_offsets.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::UnorderedElementsAre;

namespace {

constexpr rule_id kFact = 0;
constexpr rule_id kExpand2 = 1;
constexpr uint32_t kRootFrameOffset = 100;

struct MockGetRule {
    MOCK_METHOD(const rule*, get_rule, (rule_id), (const));
};
struct MockGetResolutionRule {
    MOCK_METHOD(const rule*, get, (const resolution_lineage*), (const));
};
struct MockActivateGoalCandidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*));
};

using goal_activator_t = goal_activator<goal_exprs, goal_candidate_rules, srt_active_goals,
                                        candidate_frame_offsets, NiceMock<MockGetResolutionRule>>;
using subgoals_activator_t = subgoals_activator<lineage_pool, goal_activator_t,
                                                NiceMock<MockGetRule>,
                                                NiceMock<MockActivateGoalCandidates>>;
using srt_subgoals_activator_t =
    srt_subgoals_activator<srt_active_goals, srt_active_goals, subgoals_activator_t>;
using candidate_deactivator_t = candidate_deactivator<candidate_frame_offsets, goal_candidate_rules>;
using goal_candidates_deactivator_t =
    goal_candidates_deactivator<goal_candidate_rules, lineage_pool, candidate_deactivator_t>;
using srt_goal_deactivator_t = srt_goal_deactivator<goal_exprs, goal_candidate_rules>;
using resolver_t = resolver<srt_goal_deactivator_t, srt_subgoals_activator_t,
                            goal_candidates_deactivator_t, chosen_goal_candidates>;

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

struct ResolverSrtLifecycleIntegrationTest : public ::testing::Test {
    expr head_{expr::var{0}};
    expr body0_{expr::var{1}};
    expr body1_{expr::var{2}};
    rule fact_rule_{&head_, {}, 0};
    rule expand2_rule_{&head_, {&body0_, &body1_}, 0};
    std::array<rule, 2> rule_table_{fact_rule_, expand2_rule_};

    NiceMock<MockGetRule> get_rule;
    NiceMock<MockGetResolutionRule> get_resolution_rule;
    NiceMock<MockActivateGoalCandidates> activate_goal_candidates;

    lineage_pool pool;
    ra_rule_id_set_factory rule_factory;
    goal_exprs goal_exprs_;
    goal_candidate_rules goal_candidate_rules_{rule_factory};
    candidate_frame_offsets candidate_frame_offsets_;
    chosen_goal_candidates chosen_goal_candidates_;
    srt_active_goals active_goals;

    goal_activator_t goal_activator_{goal_exprs_, goal_candidate_rules_, active_goals,
                                     candidate_frame_offsets_, get_resolution_rule};
    subgoals_activator_t subgoals_activator_{pool, goal_activator_, get_rule,
                                             activate_goal_candidates};
    srt_subgoals_activator_t srt_subgoals_activator_{active_goals, active_goals,
                                                     subgoals_activator_};
    candidate_deactivator_t candidate_deactivator_{candidate_frame_offsets_, goal_candidate_rules_};
    goal_candidates_deactivator_t goal_candidates_deactivator_{goal_candidate_rules_, pool,
                                                               candidate_deactivator_};
    srt_goal_deactivator_t srt_goal_deactivator_{goal_exprs_, goal_candidate_rules_};
    resolver_t res{srt_goal_deactivator_, srt_subgoals_activator_, goal_candidates_deactivator_,
                   chosen_goal_candidates_};

    uint32_t next_frame_offset = kRootFrameOffset;

    void SetUp() override {
        ON_CALL(get_rule, get_rule(_))
            .WillByDefault([this](rule_id id) -> const rule* { return &rule_table_.at(id); });
        ON_CALL(get_resolution_rule, get(_))
            .WillByDefault([this](const resolution_lineage* rl) -> const rule* {
                return &rule_table_.at(rl->idx);
            });
        // Stands in for goal_candidates_activator: give the freshly activated
        // goal both rules as candidates, each with its own frame offset.
        ON_CALL(activate_goal_candidates, activate_goal_candidates(_))
            .WillByDefault([this](const goal_lineage* gl) {
                link_candidates(gl);
                return true;
            });
    }

    // Links both rules as candidates of gl and reserves a frame offset per
    // candidate resolution, the way candidate_activator does in production.
    void link_candidates(const goal_lineage* gl) {
        for (rule_id rid : {kFact, kExpand2}) {
            goal_candidate_rules_.link_goal_candidate(gl, rid);
            candidate_frame_offsets_.set(pool.make_resolution_lineage(gl, rid),
                                         next_frame_offset++);
        }
    }

    // Seeds a root goal the way the initial goal activator would.
    const goal_lineage* seed_root_goal(subgoal_id idx) {
        const goal_lineage* gl = pool.make_goal_lineage(nullptr, idx);
        goal_exprs_.set(gl, framed_expr{&head_, 0});
        goal_candidate_rules_.insert(gl);
        link_candidates(gl);
        active_goals.insert_active_goal(gl);
        active_goals.flush_srt_goal_batch();
        return gl;
    }
};

TEST_F(ResolverSrtLifecycleIntegrationTest, SuccessfulResolveLinksChildrenThenDeactivatesParent) {
    const goal_lineage* root = seed_root_goal(0);
    const resolution_lineage* rl = pool.make_resolution_lineage(root, kExpand2);
    const goal_lineage* child0 = pool.make_goal_lineage(rl, 0);
    const goal_lineage* child1 = pool.make_goal_lineage(rl, 1);

    ASSERT_TRUE(res.resolve(rl));

    // Children are the new frontier; the parent has become an interior SRT node.
    EXPECT_TRUE(active_goals.is_active_goal(child0));
    EXPECT_TRUE(active_goals.is_active_goal(child1));
    EXPECT_FALSE(active_goals.is_active_goal(root));
    EXPECT_EQ(active_goals.active_goals_size(), 2u);
    EXPECT_EQ(active_goals.get_parent_goal(child0), root);
    EXPECT_EQ(active_goals.get_parent_goal(child1), root);
    EXPECT_THAT(collect_goals(active_goals.iterate_child_goals(root)),
                UnorderedElementsAre(child0, child1));
    EXPECT_EQ(chosen_goal_candidates_.try_get(root), std::optional<rule_id>{kExpand2});
}

TEST_F(ResolverSrtLifecycleIntegrationTest, FactResolveRemovesGoalFromBothActiveFrontierAndSrt) {
    // A goal that is removed from one structure but left in the other is a ghost:
    // solution_detector would never fire, or would fire on a live goal.
    const goal_lineage* root = seed_root_goal(0);
    const resolution_lineage* rl = pool.make_resolution_lineage(root, kFact);

    ASSERT_TRUE(res.resolve(rl));

    EXPECT_FALSE(active_goals.is_active_goal(root));
    EXPECT_TRUE(active_goals.empty());
    EXPECT_EQ(active_goals.active_goals_size(), 0u);
    EXPECT_TRUE(collect_goals(active_goals.iterate_root_goals()).empty());
}

TEST_F(ResolverSrtLifecycleIntegrationTest, ResolveErasesGoalExprAndCandidateRulesForParent) {
    const goal_lineage* root = seed_root_goal(0);
    const resolution_lineage* rl_fact = pool.make_resolution_lineage(root, kFact);
    const resolution_lineage* rl_expand = pool.make_resolution_lineage(root, kExpand2);
    const goal_lineage* child0 = pool.make_goal_lineage(rl_expand, 0);

    ASSERT_TRUE(res.resolve(rl_expand));

    // Every trace of the resolved parent is gone, including the frame offsets of
    // the candidate it did NOT take.
    EXPECT_THROW(goal_exprs_.get(root), std::out_of_range);
    EXPECT_THROW(goal_candidate_rules_.get(root), std::out_of_range);
    EXPECT_THROW(candidate_frame_offsets_.get(rl_fact), std::out_of_range);
    EXPECT_THROW(candidate_frame_offsets_.get(rl_expand), std::out_of_range);

    // The children it produced are fully activated, carrying the parent
    // resolution's frame offset.
    EXPECT_EQ(goal_exprs_.get(child0).skeleton, &body0_);
    EXPECT_EQ(goal_exprs_.get(child0).frame_offset, kRootFrameOffset + 1);
    EXPECT_TRUE(goal_candidate_rules_.get(child0).contains(kExpand2));
}
