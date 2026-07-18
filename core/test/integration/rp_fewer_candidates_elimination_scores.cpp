// Integration: rp_fewer_candidates_elimination_router refreshes scores on eliminate.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rp_compute_fewer_candidate_goal_value.hpp"
#include "infrastructure/rp_fewer_candidates_elimination_router.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/elimination_result.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;

namespace {

constexpr double kNegInf = -std::numeric_limits<double>::infinity();

struct MockUnsetCandidateFrameOffset {
    MOCK_METHOD(void, unset, (const resolution_lineage*));
};

using rp_srt_t = rp_srt_active_goals<
    srt_active_goals, srt_active_goals, srt_active_goals, srt_active_goals,
    srt_active_goals>;
using compute_t = rp_compute_fewer_candidate_goal_value<goal_candidate_rules>;
using deactivator_t =
    candidate_deactivator<MockUnsetCandidateFrameOffset, goal_candidate_rules>;
using elim_router_t = elimination_router<
    goal_candidate_rules, srt_active_goals, elimination_backlog, deactivator_t>;
using rp_elim_t = rp_fewer_candidates_elimination_router<
    elim_router_t, compute_t, rp_srt_t>;

}  // namespace

struct RpFewerCandidatesEliminationScoresIntegrationTest : public ::testing::Test {
    srt_active_goals srt;
    rp_srt_t rp{srt, srt, srt, srt, srt};
    ra_rule_id_set_factory factory;
    goal_candidate_rules rules{factory};
    compute_t compute{rules};
    NiceMock<MockUnsetCandidateFrameOffset> unset_cfo;
    deactivator_t deactivator{unset_cfo, rules};
    elimination_backlog backlog;
    elim_router_t elim_router{rules, srt, backlog, deactivator};
    rp_elim_t router{elim_router, compute, rp};

    goal_lineage g{nullptr, 0};
    goal_lineage p{nullptr, 1};
    goal_lineage s{nullptr, 2};

    void insert_leaf(const goal_lineage* gl) {
        rp.insert_active_goal(gl);
        srt.flush_srt_goal_batch();
    }

    void register_candidates(const goal_lineage* gl, std::initializer_list<rule_id> ids) {
        rules.insert(gl);
        for (rule_id id : ids)
            rules.link_goal_candidate(gl, id);
    }
};

TEST_F(RpFewerCandidatesEliminationScoresIntegrationTest, EliminatedDropsScoreByOne) {
    insert_leaf(&g);
    register_candidates(&g, {0, 1, 2});
    rp.set_active_goal_value(&g, -3.0);

    resolution_lineage rl{&g, 1};
    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
    EXPECT_EQ(rules.get(&g).size(), 2u);
    EXPECT_EQ(rp.get(&g), -2.0);
}

TEST_F(RpFewerCandidatesEliminationScoresIntegrationTest, EliminatedPercolatesToParent) {
    insert_leaf(&p);
    rp.insert_active_goal(&g);
    rp.insert_active_goal(&s);
    rp.link_srt_goal_batch_parent(&p);
    srt.flush_srt_goal_batch();

    register_candidates(&g, {0, 1});
    rp.set_active_goal_value(&g, -2.0);
    rp.set_active_goal_value(&s, -5.0);
    EXPECT_EQ(rp.get(&p), -2.0);

    resolution_lineage rl{&g, 0};
    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
    EXPECT_EQ(rp.get(&g), -1.0);
    EXPECT_EQ(rp.get(&p), -1.0);
}

TEST_F(RpFewerCandidatesEliminationScoresIntegrationTest, EliminateUntilEmptyYieldsZero) {
    insert_leaf(&g);
    register_candidates(&g, {0});
    rp.set_active_goal_value(&g, -1.0);

    resolution_lineage rl{&g, 0};
    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
    EXPECT_EQ(rp.get(&g), 0.0);
}

TEST_F(RpFewerCandidatesEliminationScoresIntegrationTest, AlreadyDeactivatedDoesNotChangeScore) {
    insert_leaf(&g);
    register_candidates(&g, {0, 1});
    rp.set_active_goal_value(&g, -2.0);

    resolution_lineage rl{&g, 9};  // never linked
    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
    EXPECT_EQ(rp.get(&g), -2.0);
}

TEST_F(RpFewerCandidatesEliminationScoresIntegrationTest, BacklogWhenInactiveDoesNotChangeScore) {
    insert_leaf(&p);
    rp.insert_active_goal(&g);
    rp.insert_active_goal(&s);
    rp.link_srt_goal_batch_parent(&p);
    srt.flush_srt_goal_batch();
    // P is inactive (not a leaf); G is active. Route against P.
    register_candidates(&p, {0});
    rp.set_active_goal_value(&p, -2.0);

    resolution_lineage rl{&p, 0};
    EXPECT_FALSE(srt.is_active_goal(&p));
    EXPECT_EQ(router.route(&rl), elimination_result::added_to_backlog);
    EXPECT_EQ(rp.get(&p), -2.0);
    (void)kNegInf;
}
