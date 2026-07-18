// Integration: rp_fewer_candidate_srt_subgoals_activator score pipeline on resolve.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
#include <vector>
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rp_compute_fewer_candidate_goal_value.hpp"
#include "infrastructure/rp_fewer_candidate_srt_subgoals_activator.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::Return;
using ::testing::StrictMock;

namespace {

constexpr double kNegInf = -std::numeric_limits<double>::infinity();

using rp_srt_t = rp_srt_active_goals<
    srt_active_goals, srt_active_goals, srt_active_goals, srt_active_goals,
    srt_active_goals>;
using compute_t = rp_compute_fewer_candidate_goal_value<goal_candidate_rules>;

// Hand-rolls child insert + candidate linking for the resolve slice (not a second
// production infra type).
struct HandRollSubgoalsActivator {
    srt_active_goals* srt;
    rp_srt_t* rp;
    goal_candidate_rules* rules;
    std::vector<std::pair<const goal_lineage*, std::vector<rule_id>>> children;
    bool succeed;

    bool activate_subgoals_and_candidates(const resolution_lineage*) {
        if (!succeed) return false;
        for (const auto& [child, ids] : children) {
            rp->insert_active_goal(child);
            rules->insert(child);
            for (rule_id id : ids)
                rules->link_goal_candidate(child, id);
        }
        return true;
    }
};

using activator_t = rp_fewer_candidate_srt_subgoals_activator<
    HandRollSubgoalsActivator, rp_srt_t, srt_active_goals, srt_active_goals,
    compute_t, rp_srt_t>;

}  // namespace

struct RpFewerCandidateResolveScoresIntegrationTest : public ::testing::Test {
    srt_active_goals srt;
    rp_srt_t rp{srt, srt, srt, srt, srt};
    ra_rule_id_set_factory factory;
    goal_candidate_rules rules{factory};
    compute_t compute{rules};
    HandRollSubgoalsActivator inner{&srt, &rp, &rules, {}, true};
    activator_t activator{inner, rp, srt, srt, compute, rp};

    goal_lineage gp{nullptr, 0};
    goal_lineage p{nullptr, 1};
    goal_lineage sibling{nullptr, 2};
    goal_lineage c0{nullptr, 3};
    goal_lineage c1{nullptr, 4};
    resolution_lineage rl{&p, 0};

    void insert_leaf(const goal_lineage* gl) {
        rp.insert_active_goal(gl);
        srt.flush_srt_goal_batch();
    }
};

TEST_F(RpFewerCandidateResolveScoresIntegrationTest, TwoChildrenDifferentCandidateCounts) {
    insert_leaf(&p);
    inner.children = {
        {&c0, {0, 1}},
        {&c1, {0, 1, 2, 3, 4}},
    };

    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));

    EXPECT_EQ(rp.get(&c0), -2.0);
    EXPECT_EQ(rp.get(&c1), -5.0);
    EXPECT_FALSE(srt.is_active_goal(&p));
    EXPECT_EQ(rp.get(&p), -2.0);
    EXPECT_NE(rp.get(&p), kNegInf);
}

TEST_F(RpFewerCandidateResolveScoresIntegrationTest, FactEmptyBodyOnlyNegInfPercolate) {
    insert_leaf(&gp);
    rp.insert_active_goal(&p);
    rp.insert_active_goal(&sibling);
    rp.link_srt_goal_batch_parent(&gp);
    srt.flush_srt_goal_batch();
    rp.set_active_goal_value(&sibling, -1.0);
    rp.set_active_goal_value(&p, 0.0);
    EXPECT_EQ(rp.get(&gp), 0.0);

    inner.children = {};
    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));

    EXPECT_EQ(rp.get(&p), kNegInf);
    EXPECT_EQ(rp.get(&gp), -1.0);
}

TEST_F(RpFewerCandidateResolveScoresIntegrationTest, ActivateFailureSkipsLinkAndScores) {
    insert_leaf(&p);
    const double before = rp.get(&p);
    EXPECT_TRUE(srt.is_active_goal(&p));

    inner.succeed = false;
    inner.children = {{&c0, {0, 1}}};
    EXPECT_FALSE(activator.activate_subgoals_and_candidates(&rl));

    EXPECT_TRUE(srt.is_active_goal(&p));
    EXPECT_EQ(rp.get(&p), before);
}
