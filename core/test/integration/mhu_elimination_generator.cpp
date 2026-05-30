// mhu_elimination_generator integration — real unify/MHU slice (trail, bind_map, factories, …).
// Cross-goal cases check rebase-driven elimination when shared reps become inconsistent across
// heads, and that partial rep constraints (e.g. g(B) only) do not spuriously eliminate other heads.

#include <array>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
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
using ::testing::UnorderedElementsAre;

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

expr make_functor_expr(const char* name, std::span<expr> args) {
    std::vector<const expr*> ptrs;
    ptrs.reserve(args.size());
    for (expr& arg : args)
        ptrs.push_back(&arg);
    return expr{expr::functor{name, std::move(ptrs)}};
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

    resolution_lineage* link_rl(size_t goal_idx, rule_id rid) {
        goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, goal_idx));
        resolution_lineage* rl =
            const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rid));
        ggcr.link_goal_candidate(gl, rid);
        return rl;
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

// ---------------------------------------------------------------------------
// A. try_add_head + sync_and_link (pre-bound common)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenCommonRepAlreadyBoundIncompatibly) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, &head_f);
    EXPECT_FALSE(mhu->try_add_head(rl, &var0, &head_g));
    expect_whnf_functor(common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadSucceedsWhenCommonRepAlreadyBoundCompatibly) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, &head_f);
    ASSERT_TRUE(mhu->try_add_head(rl, &var0, &head_f));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenCommonEquatesRepsHeadWouldDistinguish) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    expr goal{expr::functor{"f", {&var0, &var1}}};
    expr head{expr::functor{"f", {&abc, &def}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(1, &var0);
    EXPECT_FALSE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_EQ(common.whnf(&var0), &var0);
    EXPECT_EQ(common.whnf(&var1), &var0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenCommonDistinguishesRepsHeadWouldEquate) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, &abc);
    common.bind(1, &def);
    EXPECT_FALSE(mhu->try_add_head(rl, &var0, &var1));
    expect_whnf_functor(common, &var0, "abc", 0);
    expect_whnf_functor(common, &var1, "def", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadReSyncsNestedGoalAgainstCommon) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr fab{expr::functor{"f", {&a, &b}}};
    expr c{expr::functor{"c", {}}};
    expr goal{expr::functor{"g", {&var0, &var1}}};
    expr head{expr::functor{"g", {&fab, &c}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, &fab);
    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(common, &var0, "f", 2);
    expect_whnf_functor(common, &var1, "c", 0);
}

// ---------------------------------------------------------------------------
// B. Cascading eliminations (rebase fan-out)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEliminatesAllIncompatibleHeadsOnSharedRep) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    expr head_h{expr::functor{"h", {}}};
    expr head_i{expr::functor{"i", {}}};
    expr head_j{expr::functor{"j", {}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});
    resolution_lineage* rl_h = link_rl(2, rule_id{0});
    resolution_lineage* rl_i = link_rl(3, rule_id{0});
    resolution_lineage* rl_j = link_rl(4, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal, &head_g));
    ASSERT_TRUE(mhu->try_add_head(rl_h, &goal, &head_h));
    ASSERT_TRUE(mhu->try_add_head(rl_i, &goal, &head_i));
    ASSERT_TRUE(mhu->try_add_head(rl_j, &goal, &head_j));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)),
                UnorderedElementsAre(rl_g, rl_h, rl_i, rl_j));
    expect_whnf_functor(common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesTwoRepsEliminatesTwoWatchers) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr d{expr::functor{"d", {}}};
    expr goal_hub{expr::functor{"f", {&var0, &var1}}};
    expr head_hub{expr::functor{"f", {&a, &b}}};
    expr goal_sat0{expr::functor{"g", {&var0}}};
    expr head_sat0{expr::functor{"g", {&c}}};
    expr goal_sat1{expr::functor{"h", {&var1}}};
    expr head_sat1{expr::functor{"h", {&d}}};

    resolution_lineage* rl_hub = link_rl(0, rule_id{0});
    resolution_lineage* rl_sat0 = link_rl(1, rule_id{0});
    resolution_lineage* rl_sat1 = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_hub, &goal_hub, &head_hub));
    ASSERT_TRUE(mhu->try_add_head(rl_sat0, &goal_sat0, &head_sat0));
    ASSERT_TRUE(mhu->try_add_head(rl_sat1, &goal_sat1, &head_sat1));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub)),
                UnorderedElementsAre(rl_sat0, rl_sat1));
    expect_whnf_functor(common, &var0, "a", 0);
    expect_whnf_functor(common, &var1, "b", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEqualityHeadDoesNotEliminateSingleRepSatellites) {
    constexpr uint32_t kIdxHeadX = 2;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr head_x{expr::var{kIdxHeadX}};
    expr goal_f{expr::functor{"f", {&var0, &var1}}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    expr goal_b{expr::functor{"g", {&var0}}};
    expr head_b{expr::functor{"g", {&abc}}};
    expr goal_c{expr::functor{"g", {&var1}}};
    expr head_c{expr::functor{"g", {&def}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));
    ASSERT_TRUE(mhu->try_add_head(rl_c, &goal_c, &head_c));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    EXPECT_EQ(common.whnf(&var0), common.whnf(&var1));
}

// ---------------------------------------------------------------------------
// C. Partial overlap / diamond topologies
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPartialOverlapThreeHeadChainSurvivesMiddle) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr goal_a{expr::functor{"p", {&var0, &var1, &var2}}};
    expr head_a{expr::functor{"p", {&a, &b, &c}}};
    expr goal_b{expr::functor{"q", {&var1, &var2}}};
    expr head_b{expr::functor{"q", {&b, &c}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_a, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(common, &var0, "a", 0);
    expect_whnf_functor(common, &var1, "b", 0);
    expect_whnf_functor(common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainPartialOverlapThreeHeadChainEliminatesIncompatibleCorner) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr d{expr::functor{"d", {}}};
    expr goal_a{expr::functor{"p", {&var0, &var1, &var2}}};
    expr head_a{expr::functor{"p", {&a, &b, &c}}};
    expr goal_b{expr::functor{"q", {&var1, &var2}}};
    expr head_b{expr::functor{"q", {&b, &c}}};
    expr goal_c{expr::functor{"r", {&var0, &var2}}};
    expr head_c{expr::functor{"r", {&a, &d}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal_a, &head_a));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal_b, &head_b));
    ASSERT_TRUE(mhu->try_add_head(rl_c, &goal_c, &head_c));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_c));
    expect_whnf_functor(common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDisjointRepHeadSurvivesWhenSiblingRepBound) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;
    constexpr uint32_t kIdxC = 3;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr var_c{expr::var{kIdxC}};
    expr head_x{expr::var{kIdxHeadX}};
    expr goal_f{expr::functor{"f", {&var_a, &var_b}}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};
    expr abc{expr::functor{"abc", {}}};
    expr goal_g{expr::functor{"g", {&var_b}}};
    expr head_g{expr::functor{"g", {&abc}}};
    expr e{expr::functor{"e", {}}};
    expr goal_h{expr::functor{"h", {&var_c}}};
    expr head_h{expr::functor{"h", {&e}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});
    resolution_lineage* rl_h = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal_g, &head_g));
    ASSERT_TRUE(mhu->try_add_head(rl_h, &goal_h, &head_h));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    expect_whnf_functor(common, &var_b, "abc", 0);
    EXPECT_EQ(common.whnf(&var_a), &var_a);
    EXPECT_EQ(common.whnf(&var_c), &var_c);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDiamondOverlapEliminatesOnlyIncompatibleCorner) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr def{expr::functor{"def", {}}};
    expr goal_ab{expr::functor{"f", {&var0, &var1}}};
    expr head_ab{expr::functor{"f", {&a, &b}}};
    expr goal_bc{expr::functor{"g", {&var1, &var2}}};
    expr head_bc{expr::functor{"g", {&b, &c}}};
    expr goal_ac{expr::functor{"h", {&var0, &var2}}};
    expr head_ac{expr::functor{"h", {&a, &def}}};

    resolution_lineage* rl_ab = link_rl(0, rule_id{0});
    resolution_lineage* rl_bc = link_rl(1, rule_id{0});
    resolution_lineage* rl_ac = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_ab, &goal_ab, &head_ab));
    ASSERT_TRUE(mhu->try_add_head(rl_bc, &goal_bc, &head_bc));
    ASSERT_TRUE(mhu->try_add_head(rl_ac, &goal_ac, &head_ac));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_bc)), ElementsAre(rl_ac));
    expect_whnf_functor(common, &var1, "b", 0);
    expect_whnf_functor(common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDiamondOverlapSurvivesWhenCornerAgrees) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr goal_ab{expr::functor{"f", {&var0, &var1}}};
    expr head_ab{expr::functor{"f", {&a, &b}}};
    expr goal_bc{expr::functor{"g", {&var1, &var2}}};
    expr head_bc{expr::functor{"g", {&b, &c}}};
    expr goal_ac{expr::functor{"h", {&var0, &var2}}};
    expr head_ac{expr::functor{"h", {&a, &c}}};

    resolution_lineage* rl_ab = link_rl(0, rule_id{0});
    resolution_lineage* rl_bc = link_rl(1, rule_id{0});
    resolution_lineage* rl_ac = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_ab, &goal_ab, &head_ab));
    ASSERT_TRUE(mhu->try_add_head(rl_bc, &goal_bc, &head_bc));
    ASSERT_TRUE(mhu->try_add_head(rl_ac, &goal_ac, &head_ac));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_bc)), IsEmpty());
    expect_whnf_functor(common, &var2, "c", 0);
}

// ---------------------------------------------------------------------------
// D. Sibling vs cross-goal lifecycle (interleaved timelines)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, SecondConstrainAfterFirstCommitRebasesRemainingHeads) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add
    ASSERT_TRUE(mhu->try_add_head(rl_a, &var0, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &var0, &head_g));

    // ② constrain first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_b));
    expect_whnf_functor(common, &var0, "f", 0);

    // ③ add after commit
    resolution_lineage* rl_c = link_rl(2, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_c, &var0, &head_f));

    // ④ constrain again
    EXPECT_THAT(collect_elims(mhu->constrain(rl_c)), IsEmpty());
    expect_whnf_functor(common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotYieldSiblingsAsEliminations) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    expr head_h{expr::functor{"h", {}}};

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

    resolution_lineage* rl_cross = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, &goal, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &goal, &head_g));
    ASSERT_TRUE(mhu->try_add_head(rl_c, &goal, &head_h));
    ASSERT_TRUE(mhu->try_add_head(rl_cross, &goal, &head_g));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_cross));
    expect_whnf_functor(common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedDisjointConstrainThenSharedEliminatesLateAdds) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    expr head_h{expr::functor{"h", {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add shared f/g on var0
    ASSERT_TRUE(mhu->try_add_head(rl_a, &var0, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &var0, &head_g));

    // ② add disjoint h on var1
    resolution_lineage* rl_c = link_rl(2, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_c, &var1, &head_h));

    // ③ constrain disjoint first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_c)), IsEmpty());
    expect_whnf_functor(common, &var1, "h", 0);

    // ④ add late g on var0
    resolution_lineage* rl_d = link_rl(3, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_d, &var0, &head_g));

    // ⑤ constrain shared f
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), UnorderedElementsAre(rl_b, rl_d));
    expect_whnf_functor(common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedGroundPartialThenEqualityConstrain) {
    constexpr uint32_t kIdxHeadX = 2;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr head_x{expr::var{kIdxHeadX}};
    expr goal_f{expr::functor{"f", {&var0, &var1}}};
    expr head_fxx{expr::functor{"f", {&head_x, &head_x}}};
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    expr goal_g{expr::functor{"g", {&var1}}};
    expr head_g{expr::functor{"g", {&abc}}};
    expr goal_h{expr::functor{"h", {&var0}}};
    expr head_h{expr::functor{"h", {&def}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});

    // ① add equality + partial ground heads
    ASSERT_TRUE(mhu->try_add_head(rl_f, &goal_f, &head_fxx));
    ASSERT_TRUE(mhu->try_add_head(rl_g, &goal_g, &head_g));

    // ③ constrain partial ground first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    expect_whnf_functor(common, &var1, "abc", 0);

    // ④ add incompatible ground on var0
    resolution_lineage* rl_h = link_rl(2, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_h, &goal_h, &head_h));

    // ⑤ constrain equality head
    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), ElementsAre(rl_h));
    expect_whnf_functor(common, &var0, "abc", 0);
    expect_whnf_functor(common, &var1, "abc", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedFirstConstrainBlocksIncompatibleLateAdd) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add compatible cross-goal f heads
    ASSERT_TRUE(mhu->try_add_head(rl_a, &var0, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &var0, &head_f));

    // ② constrain first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(common, &var0, "f", 0);

    // ③ incompatible late add fails
    resolution_lineage* rl_c = link_rl(2, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_c, &var0, &head_g));

    // ④ compatible late add succeeds
    resolution_lineage* rl_d = link_rl(3, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_d, &var0, &head_f));

    // ⑤ constrain again
    EXPECT_THAT(collect_elims(mhu->constrain(rl_d)), IsEmpty());
    expect_whnf_functor(common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedThreeWaySharedRepConstrainMiddle) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    expr head_h{expr::functor{"h", {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    // ① add three-way shared rep heads
    ASSERT_TRUE(mhu->try_add_head(rl_a, &var0, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_b, &var0, &head_g));
    ASSERT_TRUE(mhu->try_add_head(rl_c, &var0, &head_h));

    // ② constrain middle g
    EXPECT_THAT(collect_elims(mhu->constrain(rl_b)), UnorderedElementsAre(rl_a, rl_c));
    expect_whnf_functor(common, &var0, "g", 0);

    // ③ compatible late add succeeds (common is g())
    resolution_lineage* rl_d = link_rl(3, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_d, &var0, &head_g));

    // ④ incompatible late add fails
    resolution_lineage* rl_e = link_rl(4, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_e, &var0, &head_f));

    // ⑤ constrain compatible g
    EXPECT_THAT(collect_elims(mhu->constrain(rl_d)), IsEmpty());
    expect_whnf_functor(common, &var0, "g", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedMultiRepHubConstrainAfterSatellites) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr d{expr::functor{"d", {}}};
    expr e{expr::functor{"e", {}}};
    expr goal_hub{expr::functor{"f", {&var0, &var1}}};
    expr head_hub{expr::functor{"f", {&a, &b}}};
    expr goal_sat0{expr::functor{"g", {&var0}}};
    expr head_sat0{expr::functor{"g", {&c}}};
    expr goal_sat1{expr::functor{"h", {&var1}}};
    expr head_sat1{expr::functor{"h", {&d}}};

    resolution_lineage* rl_hub = link_rl(0, rule_id{0});
    resolution_lineage* rl_sat0 = link_rl(1, rule_id{0});
    resolution_lineage* rl_sat1 = link_rl(2, rule_id{0});

    // ① add hub
    ASSERT_TRUE(mhu->try_add_head(rl_hub, &goal_hub, &head_hub));
    // ② add satellites
    ASSERT_TRUE(mhu->try_add_head(rl_sat0, &goal_sat0, &head_sat0));
    ASSERT_TRUE(mhu->try_add_head(rl_sat1, &goal_sat1, &head_sat1));

    // ④ constrain hub
    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub)),
                UnorderedElementsAre(rl_sat0, rl_sat1));

    // ⑤ late incompatible add fails
    resolution_lineage* rl_late = link_rl(3, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_late, &goal_sat0, &e));

    // ⑥ constrain fresh compatible hub
    resolution_lineage* rl_hub2 = link_rl(4, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_hub2, &goal_hub, &head_hub));
    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub2)), IsEmpty());
    expect_whnf_functor(common, &var0, "a", 0);
    expect_whnf_functor(common, &var1, "b", 0);
}

// ---------------------------------------------------------------------------
// E. Large / deep expressions
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesManyRepsFromNaryGoal) {
    std::array<expr, 10> vars{
        expr{expr::var{0}}, expr{expr::var{1}}, expr{expr::var{2}}, expr{expr::var{3}},
        expr{expr::var{4}}, expr{expr::var{5}}, expr{expr::var{6}}, expr{expr::var{7}},
        expr{expr::var{8}}, expr{expr::var{9}}};
    std::array<expr, 10> grounds{
        expr{expr::functor{"g0", {}}}, expr{expr::functor{"g1", {}}},
        expr{expr::functor{"g2", {}}}, expr{expr::functor{"g3", {}}},
        expr{expr::functor{"g4", {}}}, expr{expr::functor{"g5", {}}},
        expr{expr::functor{"g6", {}}}, expr{expr::functor{"g7", {}}},
        expr{expr::functor{"g8", {}}}, expr{expr::functor{"g9", {}}}};

    expr goal = make_functor_expr("f", vars);
    expr head = make_functor_expr("f", grounds);
    resolution_lineage* rl = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &goal, &head));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());

    for (size_t i = 0; i < 10; ++i) {
        std::string name = "g" + std::to_string(i);
        expect_whnf_functor(common, &vars[i], name.c_str(), 0);
    }
}

TEST_F(MhuEliminationGeneratorIntegrationTest, DeepNestedGoalTouchesManyVarsOnAdd) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr hx{expr::var{10}};
    expr fx{expr::var{12}};
    expr h_inner{expr::functor{"h", {&var0, &var1}}};
    expr g_mid{expr::functor{"g", {&h_inner, &var2}}};
    expr goal{expr::functor{"f", {&g_mid, &var3}}};
    expr head_h{expr::functor{"h", {&hx, &hx}}};
    expr def{expr::functor{"def", {}}};
    expr head_g{expr::functor{"g", {&head_h, &def}}};
    expr head_f{expr::functor{"f", {&head_g, &fx}}};
    expr abc{expr::functor{"abc", {}}};
    expr goal_sat{expr::functor{"s", {&var2}}};
    expr head_sat{expr::functor{"s", {&abc}}};

    resolution_lineage* rl_nested = link_rl(0, rule_id{0});
    resolution_lineage* rl_sat = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_nested, &goal, &head_f));
    ASSERT_TRUE(mhu->try_add_head(rl_sat, &goal_sat, &head_sat));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_nested)), ElementsAre(rl_sat));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ManyHeadsDisjointRepsNoCrossElimination) {
    std::array<expr, 6> vars{
        expr{expr::var{0}}, expr{expr::var{1}}, expr{expr::var{2}},
        expr{expr::var{3}}, expr{expr::var{4}}, expr{expr::var{5}}};
    std::array<expr, 6> heads{
        expr{expr::functor{"g0", {}}}, expr{expr::functor{"g1", {}}},
        expr{expr::functor{"g2", {}}}, expr{expr::functor{"g3", {}}},
        expr{expr::functor{"g4", {}}}, expr{expr::functor{"g5", {}}}};
    std::array<resolution_lineage*, 6> rls;

    for (size_t i = 0; i < 6; ++i)
        rls[i] = link_rl(i, rule_id{0});

    for (size_t i = 0; i < 6; ++i)
        ASSERT_TRUE(mhu->try_add_head(rls[i], &vars[i], &heads[i]));

    EXPECT_THAT(collect_elims(mhu->constrain(rls[2])), IsEmpty());
    expect_whnf_functor(common, &vars[2], "g2", 0);
    for (size_t i : {0u, 1u, 3u, 4u, 5u})
        EXPECT_EQ(common.whnf(&vars[i]), &vars[i]);
}

// ---------------------------------------------------------------------------
// F. Negative / edge paths
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainSkipsUnchangedLinkedRep) {
    expr var0{expr::var{0}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, &var0, &var0));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    EXPECT_EQ(common.whnf(&var0), &var0);
}
