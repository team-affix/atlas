#include <gtest/gtest.h>
#include <optional>
#include <vector>
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/trail.hpp"

struct MhuEliminationGeneratorIntegrationTest : public ::testing::Test {
    locator loc;
    trail t;
    bind_map common;
    lineage_pool lp;
    bind_map_factory bmf;
    overlay_bind_map_factory obmf;
    unifier_factory uf;
    goal_candidate_rules ggcr;
    std::optional<expr_pool> pool;
    std::optional<mhu_elimination_generator> mhu;

    MhuEliminationGeneratorIntegrationTest() {
        loc.bind_as<i_log_to_current_trail_frame>(t);
        loc.bind_as<i_bind_map>(common);
        loc.bind_as<i_bind_map_factory>(bmf);
        loc.bind_as<i_overlay_bind_map_factory>(obmf);
        loc.bind_as<i_unifier_factory>(uf);
        loc.bind_as<i_make_resolution_lineage>(lp);
        loc.bind_as<i_get_goal_candidate_rule_ids>(ggcr);
        pool.emplace(loc);
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(*pool);
        mhu.emplace(loc);
    }
};

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadThenConstrainAllowsReuse) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));

    auto sm = mhu->constrain(rl);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }
    EXPECT_TRUE(elims.empty());

    EXPECT_TRUE(mhu->try_add_head(rl, &goal, &head));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenUnifyFails) {
    expr goal{expr::functor{"f", {}}};
    expr head{expr::functor{"g", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    EXPECT_FALSE(mhu->try_add_head(rl, &goal, &head));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainOnSingleHeadYieldsNoElims) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));

    auto sm = mhu->constrain(rl);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }
    EXPECT_TRUE(elims.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesSeededBindingToCommon) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));

    auto sm = mhu->constrain(rl);
    while (!sm.done())
        sm.resume();

    const expr::functor& bound = std::get<expr::functor>(common.whnf(&goal)->content);
    EXPECT_EQ(bound.name, "f");
    EXPECT_TRUE(bound.args.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEliminatesHeadWithCollidingFunctorOnSameRep) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{"f", {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    ggcr.link_goal_candidate(gl_a, rule_id{0});

    expr goal_b{expr::var{0}};
    expr head_b{expr::functor{"g", {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    ggcr.link_goal_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_a, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));

    auto sm = mhu->constrain(rl_a);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }

    EXPECT_EQ(elims.size(), 1u);
    EXPECT_EQ(elims[0], rl_b);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotEliminateCompatibleHeadOnSameRep) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{"f", {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    ggcr.link_goal_candidate(gl_a, rule_id{0});

    expr goal_b{expr::var{0}};
    expr head_b{expr::functor{"f", {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    ggcr.link_goal_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_a, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));

    auto sm = mhu->constrain(rl_a);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }

    EXPECT_TRUE(elims.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotEliminateHeadWatchingDisjointRep) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{"f", {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    ggcr.link_goal_candidate(gl_a, rule_id{0});

    expr var2{expr::var{2}};
    expr goal_b{var2};
    expr head_b{expr::functor{"g", {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 2));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    ggcr.link_goal_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_a, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));

    auto sm = mhu->constrain(rl_a);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }

    EXPECT_TRUE(elims.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainDoesNotElimSiblingButEliminatesCrossGoalIncompatibleHead) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};

    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_rule0 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    resolution_lineage* rl_rule1 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{1}));
    ggcr.link_goal_candidate(gl, rule_id{0});
    ggcr.link_goal_candidate(gl, rule_id{1});

    goal_lineage* gl_other = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_other =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_other, rule_id{0}));
    ggcr.link_goal_candidate(gl_other, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_rule0, &goal, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_rule1, &goal, &head_g));
    ASSERT_TRUE(mhu->try_add_head(rl_other, &goal, &head_g));

    auto sm = mhu->constrain(rl_rule0);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }

    EXPECT_EQ(elims.size(), 1u);
    EXPECT_EQ(elims[0], rl_other);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainYieldsTwoElimsForThreeWayRepCollision) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{"f", {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    ggcr.link_goal_candidate(gl_a, rule_id{0});

    expr goal_b{expr::var{0}};
    expr head_b{expr::functor{"g", {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    ggcr.link_goal_candidate(gl_b, rule_id{0});

    expr goal_c{expr::var{0}};
    expr head_c{expr::functor{"h", {}}};
    goal_lineage* gl_c = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 2));
    resolution_lineage* rl_c =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_c, rule_id{0}));
    ggcr.link_goal_candidate(gl_c, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_a, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));
    ASSERT_TRUE(mhu->try_add_head(rl_c, &goal_c, &head_c));

    auto sm = mhu->constrain(rl_a);
    std::vector<const resolution_lineage*> elims;
    while (!sm.done()) {
        if (auto v = sm.resume())
            elims.push_back(v.value());
    }

    EXPECT_EQ(elims.size(), 2u);
    bool saw_b = false;
    bool saw_c = false;
    for (const resolution_lineage* elim : elims) {
        if (elim == rl_b)
            saw_b = true;
        if (elim == rl_c)
            saw_c = true;
    }
    EXPECT_TRUE(saw_b);
    EXPECT_TRUE(saw_c);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsOnOccursCheck) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {&goal}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    EXPECT_FALSE(mhu->try_add_head(rl, &goal, &head));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ClearMhuHeadsAllowsFreshTryAdd) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));
    mhu->clear_mhu_heads();
    EXPECT_TRUE(mhu->try_add_head(rl, &goal, &head));
}
