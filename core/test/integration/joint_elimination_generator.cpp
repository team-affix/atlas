// joint_elimination_generator integration — real CDCL then MHU constrain streams.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/lemma.hpp"
#include "functor_fixture.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

using TestUnifierFactory = unifier_factory<bind_map>;
using TestCdcl  = cdcl_elimination_generator<chosen_goal_candidates>;
using TestMhu   = mhu_elimination_generator<
    bind_map, bind_map_factory, unifier<bind_map>, TestUnifierFactory,
    lineage_pool, expr_pool, goal_candidate_rules>;
using TestJoint = joint_elimination_generator<TestCdcl, TestMhu>;

namespace {

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const resolution_lineage* v = sm.consume_yield();
            if (v != nullptr)
                out.push_back(v);
        }
    }
    return out;
}

}  // namespace

struct JointEliminationGeneratorIntegrationTest : public ::testing::Test {
    
    test_functors functors;
    trail t;
    globalizer g_;
    bind_map common{g_};
    lineage_pool lp;
    bind_map_factory bmf{g_};
    TestUnifierFactory uf{g_};
    ra_rule_id_set_factory ra_rule_id_set_factory_;
    goal_candidate_rules ggcr{ra_rule_id_set_factory_};
    std::optional<expr_pool> pool;
    chosen_goal_candidates chosen;
    std::optional<TestCdcl> cdcl;
    std::optional<TestMhu> mhu;
    std::optional<TestJoint> joint;

    JointEliminationGeneratorIntegrationTest() {
        pool.emplace();
        cdcl.emplace(chosen);
        mhu.emplace(common, lp, *pool, bmf, uf, ggcr);
        joint.emplace(*cdcl, *mhu);
    }
};

TEST_F(JointEliminationGeneratorIntegrationTest, ConstrainRunsCdclBeforeMhu) {
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
    resolution_lineage rl0{&gl0, 0};
    resolution_lineage rl1{&gl1, 1};

    expr goal{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {}}};

    ggcr.insert(&gl0);
    ggcr.link_goal_candidate(&gl0, rule_id{0});

    ASSERT_EQ(cdcl->learn(lemma{{&rl0, &rl1}}), std::nullopt);
    ASSERT_TRUE(mhu->try_add_head(&rl0, {&goal, 0}, {&head, 0}));

    auto elims = collect_elims(joint->constrain(&rl0));
    ASSERT_EQ(elims.size(), 1u);
    EXPECT_EQ(elims[0], &rl1);
}

TEST_F(JointEliminationGeneratorIntegrationTest, ConstrainYieldsMhuElimAfterCdclStreamEmpty) {
    expr goal_a{expr::var{0}};
    expr goal_b{expr::var{0}};
    expr head_a{expr::functor{functors.id("f"), {}}};
    expr head_b{expr::functor{functors.id("g"), {}}};

    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    ggcr.insert(gl_a);
    ggcr.insert(gl_b);
    ggcr.link_goal_candidate(gl_a, rule_id{0});
    ggcr.link_goal_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));

    EXPECT_THAT(collect_elims(joint->constrain(rl_a)), ElementsAre(rl_b));
}

TEST_F(JointEliminationGeneratorIntegrationTest, ConstrainYieldsCdclThenMhuElims) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    goal_lineage* gl0 = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl1 = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl0 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl0, rule_id{0}));
    resolution_lineage* rl1 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl1, rule_id{1}));

    resolution_lineage* rl_g_sibling =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl0, rule_id{1}));
    goal_lineage* gl_other = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 2));
    resolution_lineage* rl_other =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_other, rule_id{1}));
    ggcr.insert(gl0);
    ggcr.insert(gl_other);
    ggcr.link_goal_candidate(gl0, rule_id{0});
    ggcr.link_goal_candidate(gl0, rule_id{1});
    ggcr.link_goal_candidate(gl_other, rule_id{1});

    ASSERT_EQ(cdcl->learn(lemma{{rl0, rl1}}), std::nullopt);
    ASSERT_TRUE(mhu->try_add_head(rl0, {&goal, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g_sibling, {&goal, 0}, {&head_g, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_other, {&goal, 0}, {&head_g, 0}));

    EXPECT_THAT(collect_elims(joint->constrain(rl0)), ElementsAre(rl1, rl_other));
}

TEST_F(JointEliminationGeneratorIntegrationTest,
    ConstrainMayYieldSameCandidateTwiceWhenCdclAndMhuAgree) {
    /*
     * CDCL avoidance {rl0, rl1} and MHU f/g heads on the same rep both eliminate rl1 when
     * constraining rl0. Joint forwards both yields; elimination_router deduplicates on route.
     */
    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    goal_lineage* gl0 = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl1 = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl0 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl0, rule_id{0}));
    resolution_lineage* rl1 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl1, rule_id{0}));
    ggcr.insert(gl0);
    ggcr.insert(gl1);
    ggcr.link_goal_candidate(gl0, rule_id{0});
    ggcr.link_goal_candidate(gl1, rule_id{0});

    ASSERT_EQ(cdcl->learn(lemma{{rl0, rl1}}), std::nullopt);
    ASSERT_TRUE(mhu->try_add_head(rl0, {&goal, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl1, {&goal, 0}, {&head_g, 0}));

    EXPECT_THAT(collect_elims(joint->constrain(rl0)), ElementsAre(rl1, rl1));
}

TEST_F(JointEliminationGeneratorIntegrationTest, ConstrainYieldsNothingWhenBothStreamsEmpty) {
    expr goal_a{expr::var{0}};
    expr goal_b{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {}}};

    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{1}));
    ggcr.insert(gl);
    ggcr.link_goal_candidate(gl, rule_id{0});
    ggcr.link_goal_candidate(gl, rule_id{1});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head, 0}));

    EXPECT_THAT(collect_elims(joint->constrain(rl_a)), IsEmpty());
}

TEST_F(JointEliminationGeneratorIntegrationTest, CleanupRestoresCdclThroughJointConstrain) {
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
    resolution_lineage rl0{&gl0, 0};
    resolution_lineage rl1{&gl1, 1};

    expr goal{expr::functor{functors.id("f"), {}}};
    expr head{expr::functor{functors.id("f"), {}}};

    ggcr.insert(&gl0);
    ggcr.link_goal_candidate(&gl0, rule_id{0});

    ASSERT_EQ(cdcl->learn(lemma{{&rl0, &rl1}}), std::nullopt);
    ASSERT_TRUE(mhu->try_add_head(&rl0, {&goal, 0}, {&head, 0}));

    EXPECT_THAT(collect_elims(joint->constrain(&rl0)), ElementsAre(&rl1));
    EXPECT_THAT(collect_elims(cdcl->constrain(&rl0)), IsEmpty());

    t.push();
    EXPECT_THAT(collect_elims(cdcl->constrain(&rl0)), IsEmpty());
    t.pop();

    cdcl->cleanup();
    chosen.clear();
    mhu->clear_mhu_heads();
    ASSERT_TRUE(mhu->try_add_head(&rl0, {&goal, 0}, {&head, 0}));
    EXPECT_THAT(collect_elims(joint->constrain(&rl0)), ElementsAre(&rl1));
}
