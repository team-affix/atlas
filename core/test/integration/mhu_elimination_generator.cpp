#include <gtest/gtest.h>
#include <algorithm>
#include "../../../core/hpp/infrastructure/mhu_elimination_generator.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/overlay_bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/unifier_factory.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/infrastructure/goal_candidate_rules.hpp"
#include "../../../core/hpp/utility/trail.hpp"

namespace {

std::vector<const resolution_lineage*> collect_elims(
    state_machine<const resolution_lineage*>& sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value())
            out.push_back(v.value());
    }
    return out;
}

bool contains(const std::vector<const resolution_lineage*>& xs, const resolution_lineage* rl) {
    return std::find(xs.begin(), xs.end(), rl) != xs.end();
}

bool is_functor_named(const expr* e, const std::string& name) {
    const expr::functor* f = std::get_if<expr::functor>(&e->content);
    return f && f->name == name;
}

} // namespace

struct MhuEliminationGeneratorIntegrationTest : public ::testing::Test {
protected:
    trail t;
    expr_pool pool{t};
    bind_map common;
    lineage_pool lp;
    bind_map_factory bmf;
    overlay_bind_map_factory obmf;
    unifier_factory uf;
    goal_candidate_rules ggcr;
    mhu_elimination_generator mhu{common, lp, pool, bmf, obmf, uf, ggcr};

    expr f_empty{expr::functor{"f", {}}};
    expr g_empty{expr::functor{"g", {}}};
    expr var0{expr::var{0}};

    struct candidate_slot {
        goal_lineage* gl;
        resolution_lineage* rl;
        expr goal;
        expr head;
        rule r;
    };

    candidate_slot make_slot(const expr& goal, const expr& head) {
        candidate_slot s{
            nullptr,
            nullptr,
            goal,
            head,
            rule{&s.head, {&s.goal}}};
        s.gl = const_cast<goal_lineage*>(lp.goal(nullptr, &s.goal));
        s.rl = const_cast<resolution_lineage*>(lp.resolution(s.gl, &s.r));
        ggcr.link_goal_candidate(s.gl, &s.r);
        return s;
    }

    bool seed_and_add_head(
        const resolution_lineage* rl,
        const expr* goal,
        const expr* head) {
        return mhu.try_add_head(rl, goal, head);
    }
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadThenConstrainAllowsReuse) {
    auto slot = make_slot(var0, f_empty);
    ASSERT_TRUE(seed_and_add_head(slot.rl, &slot.goal, &slot.head));

    auto sm = mhu.constrain(slot.rl);
    EXPECT_TRUE(collect_elims(sm).empty());

    EXPECT_TRUE(seed_and_add_head(slot.rl, &slot.goal, &slot.head));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, TryAddHeadFailsWhenUnifyFails) {
    auto slot = make_slot(f_empty, g_empty);
    EXPECT_FALSE(mhu.try_add_head(slot.rl, &slot.goal, &slot.head));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainOnSingleHeadYieldsNoElims) {
    auto slot = make_slot(var0, f_empty);
    ASSERT_TRUE(mhu.try_add_head(slot.rl, &slot.goal, &slot.head));

    auto sm = mhu.constrain(slot.rl);
    EXPECT_TRUE(collect_elims(sm).empty());
}

// ---------------------------------------------------------------------------
// Rebasing: constrain publishes local bindings into common
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainPublishesSeededBindingToCommon) {
    auto slot = make_slot(var0, f_empty);
    ASSERT_TRUE(seed_and_add_head(slot.rl, &slot.goal, &slot.head));

    auto sm = mhu.constrain(slot.rl);
    collect_elims(sm);

    EXPECT_TRUE(is_functor_named(common.whnf(&var0), "f"));
}

// ---------------------------------------------------------------------------
// Rebasing: colliding unifications → elimination
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainEliminatesHeadWithCollidingFunctorOnSameRep) {
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var0, g_empty);
    ASSERT_TRUE(seed_and_add_head(a.rl, &a.goal, &a.head));
    ASSERT_TRUE(seed_and_add_head(b.rl, &b.goal, &b.head));

    auto sm = mhu.constrain(a.rl);
    auto elims = collect_elims(sm);

    EXPECT_TRUE(contains(elims, b.rl));
    EXPECT_FALSE(contains(elims, a.rl));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotEliminateCompatibleHeadOnSameRep) {
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var0, f_empty);
    ASSERT_TRUE(seed_and_add_head(a.rl, &a.goal, &a.head));
    ASSERT_TRUE(seed_and_add_head(b.rl, &b.goal, &b.head));

    auto sm = mhu.constrain(a.rl);
    auto elims = collect_elims(sm);

    EXPECT_FALSE(contains(elims, b.rl));
    EXPECT_FALSE(contains(elims, a.rl));
}

TEST_F(MhuEliminationGeneratorIntegrationTest, ConstrainDoesNotEliminateHeadWatchingDisjointRep) {
    expr var2{expr::var{2}};
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var2, g_empty);
    ASSERT_TRUE(seed_and_add_head(a.rl, &a.goal, &a.head));
    ASSERT_TRUE(seed_and_add_head(b.rl, &b.goal, &b.head));

    auto sm = mhu.constrain(a.rl);
    auto elims = collect_elims(sm);

    EXPECT_FALSE(contains(elims, b.rl));
}
