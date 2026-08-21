// Integration: dbuct_joint_elimination_generator over a REAL
// dbuct_cdcl_elimination_generator and a REAL dbuct_mhu_elimination_generator.
//
// The dbuct joint generator exists solely to add one obligation the shared
// joint_elimination_generator does not have: every CDCL-forced elimination must
// also drop that lineage's head from the MHU, because a goal CDCL forces out can
// no longer be a live MHU head. The unit test for this component mocks BOTH
// generators, so it can only observe that remove_head was called -- nothing there
// proves a real MHU actually forgets the head, nor that it happens before the MHU
// runs its own constrain.
//
// The sharpest contrast is DuplicateEliminationFromBothStreamsYieldsOnce: the
// non-dbuct sibling deliberately yields the same candidate TWICE when CDCL and
// MHU agree (see integration/joint_elimination_generator.cpp,
// ConstrainMayYieldSameCandidateTwiceWhenCdclAndMhuAgree). Here the head removal
// means the MHU can no longer re-report it, so exactly one yield is correct.
//
// Only the MCTS frame depth is mocked; it belongs to the MCTS sim, outside the slice.

#include <optional>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_joint_elimination_generator.hpp"
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "value_objects/lineage.hpp"
#include "functor_fixture.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::ReturnPointee;

namespace {

struct MockGetMctsFrameDepth {
    MOCK_METHOD(size_t, depth, (), (const));
};

using bind_map_t = dbuct_bind_map<globalizer>;
using bind_map_factory_t = dbuct_bind_map_factory<globalizer>;
using unifier_t = unifier<globalizer, bind_map_t>;
using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;
using local_bind_map_pool_t = pool_allocator<bind_map_t>;
using boundary_t =
    dbuct_avoidance_unit_boundary<dbuct_nearest_decision, NiceMock<MockGetMctsFrameDepth>>;

using mhu_t = dbuct_mhu_elimination_generator<
    bind_map_t, bind_map_t, bind_map_t,
    local_bind_map_pool_t, local_bind_map_pool_t, local_bind_map_pool_t,
    bind_map_factory_t, unifier_t, unifier_factory_t,
    lineage_pool, expr_pool, dbuct_goal_candidate_rules>;

using cdcl_t = dbuct_cdcl_elimination_generator<
    dbuct_chosen_goal_candidates, boundary_t, dbuct_decision_memory,
    boundary_t, boundary_t, boundary_t>;

using joint_t = dbuct_joint_elimination_generator<cdcl_t, mhu_t>;

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

}  // namespace

struct DbuctJointEliminationGeneratorIntegrationTest : public ::testing::Test {
    test_functors functors;

    size_t mcts_frame_depth = 1;
    NiceMock<MockGetMctsFrameDepth> get_mcts_frame_depth;

    globalizer g;
    bind_map_t bind_map{g};
    bind_map_factory_t bind_map_factory{g};
    local_bind_map_pool_t bind_map_pool;
    unifier_factory_t unifier_factory_{g};

    lineage_pool lineage_pool_;
    expr_pool expr_pool_;
    ra_rule_id_set_factory rule_factory;

    dbuct_goal_candidate_rules goal_candidate_rules{rule_factory};
    dbuct_chosen_goal_candidates chosen_goal_candidates;
    dbuct_decision_memory decision_memory;
    dbuct_nearest_decision nearest_decision;
    boundary_t avoidance_unit_boundary{nearest_decision, get_mcts_frame_depth};

    mhu_t mhu{bind_map, bind_map, lineage_pool_, expr_pool_,
              bind_map_pool, bind_map_pool, bind_map_pool,
              bind_map_factory, unifier_factory_, goal_candidate_rules};
    cdcl_t cdcl{chosen_goal_candidates, avoidance_unit_boundary, decision_memory,
                avoidance_unit_boundary, avoidance_unit_boundary, avoidance_unit_boundary};
    joint_t joint{cdcl, mhu};

    // Decisions on distinct root goals, interned so the MHU's own
    // make_resolution_lineage returns these exact pointers.
    const goal_lineage* gl1 = lineage_pool_.make_goal_lineage(nullptr, 0);
    const goal_lineage* gl2 = lineage_pool_.make_goal_lineage(nullptr, 1);
    const goal_lineage* gl3 = lineage_pool_.make_goal_lineage(nullptr, 2);
    const resolution_lineage* d1 = lineage_pool_.make_resolution_lineage(gl1, rule_id{0});
    const resolution_lineage* d2 = lineage_pool_.make_resolution_lineage(gl2, rule_id{0});
    const resolution_lineage* d3 = lineage_pool_.make_resolution_lineage(gl3, rule_id{0});

    expr var0{expr::var{0}};
    expr func_f{expr::functor{functors.id("f"), {}}};
    expr func_g{expr::functor{functors.id("g"), {}}};

    void SetUp() override {
        ON_CALL(get_mcts_frame_depth, depth())
            .WillByDefault(ReturnPointee(&mcts_frame_depth));
        for (const goal_lineage* gl : {gl1, gl2, gl3}) {
            goal_candidate_rules.insert(gl);
            goal_candidate_rules.link_goal_candidate(gl, rule_id{0});
        }
    }

    // Leaves a CDCL avoidance over {d3, d2, d1} armed, so constrain(d3) forces d2.
    // Mirrors production: learn at a terminal state, then unwind past the unit
    // boundary so pop_frame arms the avoidance instead of emitting it.
    void arm_cdcl_avoidance_forcing_d2_from_d3() {
        mcts_frame_depth = 1;
        decision_memory.record_decision(d1);
        avoidance_unit_boundary.log_decision(d1);

        cdcl.push_frame();
        avoidance_unit_boundary.push_frame();
        mcts_frame_depth = 2;
        decision_memory.record_decision(d2);
        avoidance_unit_boundary.log_decision(d2);

        cdcl.push_frame();
        avoidance_unit_boundary.push_frame();
        mcts_frame_depth = 3;
        decision_memory.record_decision(d3);
        avoidance_unit_boundary.log_decision(d3);

        cdcl.learn();
        // d1 is already committed to its avoidance member, so the watcher scan
        // runs off the end of the member list and the avoidance goes unit.
        chosen_goal_candidates.set(gl1, d1->idx);

        avoidance_unit_boundary.pop_frame();
        collect_elims(cdcl.pop_frame());
        avoidance_unit_boundary.pop_frame();
        collect_elims(cdcl.pop_frame());
    }
};

TEST_F(DbuctJointEliminationGeneratorIntegrationTest, CdclEliminationRemovesMhuHeadBeforeMhuConstrains) {
    arm_cdcl_avoidance_forcing_d2_from_d3();

    // Compatible heads: the MHU stream on its own would eliminate nothing, so the
    // only elimination can come from CDCL -- and its head must be gone afterwards.
    ASSERT_TRUE(mhu.try_add_head(d3, {&var0, 0}, {&func_f, 0}));
    ASSERT_TRUE(mhu.try_add_head(d2, {&var0, 0}, {&func_f, 0}));

    EXPECT_THAT(collect_elims(joint.constrain(d3)), ElementsAre(d2));

    // d2 is no longer a live MHU head.
    EXPECT_THROW(collect_elims(mhu.constrain(d2)), std::out_of_range);
}

TEST_F(DbuctJointEliminationGeneratorIntegrationTest, MhuStreamStillYieldsWhenCdclYieldsNothing) {
    // No avoidance learned, so the CDCL stream is empty; the MHU must still run.
    ASSERT_TRUE(mhu.try_add_head(d3, {&var0, 0}, {&func_f, 0}));
    ASSERT_TRUE(mhu.try_add_head(d2, {&var0, 0}, {&func_g, 0}));

    EXPECT_THAT(collect_elims(joint.constrain(d3)), ElementsAre(d2));
}

TEST_F(DbuctJointEliminationGeneratorIntegrationTest, DuplicateEliminationFromBothStreamsYieldsOnce) {
    arm_cdcl_avoidance_forcing_d2_from_d3();

    // Colliding heads on representative var0: CDCL forces d2 out AND the MHU would
    // independently eliminate d2 for the f/g clash. Because the CDCL yield removes
    // d2's head first, the MHU stream can no longer re-report it.
    ASSERT_TRUE(mhu.try_add_head(d3, {&var0, 0}, {&func_f, 0}));
    ASSERT_TRUE(mhu.try_add_head(d2, {&var0, 0}, {&func_g, 0}));

    EXPECT_THAT(collect_elims(joint.constrain(d3)), ElementsAre(d2));
}

TEST_F(DbuctJointEliminationGeneratorIntegrationTest, BothStreamsEmptyCompletesWithoutYielding) {
    ASSERT_TRUE(mhu.try_add_head(d3, {&var0, 0}, {&func_f, 0}));
    ASSERT_TRUE(mhu.try_add_head(d2, {&var0, 0}, {&func_f, 0}));

    EXPECT_THAT(collect_elims(joint.constrain(d3)), IsEmpty());
}
