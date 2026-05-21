#include <gtest/gtest.h>
#include <algorithm>
#include "../../../core/hpp/infrastructure/mhu_elimination_generator.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/overlay_bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/unifier_factory.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/value_objects/unify_head.hpp"

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

class MhuEliminationGeneratorTest : public ::testing::Test {
protected:
    trail t;
    expr_pool pool{t};
    bind_map common;
    lineage_pool lp;
    mhu_elimination_generator mhu{common, pool};

    bind_map_factory bmf;
    overlay_bind_map_factory obmf;
    unifier_factory uf;

    expr f_empty{expr::functor{"f", {}}};
    expr g_empty{expr::functor{"g", {}}};
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};

    unify_head make_head() {
        auto local = bmf.make();
        auto overlay = obmf.make(*local, common);
        auto u = uf.make(*overlay);
        return unify_head{
            std::move(local),
            std::move(overlay),
            std::move(u)};
    }

    std::unordered_set<uint32_t> seed_and_add_head(
        const resolution_lineage* rl,
        const expr* goal,
        const expr* head) {
        auto local = bmf.make();
        auto overlay = obmf.make(*local, common);
        auto u = uf.make(*overlay);
        std::unordered_set<uint32_t> rep_changed;
        EXPECT_TRUE(u->unify(goal, head, rep_changed));
        unify_head head_bundle{
            std::move(local),
            std::move(overlay),
            std::move(u)};
        mhu.add_head(rl, std::move(head_bundle), rep_changed);
        return rep_changed;
    }

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
        return s;
    }
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorTest, AddHeadThenConstrainRemovesHead) {
    auto slot = make_slot(var0, f_empty);
    seed_and_add_head(slot.rl, &slot.goal, &slot.head);

    auto sm = mhu.constrain(slot.rl);
    EXPECT_TRUE(collect_elims(sm).empty());

    EXPECT_NO_THROW(mhu.try_remove_head(slot.rl));
}

TEST_F(MhuEliminationGeneratorTest, TryRemoveHeadOnUnknownLineageIsNoOp) {
    auto slot = make_slot(var0, f_empty);
    EXPECT_NO_THROW(mhu.try_remove_head(slot.rl));
}

TEST_F(MhuEliminationGeneratorTest, TryRemoveHeadAfterAddAllowsReuse) {
    auto slot = make_slot(var0, f_empty);
    seed_and_add_head(slot.rl, &slot.goal, &slot.head);
    mhu.try_remove_head(slot.rl);
    EXPECT_NO_THROW(seed_and_add_head(slot.rl, &slot.goal, &slot.head));
}

TEST_F(MhuEliminationGeneratorTest, ConstrainWithNoRepLinksCommitsNothingAndRemovesHead) {
    auto slot = make_slot(var0, f_empty);
    mhu.add_head(slot.rl, make_head(), {});

    auto sm = mhu.constrain(slot.rl);
    EXPECT_TRUE(collect_elims(sm).empty());
    EXPECT_NO_THROW(mhu.try_remove_head(slot.rl));
}

// ---------------------------------------------------------------------------
// Rebasing: constrain publishes local bindings into common
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorTest, ConstrainPublishesSeededBindingToCommon) {
    auto slot = make_slot(var0, f_empty);
    seed_and_add_head(slot.rl, &slot.goal, &slot.head);

    auto sm = mhu.constrain(slot.rl);
    collect_elims(sm);

    EXPECT_TRUE(is_functor_named(common.whnf(&var0), "f"));
}

// ---------------------------------------------------------------------------
// Rebasing: colliding unifications → elimination
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorTest, ConstrainEliminatesHeadWithCollidingFunctorOnSameRep) {
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var0, g_empty);
    seed_and_add_head(a.rl, &a.goal, &a.head);
    seed_and_add_head(b.rl, &b.goal, &b.head);

    auto sm = mhu.constrain(a.rl);
    auto elims = collect_elims(sm);

    EXPECT_TRUE(contains(elims, b.rl));
    EXPECT_FALSE(contains(elims, a.rl));
}

TEST_F(MhuEliminationGeneratorTest, ConstrainDoesNotEliminateCompatibleHeadOnSameRep) {
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var0, f_empty);
    seed_and_add_head(a.rl, &a.goal, &a.head);
    seed_and_add_head(b.rl, &b.goal, &b.head);

    auto sm = mhu.constrain(a.rl);
    auto elims = collect_elims(sm);

    EXPECT_FALSE(contains(elims, b.rl));
    EXPECT_FALSE(contains(elims, a.rl));
}

TEST_F(MhuEliminationGeneratorTest, ConstrainDoesNotEliminateHeadWatchingDisjointRep) {
    expr var2{expr::var{2}};
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var2, g_empty);
    seed_and_add_head(a.rl, &a.goal, &a.head);
    seed_and_add_head(b.rl, &b.goal, &b.head);

    auto sm = mhu.constrain(a.rl);
    auto elims = collect_elims(sm);

    EXPECT_FALSE(contains(elims, b.rl));
}

// ---------------------------------------------------------------------------
// Rebasing: try_remove_head after revalidate failure (head already erased)
// ---------------------------------------------------------------------------

TEST_F(MhuEliminationGeneratorTest, TryRemoveHeadAfterRevalidateEliminationIsNoOp) {
    auto a = make_slot(var0, f_empty);
    auto b = make_slot(var0, g_empty);
    seed_and_add_head(a.rl, &a.goal, &a.head);
    seed_and_add_head(b.rl, &b.goal, &b.head);

    auto sm = mhu.constrain(a.rl);
    collect_elims(sm);

    EXPECT_NO_THROW(mhu.try_remove_head(b.rl));
}

