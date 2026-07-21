// dbuct_mhu_elimination_generator integration: real bind/unify/pools + frame restore.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <unordered_set>
#include <vector>
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/coroutine.hpp"
#include "functor_fixture.hpp"

using bind_map_t = dbuct_bind_map<globalizer>;
using bind_map_factory_t = dbuct_bind_map_factory<globalizer>;
using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;
using local_bind_map_pool_t = pool_allocator<bind_map_t>;
using test_dbuct_mhu_t = dbuct_mhu_elimination_generator<
    bind_map_t, bind_map_t, bind_map_t,
    local_bind_map_pool_t, local_bind_map_pool_t, local_bind_map_pool_t,
    bind_map_factory_t, unifier<globalizer, bind_map_t>, unifier_factory_t,
    lineage_pool, expr_pool, dbuct_goal_candidate_rules>;

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

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

} // namespace

struct DbuctMhuEliminationGeneratorIntegrationTest : public ::testing::Test {
    test_functors functors;
    globalizer g_;
    bind_map_t common{g_};
    lineage_pool lp;
    bind_map_factory_t bmf{g_};
    local_bind_map_pool_t bind_map_pool;
    unifier_factory_t uf{g_};
    ra_rule_id_set_factory ra_factory;
    dbuct_goal_candidate_rules ggcr{ra_factory};
    std::optional<expr_pool> pool;
    std::optional<test_dbuct_mhu_t> mhu;
    std::unordered_set<const goal_lineage*> registered_goals_;

    DbuctMhuEliminationGeneratorIntegrationTest() {
        pool.emplace();
        mhu.emplace(common, common, lp, *pool, bind_map_pool, bind_map_pool, bind_map_pool, bmf, uf, ggcr);
    }

    void ensure_goal_candidates(const goal_lineage* gl) {
        if (registered_goals_.insert(gl).second)
            ggcr.insert(gl);
    }

    void link_candidate(const goal_lineage* gl, rule_id rid) {
        ensure_goal_candidates(gl);
        ggcr.link_goal_candidate(gl, rid);
    }
};

TEST_F(DbuctMhuEliminationGeneratorIntegrationTest, ConstrainEliminatesCollidingHeadOnSameRep) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    link_candidate(gl_a, rule_id{0});

    expr goal_b{expr::var{0}};
    expr head_b{expr::functor{functors.id("g"), {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    link_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_b));
}

TEST_F(DbuctMhuEliminationGeneratorIntegrationTest, PopFrameRestoresHeadAfterRemove) {
    expr goal{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    mhu->push_frame();
    mhu->remove_head(rl);
    EXPECT_THROW(collect_elims(mhu->constrain(rl)), std::out_of_range);
    mhu->pop_frame();
    EXPECT_NO_THROW(collect_elims(mhu->constrain(rl)));
}

TEST_F(DbuctMhuEliminationGeneratorIntegrationTest, ConstrainOnCompatibleHeadsYieldsNothing) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    link_candidate(gl_a, rule_id{0});

    expr goal_b{expr::var{0}};
    expr head_b{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    link_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
}
