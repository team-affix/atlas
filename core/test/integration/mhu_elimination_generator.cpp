// mhu_elimination_generator integration — real unify/MHU slice (trail, bind_map, factories, …).
// Cross-goal cases check rebase-driven elimination when shared reps become inconsistent across
// heads, and that partial rep constraints (e.g. g(B) only) do not spuriously eliminate other heads.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <stdexcept>
#include <vector>
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

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

void expect_whnf_functor(bind_map& bm, const expr* e, const char* name, size_t argc) {
    const auto& f = std::get<expr::functor>(bm.whnf(e)->content);
    EXPECT_EQ(f.name, name);
    EXPECT_EQ(f.args.size(), argc);
}

} // namespace

struct MhuEliminationGeneratorIntegrationTest : public ::testing::Test {
    locator loc;
    trail t;
    bind_map common;
    lineage_pool lp;
    bind_map_factory bmf;
    unifier_factory uf;
    goal_candidate_rules ggcr;
    std::optional<expr_pool> pool;
    std::optional<mhu_elimination_generator> mhu;

    MhuEliminationGeneratorIntegrationTest() {
        loc.bind_as<i_log_to_current_trail_frame>(t);
        loc.bind_as<i_bind_map>(common);
        loc.bind_as<i_bind_map_factory>(bmf);
        loc.bind_as<i_unifier_factory>(uf);
        loc.bind_as<i_make_resolution_lineage>(lp);
        loc.bind_as<i_get_goal_candidate_rule_ids>(ggcr);
        pool.emplace(loc);
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(*pool);
        mhu.emplace(loc);
    }

    size_t rules_for(const goal_lineage* gl) const { return ggcr.get(gl).size(); }
};

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadThenConstrainAllowsReuse) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_EQ(common.whnf(&goal), &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(common, &goal, "f", 0);

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
    EXPECT_EQ(common.whnf(&goal), &goal);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesSeededBindingToCommon) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_EQ(common.whnf(&goal), &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(common, &goal, "f", 0);
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
    EXPECT_EQ(common.whnf(&goal_a), &goal_a);
    EXPECT_EQ(common.whnf(&goal_b), &goal_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_b));
    expect_whnf_functor(common, &goal_a, "f", 0);
    expect_whnf_functor(common, &goal_b, "f", 0);
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
    EXPECT_EQ(common.whnf(&goal_a), &goal_a);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(common, &goal_a, "f", 0);
    expect_whnf_functor(common, &goal_b, "f", 0);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_b)), IsEmpty());
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
    EXPECT_EQ(common.whnf(&goal_a), &goal_a);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(common, &goal_a, "f", 0);
    EXPECT_EQ(common.whnf(&var2), &var2);
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
    EXPECT_EQ(rules_for(gl), 2u);

    goal_lineage* gl_other = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_other =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_other, rule_id{1}));
    ggcr.link_goal_candidate(gl_other, rule_id{1});
    EXPECT_EQ(rules_for(gl_other), 1u);

    ASSERT_TRUE(mhu->try_add_head(rl_rule0, &goal, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_rule1, &goal, &head_g));
    ASSERT_TRUE(mhu->try_add_head(rl_other, &goal, &head_g));
    EXPECT_EQ(common.whnf(&goal), &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_rule0)), ElementsAre(rl_other));
    expect_whnf_functor(common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainFxXHeadEliminatesGroundGHeadOnSharedReps) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{"f", {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};

    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr goal_g{expr::functor{"g", {&var_a, &var_b}}};
    expr head_g{expr::functor{"g", {&abc, &_123}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_id{0}));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_id{0}));
    ggcr.link_goal_candidate(gl_f, rule_id{0});
    ggcr.link_goal_candidate(gl_g, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal_g, &head_g));
    ASSERT_EQ(common.whnf(&var_a), &var_a);
    ASSERT_EQ(common.whnf(&var_b), &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), ElementsAre(rl_g));
    EXPECT_EQ(common.whnf(&var_a), common.whnf(&var_b));
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainGroundGHeadEliminatesFxXHeadOnSharedReps) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;
    rule_id rule_f_id = rule_id{0};
    rule_id rule_g_id = rule_id{1};

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{"f", {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};

    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr goal_g{expr::functor{"g", {&var_a, &var_b}}};
    expr head_g{expr::functor{"g", {&abc, &_123}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_f_id));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_g_id));
    ggcr.link_goal_candidate(gl_f, rule_f_id);
    ggcr.link_goal_candidate(gl_g, rule_g_id);

    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal_g, &head_g));
    ASSERT_EQ(common.whnf(&var_a), &var_a);
    ASSERT_EQ(common.whnf(&var_b), &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), ElementsAre(rl_f));
    const auto& whnf_a = std::get<expr::functor>(common.whnf(&var_a)->content);
    const auto& whnf_b = std::get<expr::functor>(common.whnf(&var_b)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "123");
    EXPECT_TRUE(whnf_b.args.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainGroundGHeadDoesNotEliminateFxXHead) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{"f", {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};

    expr abc{expr::functor{"abc", {}}};
    expr goal_g{expr::functor{"g", {&var_b}}};
    expr head_g{expr::functor{"g", {&abc}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_id{0}));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_id{0}));
    ggcr.link_goal_candidate(gl_f, rule_id{0});
    ggcr.link_goal_candidate(gl_g, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal_g, &head_g));
    ASSERT_EQ(common.whnf(&var_a), &var_a);
    ASSERT_EQ(common.whnf(&var_b), &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    const auto& whnf_b = std::get<expr::functor>(common.whnf(&var_b)->content);
    EXPECT_EQ(whnf_b.name, "abc");
    EXPECT_TRUE(whnf_b.args.empty());
    EXPECT_EQ(common.whnf(&var_a), &var_a);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainFxXHeadAfterGroundGHeadBindsAWithoutEliminatingG) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{"f", {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};

    expr abc{expr::functor{"abc", {}}};
    expr goal_g{expr::functor{"g", {&var_b}}};
    expr head_g{expr::functor{"g", {&abc}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_id{0}));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_id{0}));
    ggcr.link_goal_candidate(gl_f, rule_id{0});
    ggcr.link_goal_candidate(gl_g, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal_g, &head_g));
    ASSERT_EQ(common.whnf(&var_a), &var_a);
    ASSERT_EQ(common.whnf(&var_b), &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    const auto& whnf_b_after_g = std::get<expr::functor>(common.whnf(&var_b)->content);
    EXPECT_EQ(whnf_b_after_g.name, "abc");
    EXPECT_TRUE(whnf_b_after_g.args.empty());
    EXPECT_EQ(common.whnf(&var_a), &var_a);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), IsEmpty());
    const auto& whnf_a = std::get<expr::functor>(common.whnf(&var_a)->content);
    const auto& whnf_b = std::get<expr::functor>(common.whnf(&var_b)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "abc");
    EXPECT_TRUE(whnf_b.args.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainOnThreeSiblingRulesWatchingSameRepYieldsNoElims) {
    expr goal{expr::var{0}};
    expr head_a{expr::functor{"f", {}}};
    expr head_b{expr::functor{"g", {}}};
    expr head_c{expr::functor{"h", {}}};

    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{1}));
    resolution_lineage* rl_c =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{2}));
    ggcr.link_goal_candidate(gl, rule_id{0});
    ggcr.link_goal_candidate(gl, rule_id{1});
    ggcr.link_goal_candidate(gl, rule_id{2});
    EXPECT_EQ(rules_for(gl), 3u);

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal, &head_b));
    ASSERT_TRUE(mhu->try_add_head(rl_c, &goal, &head_c));
    EXPECT_EQ(common.whnf(&goal), &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsOnOccursCheck) {
    expr goal{expr::var{0}};
    expr head{expr::functor{"f", {&goal}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    EXPECT_FALSE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_EQ(common.whnf(&goal), &goal);
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

    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(common, &goal, "f", 0);

    mhu->clear_mhu_heads();
    EXPECT_TRUE(mhu->try_add_head(rl, &goal, &head));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadOnSameLineageTwiceThrowsInDebug) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head_f));
    EXPECT_THROW(mhu->try_add_head(rl, &goal, &head_g), std::logic_error);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesTwoRepsToCommon) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr goal{expr::functor{"f", {&var0, &var1}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    ggcr.link_goal_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());

    expect_whnf_functor(common, &var0, "abc", 0);
    expect_whnf_functor(common, &var1, "123", 0);
}
