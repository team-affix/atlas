// mhu_elimination_generator integration — real unify/MHU slice (trail, bind_map, factories, …).
// Cross-goal cases check rebase-driven elimination when shared reps become inconsistent across
// heads, and that partial rep constraints (e.g. g(B) only) do not spuriously eliminate other heads.

#include <array>
#include <unordered_set>
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
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/coroutine.hpp"
#include "functor_fixture.hpp"

using test_unifier_factory_t = unifier_factory<globalizer, bind_map<globalizer>>;
using local_bind_map_pool_t = pool_allocator<bind_map<globalizer>>;
using test_mhu_t = mhu_elimination_generator<
    bind_map<globalizer>, bind_map<globalizer>, bind_map<globalizer>,
    local_bind_map_pool_t, local_bind_map_pool_t, local_bind_map_pool_t,
    bind_map_factory<globalizer>, unifier<globalizer, bind_map<globalizer>>, test_unifier_factory_t,
    lineage_pool, expr_pool, goal_candidate_rules>;

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

void expect_whnf_functor(test_functors& functors, bind_map<globalizer>& bm, const expr* e, const char* name, size_t argc) {
    const auto& f = std::get<expr::functor>(bm.whnf({e, 0}).skeleton->content);
    EXPECT_EQ(f.id, functors.id(name));
    EXPECT_EQ(f.args.size(), argc);
}

expr make_functor_expr(test_functors& functors, const char* name, std::span<expr> args) {
    std::vector<const expr*> ptrs;
    ptrs.reserve(args.size());
    for (expr& arg : args)
        ptrs.push_back(&arg);
    return expr{expr::functor{functors.id(name), std::move(ptrs)}};
}

} // namespace

struct MhuEliminationGeneratorIntegrationTest : public ::testing::Test {
    
    test_functors functors;
    globalizer g_;
    bind_map<globalizer> common{g_};
    lineage_pool lp;
    bind_map_factory<globalizer> bmf{g_};
    local_bind_map_pool_t bind_map_pool;
    test_unifier_factory_t uf{g_};
    ra_rule_id_set_factory ra_rule_id_set_factory_;
    goal_candidate_rules ggcr{ra_rule_id_set_factory_};
    std::optional<expr_pool> pool;
    std::optional<test_mhu_t> mhu;

    MhuEliminationGeneratorIntegrationTest() {
        pool.emplace();
        mhu.emplace(common, common, lp, *pool, bind_map_pool, bind_map_pool, bind_map_pool, bmf, uf, ggcr);
    }

    size_t rules_for(const goal_lineage* gl) const { return ggcr.get(gl).size(); }

    void ensure_goal_candidates(const goal_lineage* gl) {
        if (registered_goals_.insert(gl).second)
            ggcr.insert(gl);
    }

    void link_candidate(const goal_lineage* gl, rule_id rid) {
        ensure_goal_candidates(gl);
        ggcr.link_goal_candidate(gl, rid);
    }

    resolution_lineage* link_rl(size_t goal_idx, rule_id rid) {
        goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, goal_idx));
        resolution_lineage* rl =
            const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rid));
        link_candidate(gl, rid);
        return rl;
    }

private:
    std::unordered_set<const goal_lineage*> registered_goals_;
};

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadThenConstrainAllowsReuse) {
    expr goal{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_EQ(common.whnf({&goal, 0}).skeleton, &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &goal, "f", 0);

    EXPECT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenUnifyFails) {
    expr goal{expr::functor{functors.id("f"), {}}};
    expr head{expr::functor{functors.id("g"), {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    EXPECT_FALSE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_EQ(common.whnf({&goal, 0}).skeleton, &goal);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesSeededBindingToCommon) {
    expr goal{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_EQ(common.whnf({&goal, 0}).skeleton, &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEliminatesHeadWithCollidingFunctorOnSameRep) {
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
    EXPECT_EQ(common.whnf({&goal_a, 0}).skeleton, &goal_a);
    EXPECT_EQ(common.whnf({&goal_b, 0}).skeleton, &goal_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_b));
    expect_whnf_functor(functors, common, &goal_a, "f", 0);
    expect_whnf_functor(functors, common, &goal_b, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotEliminateCompatibleHeadOnSameRep) {
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
    EXPECT_EQ(common.whnf({&goal_a, 0}).skeleton, &goal_a);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(functors, common, &goal_a, "f", 0);
    expect_whnf_functor(functors, common, &goal_b, "f", 0);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_b)), IsEmpty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotEliminateHeadWatchingDisjointRep) {
    expr goal_a{expr::var{0}};
    expr head_a{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl_a = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_a, rule_id{0}));
    link_candidate(gl_a, rule_id{0});

    expr var2{expr::var{2}};
    expr goal_b{var2};
    expr head_b{expr::functor{functors.id("g"), {}}};
    goal_lineage* gl_b = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 2));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_b, rule_id{0}));
    link_candidate(gl_b, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));
    EXPECT_EQ(common.whnf({&goal_a, 0}).skeleton, &goal_a);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(functors, common, &goal_a, "f", 0);
    EXPECT_EQ(common.whnf({&var2, 0}).skeleton, &var2);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainDoesNotElimSiblingButEliminatesCrossGoalIncompatibleHead) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_rule0 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    resolution_lineage* rl_rule1 =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{1}));
    link_candidate(gl, rule_id{0});
    link_candidate(gl, rule_id{1});
    EXPECT_EQ(rules_for(gl), 2u);

    goal_lineage* gl_other = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_other =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_other, rule_id{1}));
    link_candidate(gl_other, rule_id{1});
    EXPECT_EQ(rules_for(gl_other), 1u);

    ASSERT_TRUE(mhu->try_add_head(rl_rule0, {&goal, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_rule1, {&goal, 0}, {&head_g, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_other, {&goal, 0}, {&head_g, 0}));
    EXPECT_EQ(common.whnf({&goal, 0}).skeleton, &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_rule0)), ElementsAre(rl_other));
    expect_whnf_functor(functors, common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainFxXHeadEliminatesGroundGHeadOnSharedReps) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{functors.id("f"), {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};

    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var_a, &var_b}}};
    expr head_g{expr::functor{functors.id("g"), {&abc, &_123}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_id{0}));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_id{0}));
    link_candidate(gl_f, rule_id{0});
    link_candidate(gl_g, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));
    ASSERT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);
    ASSERT_EQ(common.whnf({&var_b, 0}).skeleton, &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), ElementsAre(rl_g));
    EXPECT_EQ(common.whnf({&var_a, 0}).skeleton, common.whnf({&var_b, 0}).skeleton);
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
    expr goal_f{expr::functor{functors.id("f"), {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};

    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var_a, &var_b}}};
    expr head_g{expr::functor{functors.id("g"), {&abc, &_123}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_f_id));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_g_id));
    link_candidate(gl_f, rule_f_id);
    link_candidate(gl_g, rule_g_id);

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));
    ASSERT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);
    ASSERT_EQ(common.whnf({&var_b, 0}).skeleton, &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), ElementsAre(rl_f));
    const auto& whnf_a = std::get<expr::functor>(common.whnf({&var_a, 0}).skeleton->content);
    const auto& whnf_b = std::get<expr::functor>(common.whnf({&var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("123"));
    EXPECT_TRUE(whnf_b.args.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainGroundGHeadDoesNotEliminateFxXHead) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{functors.id("f"), {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};

    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var_b}}};
    expr head_g{expr::functor{functors.id("g"), {&abc}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_id{0}));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_id{0}));
    link_candidate(gl_f, rule_id{0});
    link_candidate(gl_g, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));
    ASSERT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);
    ASSERT_EQ(common.whnf({&var_b, 0}).skeleton, &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    const auto& whnf_b = std::get<expr::functor>(common.whnf({&var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_b.id, functors.id("abc"));
    EXPECT_TRUE(whnf_b.args.empty());
    EXPECT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainFxXHeadAfterGroundGHeadBindsAWithoutEliminatingG) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr goal_f{expr::functor{functors.id("f"), {&var_a, &var_b}}};
    expr head_x{expr::var{kIdxHeadX}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};

    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var_b}}};
    expr head_g{expr::functor{functors.id("g"), {&abc}}};

    goal_lineage* gl_f = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    goal_lineage* gl_g = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 1));
    resolution_lineage* rl_f =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_f, rule_id{0}));
    resolution_lineage* rl_g =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl_g, rule_id{0}));
    link_candidate(gl_f, rule_id{0});
    link_candidate(gl_g, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));
    ASSERT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);
    ASSERT_EQ(common.whnf({&var_b, 0}).skeleton, &var_b);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    const auto& whnf_b_after_g = std::get<expr::functor>(common.whnf({&var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_b_after_g.id, functors.id("abc"));
    EXPECT_TRUE(whnf_b_after_g.args.empty());
    EXPECT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), IsEmpty());
    const auto& whnf_a = std::get<expr::functor>(common.whnf({&var_a, 0}).skeleton->content);
    const auto& whnf_b = std::get<expr::functor>(common.whnf({&var_b, 0}).skeleton->content);
    EXPECT_EQ(whnf_a.id, functors.id("abc"));
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.id, functors.id("abc"));
    EXPECT_TRUE(whnf_b.args.empty());
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainOnThreeSiblingRulesWatchingSameRepYieldsNoElims) {
    expr goal{expr::var{0}};
    expr head_a{expr::functor{functors.id("f"), {}}};
    expr head_b{expr::functor{functors.id("g"), {}}};
    expr head_c{expr::functor{functors.id("h"), {}}};

    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{1}));
    resolution_lineage* rl_c =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{2}));
    link_candidate(gl, rule_id{0});
    link_candidate(gl, rule_id{1});
    link_candidate(gl, rule_id{2});
    EXPECT_EQ(rules_for(gl), 3u);

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal, 0}, {&head_b, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&goal, 0}, {&head_c, 0}));
    EXPECT_EQ(common.whnf({&goal, 0}).skeleton, &goal);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(functors, common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsOnOccursCheck) {
    expr goal{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {&goal}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    EXPECT_FALSE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_EQ(common.whnf({&goal, 0}).skeleton, &goal);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ClearMhuHeadsAllowsFreshTryAdd) {
    expr goal{expr::var{0}};
    expr head{expr::functor{functors.id("f"), {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    mhu->clear_mhu_heads();
    EXPECT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &goal, "f", 0);

    mhu->clear_mhu_heads();
    EXPECT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadOnSameLineageTwiceThrowsInDebug) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head_f, 0}));
    EXPECT_THROW(mhu->try_add_head(rl, {&goal, 0}, {&head_g, 0}), std::logic_error);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainWithoutRegisteredHeadThrowsOutOfRange) {
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    EXPECT_THROW(
        (void)collect_elims(mhu->constrain(rl)),
        std::out_of_range);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesTwoRepsToCommon) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr _123{expr::functor{functors.id("123"), {}}};
    expr goal{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head{expr::functor{functors.id("f"), {&abc, &_123}}};
    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    link_candidate(gl, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());

    expect_whnf_functor(functors, common, &var0, "abc", 0);
    expect_whnf_functor(functors, common, &var1, "123", 0);
}

// ---------------------------------------------------------------------------
// A. try_add_head + sync_and_link (pre-bound common)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenCommonRepAlreadyBoundIncompatibly) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, {&head_f, 0});
    EXPECT_FALSE(mhu->try_add_head(rl, {&var0, 0}, {&head_g, 0}));
    expect_whnf_functor(functors, common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadSucceedsWhenCommonRepAlreadyBoundCompatibly) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, {&head_f, 0});
    ASSERT_TRUE(mhu->try_add_head(rl, {&var0, 0}, {&head_f, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenCommonEquatesRepsHeadWouldDistinguish) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr goal{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head{expr::functor{functors.id("f"), {&abc, &def}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(1, {&var0, 0});
    EXPECT_FALSE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_EQ(common.whnf({&var0, 0}).skeleton, &var0);
    EXPECT_EQ(common.whnf({&var1, 0}).skeleton, &var0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenCommonDistinguishesRepsHeadWouldEquate) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, {&abc, 0});
    common.bind(1, {&def, 0});
    EXPECT_FALSE(mhu->try_add_head(rl, {&var0, 0}, {&var1, 0}));
    expect_whnf_functor(functors, common, &var0, "abc", 0);
    expect_whnf_functor(functors, common, &var1, "def", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadReSyncsNestedGoalAgainstCommon) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr fab{expr::functor{functors.id("f"), {&a, &b}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr goal{expr::functor{functors.id("g"), {&var0, &var1}}};
    expr head{expr::functor{functors.id("g"), {&fab, &c}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    common.bind(0, {&fab, 0});
    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "f", 2);
    expect_whnf_functor(functors, common, &var1, "c", 0);
}

// ---------------------------------------------------------------------------
// B. Cascading eliminations (rebase fan-out)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEliminatesAllIncompatibleHeadsOnSharedRep) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    expr head_h{expr::functor{functors.id("h"), {}}};
    expr head_i{expr::functor{functors.id("i"), {}}};
    expr head_j{expr::functor{functors.id("j"), {}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});
    resolution_lineage* rl_h = link_rl(2, rule_id{0});
    resolution_lineage* rl_i = link_rl(3, rule_id{0});
    resolution_lineage* rl_j = link_rl(4, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal, 0}, {&head_g, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_h, {&goal, 0}, {&head_h, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_i, {&goal, 0}, {&head_i, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_j, {&goal, 0}, {&head_j, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)),
                UnorderedElementsAre(rl_g, rl_h, rl_i, rl_j));
    expect_whnf_functor(functors, common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesTwoRepsEliminatesTwoWatchers) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr d{expr::functor{functors.id("d"), {}}};
    expr goal_hub{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_hub{expr::functor{functors.id("f"), {&a, &b}}};
    expr goal_sat0{expr::functor{functors.id("g"), {&var0}}};
    expr head_sat0{expr::functor{functors.id("g"), {&c}}};
    expr goal_sat1{expr::functor{functors.id("h"), {&var1}}};
    expr head_sat1{expr::functor{functors.id("h"), {&d}}};

    resolution_lineage* rl_hub = link_rl(0, rule_id{0});
    resolution_lineage* rl_sat0 = link_rl(1, rule_id{0});
    resolution_lineage* rl_sat1 = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_hub, {&goal_hub, 0}, {&head_hub, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_sat0, {&goal_sat0, 0}, {&head_sat0, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_sat1, {&goal_sat1, 0}, {&head_sat1, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub)),
                UnorderedElementsAre(rl_sat0, rl_sat1));
    expect_whnf_functor(functors, common, &var0, "a", 0);
    expect_whnf_functor(functors, common, &var1, "b", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEqualityHeadDoesNotEliminateSingleRepSatellites) {
    constexpr uint32_t kIdxHeadX = 2;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr head_x{expr::var{kIdxHeadX}};
    expr goal_f{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr goal_b{expr::functor{functors.id("g"), {&var0}}};
    expr head_b{expr::functor{functors.id("g"), {&abc}}};
    expr goal_c{expr::functor{functors.id("g"), {&var1}}};
    expr head_c{expr::functor{functors.id("g"), {&def}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&goal_c, 0}, {&head_c, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    EXPECT_EQ(common.whnf({&var0, 0}).skeleton, common.whnf({&var1, 0}).skeleton);
}

// ---------------------------------------------------------------------------
// C. Partial overlap / diamond topologies
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPartialOverlapThreeHeadChainSurvivesMiddle) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr goal_a{expr::functor{functors.id("p"), {&var0, &var1, &var2}}};
    expr head_a{expr::functor{functors.id("p"), {&a, &b, &c}}};
    expr goal_b{expr::functor{functors.id("q"), {&var1, &var2}}};
    expr head_b{expr::functor{functors.id("q"), {&b, &c}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "a", 0);
    expect_whnf_functor(functors, common, &var1, "b", 0);
    expect_whnf_functor(functors, common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainPartialOverlapThreeHeadChainEliminatesIncompatibleCorner) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr d{expr::functor{functors.id("d"), {}}};
    expr goal_a{expr::functor{functors.id("p"), {&var0, &var1, &var2}}};
    expr head_a{expr::functor{functors.id("p"), {&a, &b, &c}}};
    expr goal_b{expr::functor{functors.id("q"), {&var1, &var2}}};
    expr head_b{expr::functor{functors.id("q"), {&b, &c}}};
    expr goal_c{expr::functor{functors.id("r"), {&var0, &var2}}};
    expr head_c{expr::functor{functors.id("r"), {&a, &d}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&goal_c, 0}, {&head_c, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_c));
    expect_whnf_functor(functors, common, &var2, "c", 0);
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
    expr goal_f{expr::functor{functors.id("f"), {&var_a, &var_b}}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var_b}}};
    expr head_g{expr::functor{functors.id("g"), {&abc}}};
    expr e{expr::functor{functors.id("e"), {}}};
    expr goal_h{expr::functor{functors.id("h"), {&var_c}}};
    expr head_h{expr::functor{functors.id("h"), {&e}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});
    resolution_lineage* rl_h = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_h, {&goal_h, 0}, {&head_h, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    expect_whnf_functor(functors, common, &var_b, "abc", 0);
    EXPECT_EQ(common.whnf({&var_a, 0}).skeleton, &var_a);
    EXPECT_EQ(common.whnf({&var_c, 0}).skeleton, &var_c);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDiamondOverlapEliminatesOnlyIncompatibleCorner) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr goal_ab{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_ab{expr::functor{functors.id("f"), {&a, &b}}};
    expr goal_bc{expr::functor{functors.id("g"), {&var1, &var2}}};
    expr head_bc{expr::functor{functors.id("g"), {&b, &c}}};
    expr goal_ac{expr::functor{functors.id("h"), {&var0, &var2}}};
    expr head_ac{expr::functor{functors.id("h"), {&a, &def}}};

    resolution_lineage* rl_ab = link_rl(0, rule_id{0});
    resolution_lineage* rl_bc = link_rl(1, rule_id{0});
    resolution_lineage* rl_ac = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_ab, {&goal_ab, 0}, {&head_ab, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_bc, {&goal_bc, 0}, {&head_bc, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_ac, {&goal_ac, 0}, {&head_ac, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_bc)), ElementsAre(rl_ac));
    expect_whnf_functor(functors, common, &var1, "b", 0);
    expect_whnf_functor(functors, common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDiamondOverlapSurvivesWhenCornerAgrees) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr goal_ab{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_ab{expr::functor{functors.id("f"), {&a, &b}}};
    expr goal_bc{expr::functor{functors.id("g"), {&var1, &var2}}};
    expr head_bc{expr::functor{functors.id("g"), {&b, &c}}};
    expr goal_ac{expr::functor{functors.id("h"), {&var0, &var2}}};
    expr head_ac{expr::functor{functors.id("h"), {&a, &c}}};

    resolution_lineage* rl_ab = link_rl(0, rule_id{0});
    resolution_lineage* rl_bc = link_rl(1, rule_id{0});
    resolution_lineage* rl_ac = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_ab, {&goal_ab, 0}, {&head_ab, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_bc, {&goal_bc, 0}, {&head_bc, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_ac, {&goal_ac, 0}, {&head_ac, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_bc)), IsEmpty());
    expect_whnf_functor(functors, common, &var2, "c", 0);
}

// ---------------------------------------------------------------------------
// D. Sibling vs cross-goal lifecycle (interleaved timelines)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, SecondConstrainAfterFirstCommitRebasesRemainingHeads) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add
    ASSERT_TRUE(mhu->try_add_head(rl_a, {&var0, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&var0, 0}, {&head_g, 0}));

    // ② constrain first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_b));
    expect_whnf_functor(functors, common, &var0, "f", 0);

    // ③ add after commit
    resolution_lineage* rl_c = link_rl(2, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&var0, 0}, {&head_f, 0}));

    // ④ constrain again
    EXPECT_THAT(collect_elims(mhu->constrain(rl_c)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotYieldSiblingsAsEliminations) {
    expr goal{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    expr head_h{expr::functor{functors.id("h"), {}}};

    goal_lineage* gl = const_cast<goal_lineage*>(lp.make_goal_lineage(nullptr, 0));
    resolution_lineage* rl_a =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{0}));
    resolution_lineage* rl_b =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{1}));
    resolution_lineage* rl_c =
        const_cast<resolution_lineage*>(lp.make_resolution_lineage(gl, rule_id{2}));
    link_candidate(gl, rule_id{0});
    link_candidate(gl, rule_id{1});
    link_candidate(gl, rule_id{2});

    resolution_lineage* rl_cross = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal, 0}, {&head_g, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&goal, 0}, {&head_h, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_cross, {&goal, 0}, {&head_g, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), ElementsAre(rl_cross));
    expect_whnf_functor(functors, common, &goal, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedDisjointConstrainThenSharedEliminatesLateAdds) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    expr head_h{expr::functor{functors.id("h"), {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add shared f/g on var0
    ASSERT_TRUE(mhu->try_add_head(rl_a, {&var0, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&var0, 0}, {&head_g, 0}));

    // ② add disjoint h on var1
    resolution_lineage* rl_c = link_rl(2, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&var1, 0}, {&head_h, 0}));

    // ③ constrain disjoint first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_c)), IsEmpty());
    expect_whnf_functor(functors, common, &var1, "h", 0);

    // ④ add late g on var0
    resolution_lineage* rl_d = link_rl(3, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_d, {&var0, 0}, {&head_g, 0}));

    // ⑤ constrain shared f
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), UnorderedElementsAre(rl_b, rl_d));
    expect_whnf_functor(functors, common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedGroundPartialThenEqualityConstrain) {
    constexpr uint32_t kIdxHeadX = 2;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr head_x{expr::var{kIdxHeadX}};
    expr goal_f{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var1}}};
    expr head_g{expr::functor{functors.id("g"), {&abc}}};
    expr goal_h{expr::functor{functors.id("h"), {&var0}}};
    expr head_h{expr::functor{functors.id("h"), {&def}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});

    // ① add equality + partial ground heads
    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));

    // ③ constrain partial ground first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    expect_whnf_functor(functors, common, &var1, "abc", 0);

    // ④ add incompatible ground on var0
    resolution_lineage* rl_h = link_rl(2, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_h, {&goal_h, 0}, {&head_h, 0}));

    // ⑤ constrain equality head
    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), ElementsAre(rl_h));
    expect_whnf_functor(functors, common, &var0, "abc", 0);
    expect_whnf_functor(functors, common, &var1, "abc", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedFirstConstrainBlocksIncompatibleLateAdd) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add compatible cross-goal f heads
    ASSERT_TRUE(mhu->try_add_head(rl_a, {&var0, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&var0, 0}, {&head_f, 0}));

    // ② constrain first
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "f", 0);

    // ③ incompatible late add fails
    resolution_lineage* rl_c = link_rl(2, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_c, {&var0, 0}, {&head_g, 0}));

    // ④ compatible late add succeeds
    resolution_lineage* rl_d = link_rl(3, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_d, {&var0, 0}, {&head_f, 0}));

    // ⑤ constrain again
    EXPECT_THAT(collect_elims(mhu->constrain(rl_d)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "f", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedThreeWaySharedRepConstrainMiddle) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    expr head_h{expr::functor{functors.id("h"), {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    // ① add three-way shared rep heads
    ASSERT_TRUE(mhu->try_add_head(rl_a, {&var0, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&var0, 0}, {&head_g, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&var0, 0}, {&head_h, 0}));

    // ② constrain middle g
    EXPECT_THAT(collect_elims(mhu->constrain(rl_b)), UnorderedElementsAre(rl_a, rl_c));
    expect_whnf_functor(functors, common, &var0, "g", 0);

    // ③ compatible late add succeeds (common is g())
    resolution_lineage* rl_d = link_rl(3, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_d, {&var0, 0}, {&head_g, 0}));

    // ④ incompatible late add fails
    resolution_lineage* rl_e = link_rl(4, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_e, {&var0, 0}, {&head_f, 0}));

    // ⑤ constrain compatible g
    EXPECT_THAT(collect_elims(mhu->constrain(rl_d)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "g", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, InterleavedMultiRepHubConstrainAfterSatellites) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr d{expr::functor{functors.id("d"), {}}};
    expr e{expr::functor{functors.id("e"), {}}};
    expr goal_hub{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_hub{expr::functor{functors.id("f"), {&a, &b}}};
    expr goal_sat0{expr::functor{functors.id("g"), {&var0}}};
    expr head_sat0{expr::functor{functors.id("g"), {&c}}};
    expr goal_sat1{expr::functor{functors.id("h"), {&var1}}};
    expr head_sat1{expr::functor{functors.id("h"), {&d}}};

    resolution_lineage* rl_hub = link_rl(0, rule_id{0});
    resolution_lineage* rl_sat0 = link_rl(1, rule_id{0});
    resolution_lineage* rl_sat1 = link_rl(2, rule_id{0});

    // ① add hub
    ASSERT_TRUE(mhu->try_add_head(rl_hub, {&goal_hub, 0}, {&head_hub, 0}));
    // ② add satellites
    ASSERT_TRUE(mhu->try_add_head(rl_sat0, {&goal_sat0, 0}, {&head_sat0, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_sat1, {&goal_sat1, 0}, {&head_sat1, 0}));

    // ④ constrain hub
    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub)),
                UnorderedElementsAre(rl_sat0, rl_sat1));

    // ⑤ late incompatible add fails
    resolution_lineage* rl_late = link_rl(3, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_late, {&goal_sat0, 0}, {&e, 0}));

    // ⑥ constrain fresh compatible hub
    resolution_lineage* rl_hub2 = link_rl(4, rule_id{0});
    ASSERT_TRUE(mhu->try_add_head(rl_hub2, {&goal_hub, 0}, {&head_hub, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub2)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "a", 0);
    expect_whnf_functor(functors, common, &var1, "b", 0);
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
        expr{expr::functor{functors.id("g0"), {}}}, expr{expr::functor{functors.id("g1"), {}}},
        expr{expr::functor{functors.id("g2"), {}}}, expr{expr::functor{functors.id("g3"), {}}},
        expr{expr::functor{functors.id("g4"), {}}}, expr{expr::functor{functors.id("g5"), {}}},
        expr{expr::functor{functors.id("g6"), {}}}, expr{expr::functor{functors.id("g7"), {}}},
        expr{expr::functor{functors.id("g8"), {}}}, expr{expr::functor{functors.id("g9"), {}}}};

    expr goal = make_functor_expr(functors, "f", vars);
    expr head = make_functor_expr(functors, "f", grounds);
    resolution_lineage* rl = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());

    for (size_t i = 0; i < 10; ++i) {
        std::string name = "g" + std::to_string(i);
        expect_whnf_functor(functors, common, &vars[i], name.c_str(), 0);
    }
}

TEST_F(MhuEliminationGeneratorIntegrationTest, DeepNestedGoalTouchesManyVarsOnAdd) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr hx{expr::var{10}};
    expr fx{expr::var{12}};
    expr h_inner{expr::functor{functors.id("h"), {&var0, &var1}}};
    expr g_mid{expr::functor{functors.id("g"), {&h_inner, &var2}}};
    expr goal{expr::functor{functors.id("f"), {&g_mid, &var3}}};
    expr head_h{expr::functor{functors.id("h"), {&hx, &hx}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr head_g{expr::functor{functors.id("g"), {&head_h, &def}}};
    expr head_f{expr::functor{functors.id("f"), {&head_g, &fx}}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_sat{expr::functor{functors.id("s"), {&var2}}};
    expr head_sat{expr::functor{functors.id("s"), {&abc}}};

    resolution_lineage* rl_nested = link_rl(0, rule_id{0});
    resolution_lineage* rl_sat = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_nested, {&goal, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_sat, {&goal_sat, 0}, {&head_sat, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_nested)), ElementsAre(rl_sat));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ManyHeadsDisjointRepsNoCrossElimination) {
    std::array<expr, 6> vars{
        expr{expr::var{0}}, expr{expr::var{1}}, expr{expr::var{2}},
        expr{expr::var{3}}, expr{expr::var{4}}, expr{expr::var{5}}};
    std::array<expr, 6> heads{
        expr{expr::functor{functors.id("g0"), {}}}, expr{expr::functor{functors.id("g1"), {}}},
        expr{expr::functor{functors.id("g2"), {}}}, expr{expr::functor{functors.id("g3"), {}}},
        expr{expr::functor{functors.id("g4"), {}}}, expr{expr::functor{functors.id("g5"), {}}}};
    std::array<resolution_lineage*, 6> rls;

    for (size_t i = 0; i < 6; ++i)
        rls[i] = link_rl(i, rule_id{0});

    for (size_t i = 0; i < 6; ++i)
        ASSERT_TRUE(mhu->try_add_head(rls[i], {&vars[i], 0}, {&heads[i], 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rls[2])), IsEmpty());
    expect_whnf_functor(functors, common, &vars[2], "g2", 0);
    for (size_t i : {0u, 1u, 3u, 4u, 5u})
        EXPECT_EQ(common.whnf({&vars[i], 0}).skeleton, &vars[i]);
}

// ---------------------------------------------------------------------------
// F. Negative / edge paths
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainSkipsUnchangedLinkedRep) {
    expr var0{expr::var{0}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&var0, 0}, {&var0, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    EXPECT_EQ(common.whnf({&var0, 0}).skeleton, &var0);
}

// ---------------------------------------------------------------------------
// G–N. Round 2 — rep unlink, multi-hub, lifecycle, functor rebase, gaps
// ---------------------------------------------------------------------------

// Priority 1 — K, J, N

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ConstrainPartialOverlapThreeHeadChainSurvivesWhenCornerAgrees) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr goal_a{expr::functor{functors.id("p"), {&var0, &var1, &var2}}};
    expr head_a{expr::functor{functors.id("p"), {&a, &b, &c}}};
    expr goal_b{expr::functor{functors.id("q"), {&var1, &var2}}};
    expr head_b{expr::functor{functors.id("q"), {&b, &c}}};
    expr goal_c{expr::functor{functors.id("r"), {&var0, &var2}}};
    expr head_c{expr::functor{functors.id("r"), {&a, &c}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});
    resolution_lineage* rl_c = link_rl(2, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_c, {&goal_c, 0}, {&head_c, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "a", 0);
    expect_whnf_functor(functors, common, &var1, "b", 0);
    expect_whnf_functor(functors, common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDualRepGroundHeadSurvivesWhenGroundsAgree) {
    constexpr uint32_t kIdxA = 0;
    constexpr uint32_t kIdxB = 1;
    constexpr uint32_t kIdxHeadX = 2;

    expr var_a{expr::var{kIdxA}};
    expr var_b{expr::var{kIdxB}};
    expr head_x{expr::var{kIdxHeadX}};
    expr goal_f{expr::functor{functors.id("f"), {&var_a, &var_b}}};
    expr head_fxx{expr::functor{functors.id("f"), {&head_x, &head_x}}};

    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_g{expr::functor{functors.id("g"), {&var_a, &var_b}}};
    expr head_g{expr::functor{functors.id("g"), {&abc, &abc}}};

    resolution_lineage* rl_f = link_rl(0, rule_id{0});
    resolution_lineage* rl_g = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_f, {&goal_f, 0}, {&head_fxx, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_g, {&goal_g, 0}, {&head_g, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_f)), IsEmpty());
    EXPECT_EQ(common.whnf({&var_a, 0}).skeleton, common.whnf({&var_b, 0}).skeleton);

    EXPECT_THAT(collect_elims(mhu->constrain(rl_g)), IsEmpty());
    expect_whnf_functor(functors, common, &var_a, "abc", 0);
    expect_whnf_functor(functors, common, &var_b, "abc", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainSkipsRepAlreadyBoundInLocalToSameTarget) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head{expr::functor{functors.id("f"), {&abc, &var1}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&goal, 0}, {&head, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "abc", 0);
    EXPECT_EQ(common.whnf({&var1, 0}).skeleton, &var1);
}

// Priority 2 — G, L, M

TEST_F(MhuEliminationGeneratorIntegrationTest, SecondConstrainOnDisjointRepDoesNotBreakPriorSurvivor) {
    constexpr uint32_t kIdxHeadX = 2;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr head_x{expr::var{kIdxHeadX}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_a{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_a{expr::functor{functors.id("f"), {&head_x, &head_x}}};
    expr goal_b{expr::functor{functors.id("g"), {&var2}}};
    expr head_b{expr::functor{functors.id("g"), {&abc}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_a, {&goal_a, 0}, {&head_a, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&goal_b, 0}, {&head_b, 0}));

    // ① constrain disjoint rep
    EXPECT_THAT(collect_elims(mhu->constrain(rl_b)), IsEmpty());
    expect_whnf_functor(functors, common, &var2, "abc", 0);
    EXPECT_EQ(common.whnf({&var0, 0}).skeleton, &var0);
    EXPECT_EQ(common.whnf({&var1, 0}).skeleton, &var1);

    // ② constrain equality head on {var0, var1} — no elims; disjoint binding unchanged
    EXPECT_THAT(collect_elims(mhu->constrain(rl_a)), IsEmpty());
    EXPECT_EQ(common.whnf({&var0, 0}).skeleton, common.whnf({&var1, 0}).skeleton);
    expect_whnf_functor(functors, common, &var2, "abc", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainHubEliminatesOtherHubOnSharedRep) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr goal_hub1{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_hub1{expr::functor{functors.id("f"), {&a, &b}}};
    expr goal_hub2{expr::functor{functors.id("g"), {&var0, &var1}}};
    expr head_hub2{expr::functor{functors.id("g"), {&a, &c}}};

    resolution_lineage* rl_hub1 = link_rl(0, rule_id{0});
    resolution_lineage* rl_hub2 = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_hub1, {&goal_hub1, 0}, {&head_hub1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_hub2, {&goal_hub2, 0}, {&head_hub2, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub1)), ElementsAre(rl_hub2));
    expect_whnf_functor(functors, common, &var0, "a", 0);
    expect_whnf_functor(functors, common, &var1, "b", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    TwoCompatibleHubsOnOverlappingRepsBothSurviveUntilConstrain) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr a{expr::functor{functors.id("a"), {}}};
    expr b{expr::functor{functors.id("b"), {}}};
    expr c{expr::functor{functors.id("c"), {}}};
    expr goal_hub1{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr head_hub1{expr::functor{functors.id("f"), {&a, &b}}};
    expr goal_hub2{expr::functor{functors.id("f"), {&var1, &var2}}};
    expr head_hub2{expr::functor{functors.id("f"), {&b, &c}}};

    resolution_lineage* rl_hub1 = link_rl(0, rule_id{0});
    resolution_lineage* rl_hub2 = link_rl(1, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_hub1, {&goal_hub1, 0}, {&head_hub1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_hub2, {&goal_hub2, 0}, {&head_hub2, 0}));

    // ① constrain hub1 — hub2 survives
    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub1)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "a", 0);
    expect_whnf_functor(functors, common, &var1, "b", 0);

    // ② constrain hub2 — still valid after hub1 publish/rebase
    EXPECT_THAT(collect_elims(mhu->constrain(rl_hub2)), IsEmpty());
    expect_whnf_functor(functors, common, &var2, "c", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ClearMhuHeadsWithoutConstrainAllowsBothHeadsReAdd) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};

    resolution_lineage* rl_a = link_rl(0, rule_id{0});
    resolution_lineage* rl_b = link_rl(1, rule_id{0});

    // ① add both heads
    ASSERT_TRUE(mhu->try_add_head(rl_a, {&var0, 0}, {&head_f, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_b, {&var0, 0}, {&head_g, 0}));

    // ② clear without constrain
    mhu->clear_mhu_heads();

    // ③ both re-add succeed
    EXPECT_TRUE(mhu->try_add_head(rl_a, {&var0, 0}, {&head_f, 0}));
    EXPECT_TRUE(mhu->try_add_head(rl_b, {&var0, 0}, {&head_g, 0}));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ClearMhuHeadsAfterPartialConstrainAllowsReAddOnDisjointRep) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr goal_h{expr::functor{functors.id("h"), {&var1}}};
    expr head_h{expr::functor{functors.id("h"), {&abc}}};

    resolution_lineage* rl_h = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl_h, {&goal_h, 0}, {&head_h, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl_h)), IsEmpty());
    expect_whnf_functor(functors, common, &var1, "abc", 0);

    mhu->clear_mhu_heads();

    resolution_lineage* rl_f = link_rl(1, rule_id{0});
    EXPECT_TRUE(mhu->try_add_head(rl_f, {&var0, 0}, {&head_f, 0}));
    EXPECT_EQ(common.whnf({&var0, 0}).skeleton, &var0);
    expect_whnf_functor(functors, common, &var1, "abc", 0);
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ClearMhuHeadsAfterPartialConstrainDoesNotResetCommon) {
    expr var0{expr::var{0}};
    expr head_f{expr::functor{functors.id("f"), {}}};
    expr head_g{expr::functor{functors.id("g"), {}}};
    resolution_lineage* rl = link_rl(0, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl, {&var0, 0}, {&head_f, 0}));
    EXPECT_THAT(collect_elims(mhu->constrain(rl)), IsEmpty());
    expect_whnf_functor(functors, common, &var0, "f", 0);

    mhu->clear_mhu_heads();

    resolution_lineage* rl_g = link_rl(1, rule_id{0});
    EXPECT_FALSE(mhu->try_add_head(rl_g, {&var0, 0}, {&head_g, 0}));
    expect_whnf_functor(functors, common, &var0, "f", 0);
}

// Priority 3 — I: functor common WHNF during rebase (satellite must link rep 1)

TEST_F(MhuEliminationGeneratorIntegrationTest, RebaseFollowsCommonWhnfThroughFunctorArg) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr f_var1{expr::functor{functors.id("f"), {&var1}}};
    expr f_abc{expr::functor{functors.id("f"), {&abc}}};
    expr goal_sat = var0;
    expr head_sat = f_abc;
    expr goal_hub{expr::functor{functors.id("h"), {&var1}}};
    expr head_hub{expr::functor{functors.id("h"), {&def}}};

    // Pre-bind: rep 0 bound to functor containing rep 1
    common.bind(0, {&f_var1, 0});

    resolution_lineage* rl_sat = link_rl(0, rule_id{0});
    resolution_lineage* rl_hub = link_rl(1, rule_id{0});

    // Satellite try_add links reps {0,1} by unifying var0 with f(var1) in common
    ASSERT_TRUE(mhu->try_add_head(rl_sat, {&goal_sat, 0}, {&head_sat, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_hub, {&goal_hub, 0}, {&head_hub, 0}));

    // constrain(satellite) publishes var0→f(abc), var1→abc; rebase_all(1) eliminates hub
    EXPECT_THAT(collect_elims(mhu->constrain(rl_sat)), ElementsAre(rl_hub));
    expect_whnf_functor(functors, common, &var0, "f", 1);
    expect_whnf_functor(functors, common, &var1, "abc", 0);
    const auto& f0 = std::get<expr::functor>(common.whnf({&var0, 0}).skeleton->content);
    expect_whnf_functor(functors, common, f0.args[0], "abc", 0);
}

// ---------------------------------------------------------------------------
// O–Q. Round 3 — serial chain stress, late-ground watchers, rebase fan-out
// ---------------------------------------------------------------------------

namespace {

void expect_all_whnf_abc(test_functors& functors, bind_map<globalizer>& common, std::initializer_list<expr*> vars) {
    for (expr* v : vars)
        expect_whnf_functor(functors, common, v, "abc", 0);
}

void expect_equated_to_canonical(bind_map<globalizer>& common, std::initializer_list<expr*> vars,
    const expr* canonical) {
    for (expr* v : vars)
        EXPECT_EQ(common.whnf({v, 0}).skeleton, canonical);
}

void expect_free_reps(bind_map<globalizer>& common, std::initializer_list<expr*> vars) {
    for (expr* v : vars)
        EXPECT_EQ(common.whnf({v, 0}).skeleton, v);
}

} // namespace

TEST_F(MhuEliminationGeneratorIntegrationTest,
    SequentialFourHeadChainConstrainsPropagateGroundToAllReps) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr abc{expr::functor{functors.id("abc"), {}}};

    resolution_lineage* rl0 = link_rl(0, rule_id{0});
    resolution_lineage* rl1 = link_rl(1, rule_id{0});
    resolution_lineage* rl2 = link_rl(2, rule_id{0});
    resolution_lineage* rl3 = link_rl(3, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl0, {&var0, 0}, {&var1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl1, {&var1, 0}, {&var2, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl2, {&var2, 0}, {&var3, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl3, {&var3, 0}, {&abc, 0}));

    // ① constrain rl0
    EXPECT_THAT(collect_elims(mhu->constrain(rl0)), IsEmpty());
    expect_equated_to_canonical(common, {&var0, &var1}, &var0);
    expect_free_reps(common, {&var2, &var3});

    // ② constrain rl1
    EXPECT_THAT(collect_elims(mhu->constrain(rl1)), IsEmpty());
    expect_equated_to_canonical(common, {&var0, &var1, &var2}, &var0);
    expect_free_reps(common, {&var3});

    // ③ constrain rl2
    EXPECT_THAT(collect_elims(mhu->constrain(rl2)), IsEmpty());
    expect_equated_to_canonical(common, {&var0, &var1, &var2, &var3}, &var0);

    // ④ constrain rl3 — ground propagates to all reps
    EXPECT_THAT(collect_elims(mhu->constrain(rl3)), IsEmpty());
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});
}

TEST_F(MhuEliminationGeneratorIntegrationTest,
    ReverseFourHeadChainConstrainsPropagateGroundToAllReps) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr abc{expr::functor{functors.id("abc"), {}}};

    resolution_lineage* rl0 = link_rl(0, rule_id{0});
    resolution_lineage* rl1 = link_rl(1, rule_id{0});
    resolution_lineage* rl2 = link_rl(2, rule_id{0});
    resolution_lineage* rl3 = link_rl(3, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl0, {&var0, 0}, {&var1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl1, {&var1, 0}, {&var2, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl2, {&var2, 0}, {&var3, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl3, {&var3, 0}, {&abc, 0}));

    // ① constrain rl3 — ground rep 3 only
    EXPECT_THAT(collect_elims(mhu->constrain(rl3)), IsEmpty());
    expect_whnf_functor(functors, common, &var3, "abc", 0);
    expect_free_reps(common, {&var0, &var1, &var2});

    // ② constrain rl2 — ground enters equate class
    EXPECT_THAT(collect_elims(mhu->constrain(rl2)), IsEmpty());
    expect_whnf_functor(functors, common, &var2, "abc", 0);

    // ③ constrain rl1
    EXPECT_THAT(collect_elims(mhu->constrain(rl1)), IsEmpty());

    // ④ constrain rl0 — all reps grounded
    EXPECT_THAT(collect_elims(mhu->constrain(rl0)), IsEmpty());
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});
}

TEST_F(MhuEliminationGeneratorIntegrationTest, SixHeadChainConstrainsPropagateGroundToAllReps) {
    std::array<expr, 6> vars{
        expr{expr::var{0}}, expr{expr::var{1}}, expr{expr::var{2}},
        expr{expr::var{3}}, expr{expr::var{4}}, expr{expr::var{5}}};
    expr abc{expr::functor{functors.id("abc"), {}}};

    std::array<resolution_lineage*, 6> rls;
    for (size_t i = 0; i < 6; ++i)
        rls[i] = link_rl(i, rule_id{0});

    for (size_t i = 0; i < 5; ++i)
        ASSERT_TRUE(mhu->try_add_head(rls[i], {&vars[i], 0}, {&vars[i + 1], 0}));
    ASSERT_TRUE(mhu->try_add_head(rls[5], {&vars[5], 0}, {&abc, 0}));

    for (size_t i = 0; i < 6; ++i)
        EXPECT_THAT(collect_elims(mhu->constrain(rls[i])), IsEmpty());

    expect_all_whnf_abc(functors, common, {&vars[0], &vars[1], &vars[2], &vars[3], &vars[4], &vars[5]});
}

TEST_F(MhuEliminationGeneratorIntegrationTest, SerialChainLateGroundWithCompatibleWatcherSurvives) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_w{expr::functor{functors.id("g"), {&var3}}};
    expr head_w{expr::functor{functors.id("g"), {&abc}}};

    resolution_lineage* rl0 = link_rl(0, rule_id{0});
    resolution_lineage* rl1 = link_rl(1, rule_id{0});
    resolution_lineage* rl2 = link_rl(2, rule_id{0});
    resolution_lineage* rl3 = link_rl(3, rule_id{0});
    resolution_lineage* rl_w = link_rl(4, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl0, {&var0, 0}, {&var1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl1, {&var1, 0}, {&var2, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl2, {&var2, 0}, {&var3, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl3, {&var3, 0}, {&abc, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_w, {&goal_w, 0}, {&head_w, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl0)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl1)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl2)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl3)), IsEmpty());
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});

    EXPECT_THAT(collect_elims(mhu->constrain(rl_w)), IsEmpty());
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});
}

TEST_F(MhuEliminationGeneratorIntegrationTest, SerialChainLateGroundEliminatesIncompatibleWatcher) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr def{expr::functor{functors.id("def"), {}}};
    expr goal_w{expr::functor{functors.id("g"), {&var3}}};
    expr head_w{expr::functor{functors.id("g"), {&def}}};

    resolution_lineage* rl0 = link_rl(0, rule_id{0});
    resolution_lineage* rl1 = link_rl(1, rule_id{0});
    resolution_lineage* rl2 = link_rl(2, rule_id{0});
    resolution_lineage* rl3 = link_rl(3, rule_id{0});
    resolution_lineage* rl_w = link_rl(4, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl0, {&var0, 0}, {&var1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl1, {&var1, 0}, {&var2, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl2, {&var2, 0}, {&var3, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl3, {&var3, 0}, {&abc, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl_w, {&goal_w, 0}, {&head_w, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl0)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl1)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl2)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl3)), ElementsAre(rl_w));
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ManyHeadsOnVar2SurviveSerialChainConstrains) {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr goal_w{expr::functor{functors.id("g"), {&var2}}};
    expr head_w{expr::functor{functors.id("g"), {&abc}}};

    resolution_lineage* rl0 = link_rl(0, rule_id{0});
    resolution_lineage* rl1 = link_rl(1, rule_id{0});
    resolution_lineage* rl2 = link_rl(2, rule_id{0});
    resolution_lineage* rl3 = link_rl(3, rule_id{0});

    constexpr size_t kWatcherCount = 8;
    std::array<resolution_lineage*, kWatcherCount> watchers;
    for (size_t i = 0; i < kWatcherCount; ++i)
        watchers[i] = link_rl(4 + i, rule_id{0});

    ASSERT_TRUE(mhu->try_add_head(rl0, {&var0, 0}, {&var1, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl1, {&var1, 0}, {&var2, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl2, {&var2, 0}, {&var3, 0}));
    ASSERT_TRUE(mhu->try_add_head(rl3, {&var3, 0}, {&abc, 0}));
    for (resolution_lineage* rl_w : watchers)
        ASSERT_TRUE(mhu->try_add_head(rl_w, {&goal_w, 0}, {&head_w, 0}));

    EXPECT_THAT(collect_elims(mhu->constrain(rl0)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl1)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl2)), IsEmpty());
    EXPECT_THAT(collect_elims(mhu->constrain(rl3)), IsEmpty());
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});

    EXPECT_THAT(collect_elims(mhu->constrain(watchers[0])), IsEmpty());
    expect_all_whnf_abc(functors, common, {&var0, &var1, &var2, &var3});
}
