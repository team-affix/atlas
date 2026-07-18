// Integration: rp_srt_active_goals score topology over real srt_active_goals.

#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/lineage.hpp"

namespace {

constexpr double kNegInf = -std::numeric_limits<double>::infinity();

using rp_srt_t = rp_srt_active_goals<
    srt_active_goals, srt_active_goals, srt_active_goals, srt_active_goals,
    srt_active_goals>;

}  // namespace

struct RpSrtActiveGoalsTopologyIntegrationTest : public ::testing::Test {
    srt_active_goals srt;
    rp_srt_t rp{srt, srt, srt, srt, srt};

    goal_lineage r{nullptr, 0};
    goal_lineage m{nullptr, 1};
    goal_lineage l0{nullptr, 2};
    goal_lineage l1{nullptr, 3};
    goal_lineage rs{nullptr, 11};  // sibling so R stays branched
    goal_lineage gp{nullptr, 4};
    goal_lineage a{nullptr, 5};
    goal_lineage b{nullptr, 6};
    goal_lineage p{nullptr, 7};
    goal_lineage c{nullptr, 8};
    goal_lineage s{nullptr, 12};  // sibling under GP
    goal_lineage c0{nullptr, 9};
    goal_lineage c1{nullptr, 10};

    void insert_leaf(const goal_lineage* gl) {
        rp.insert_active_goal(gl);
        srt.flush_srt_goal_batch();
    }
};

TEST_F(RpSrtActiveGoalsTopologyIntegrationTest, MultiLevelSetPercolatesMax) {
    insert_leaf(&r);
    rp.insert_active_goal(&m);
    rp.insert_active_goal(&rs);
    rp.link_srt_goal_batch_parent(&r);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&l0);
    rp.insert_active_goal(&l1);
    rp.link_srt_goal_batch_parent(&m);
    srt.flush_srt_goal_batch();

    // Sibling keeps R branched under SRT; score it out so R tracks M's max.
    rp.set_active_goal_value(&rs, kNegInf);

    rp.set_active_goal_value(&l0, -2.0);
    rp.set_active_goal_value(&l1, -5.0);
    EXPECT_EQ(rp.get(&m), -2.0);
    EXPECT_EQ(rp.get(&r), -2.0);

    rp.set_active_goal_value(&l0, -8.0);
    EXPECT_EQ(rp.get(&m), -5.0);
    EXPECT_EQ(rp.get(&r), -5.0);
}

TEST_F(RpSrtActiveGoalsTopologyIntegrationTest, LinkNegInfLetsSiblingWinAtGrandparent) {
    insert_leaf(&gp);
    rp.insert_active_goal(&a);
    rp.insert_active_goal(&b);
    rp.link_srt_goal_batch_parent(&gp);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&a, -1.0);
    rp.set_active_goal_value(&b, -4.0);
    EXPECT_EQ(rp.get(&gp), -1.0);

    rp.link_srt_goal_batch_parent(&a);
    srt.flush_srt_goal_batch();

    EXPECT_EQ(rp.get(&a), kNegInf);
    EXPECT_EQ(rp.get(&gp), -4.0);
    EXPECT_FALSE(srt.is_active_goal(&a));
    EXPECT_TRUE(srt.is_active_goal(&b));
}

TEST_F(RpSrtActiveGoalsTopologyIntegrationTest, NullaryLinkOnlyChildMakesGrandparentNegInf) {
    // SRT forbids a lasting unary GP→A edge, so B is a scaffold sibling scored at
    // -inf: A is the only finite child (GP==-3), then nullary A percolates GP to -inf.
    insert_leaf(&gp);
    rp.insert_active_goal(&a);
    rp.insert_active_goal(&b);
    rp.link_srt_goal_batch_parent(&gp);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&a, -3.0);
    rp.set_active_goal_value(&b, kNegInf);
    EXPECT_EQ(rp.get(&gp), -3.0);

    rp.link_srt_goal_batch_parent(&a);
    srt.flush_srt_goal_batch();

    EXPECT_EQ(rp.get(&a), kNegInf);
    EXPECT_EQ(rp.get(&gp), kNegInf);
    EXPECT_FALSE(srt.is_active_goal(&a));
}

TEST_F(RpSrtActiveGoalsTopologyIntegrationTest, UnaryAbsorbChildSetPercolatesToGrandparent) {
    insert_leaf(&gp);
    rp.insert_active_goal(&p);
    rp.insert_active_goal(&s);
    rp.link_srt_goal_batch_parent(&gp);
    srt.flush_srt_goal_batch();

    rp.insert_active_goal(&c);
    rp.link_srt_goal_batch_parent(&p);
    srt.flush_srt_goal_batch();

    EXPECT_FALSE(srt.is_active_goal(&p));
    EXPECT_TRUE(srt.is_active_goal(&c));
    EXPECT_EQ(srt.get_parent_goal(&c), &gp);

    rp.set_active_goal_value(&s, -10.0);
    rp.set_active_goal_value(&c, -6.0);
    EXPECT_EQ(rp.get(&c), -6.0);
    EXPECT_EQ(rp.get(&gp), -6.0);
    EXPECT_EQ(rp.get(&p), kNegInf);
}

TEST_F(RpSrtActiveGoalsTopologyIntegrationTest, MultiChildLinkThenSetsOverwriteNegInf) {
    insert_leaf(&p);
    EXPECT_EQ(rp.get(&p), 0.0);

    rp.insert_active_goal(&c0);
    rp.insert_active_goal(&c1);
    rp.link_srt_goal_batch_parent(&p);
    EXPECT_EQ(rp.get(&p), kNegInf);
    srt.flush_srt_goal_batch();

    rp.set_active_goal_value(&c0, -2.0);
    rp.set_active_goal_value(&c1, -5.0);
    EXPECT_EQ(rp.get(&p), -2.0);
}

TEST_F(RpSrtActiveGoalsTopologyIntegrationTest, ClearRemovesScoresAndTree) {
    insert_leaf(&a);
    rp.set_active_goal_value(&a, -3.0);
    rp.clear_active_goals();
    EXPECT_TRUE(srt.empty());
    EXPECT_THROW(rp.get(&a), std::out_of_range);
}
