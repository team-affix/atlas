// Integration: a real unifier writing through the REAL journaling dbuct_bind_map,
// across camp/pop boundaries.
//
// unifier_bind_map.cpp covers the unifier against the plain non-journaling
// bind_map, and dbuct_bind_map's unit test journals hand-written bind() calls.
// Neither combination proves what production actually needs: that the bindings a
// real unification produces inside a camped frame are gone after the pop.
//
// The subtle half is whnf's path compression. whnf rewrites intermediate chain
// entries in place and records a SECOND journal entry for each, so a frame can
// contain undo actions for keys the unifier never bound directly.

#include <cstdint>

#include <gtest/gtest.h>

#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "functor_fixture.hpp"

namespace {

using bind_map_t = dbuct_bind_map<globalizer>;
using unifier_t = unifier<globalizer, bind_map_t>;

framed_expr fe(const expr* e) { return {e, 0}; }

}  // namespace

struct DbuctUnifierBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;

    globalizer g;
    bind_map_t bm{g};
    unifier_t u{g, &bm};

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func_f{expr::functor{functors.id("f"), {}}};
    expr func_g{expr::functor{functors.id("g"), {}}};

    bool run_unify(framed_expr lhs, framed_expr rhs) {
        auto task = u.unify(lhs, rhs);
        while (!task.done())
            task.resume();
        return task.result();
    }

    const expr* whnf(const expr* e) { return bm.whnf(fe(e)).skeleton; }
};

TEST_F(DbuctUnifierBindMapIntegrationTest, BindingsMadeInCampedFrameAreGoneAfterPop) {
    ASSERT_TRUE(run_unify(fe(&var1), fe(&func_g)));
    ASSERT_EQ(whnf(&var1), &func_g);

    bm.push_frame();
    ASSERT_TRUE(run_unify(fe(&var0), fe(&func_f)));
    ASSERT_EQ(whnf(&var0), &func_f);
    bm.pop_frame();

    EXPECT_EQ(whnf(&var0), &var0);
    // The pre-camp binding is untouched.
    EXPECT_EQ(whnf(&var1), &func_g);
}

TEST_F(DbuctUnifierBindMapIntegrationTest, ChainedBindingWhnfRestoredAfterPop) {
    // Build the chain var2 -> var1 -> var0 without resolving it, so no path
    // compression is recorded in the base frame.
    bm.bind(2, fe(&var1));
    bm.bind(1, fe(&var0));

    bm.push_frame();
    ASSERT_TRUE(run_unify(fe(&var0), fe(&func_f)));
    // Resolving from the tail compresses var2 and var1 to point straight at f.
    ASSERT_EQ(whnf(&var2), &func_f);
    ASSERT_EQ(whnf(&var1), &func_f);
    bm.pop_frame();

    // Both the binding and every compressed link it produced are rewound.
    EXPECT_EQ(whnf(&var0), &var0);
    EXPECT_EQ(whnf(&var1), &var0);
    EXPECT_EQ(whnf(&var2), &var0);
}

TEST_F(DbuctUnifierBindMapIntegrationTest, OccursCheckFailureLeavesNoResidualBindings) {
    // The first argument unifies and binds var1 before the second argument hits
    // the occurs check, so the failed unification leaves a residual binding.
    // Popping the frame is what production relies on to discard it.
    expr h_var0{expr::functor{functors.id("h"), {&var0}}};
    expr lhs{expr::functor{functors.id("f"), {&var1, &var0}}};
    expr rhs{expr::functor{functors.id("f"), {&func_g, &h_var0}}};

    bm.push_frame();
    EXPECT_FALSE(run_unify(fe(&lhs), fe(&rhs)));
    bm.pop_frame();

    EXPECT_EQ(whnf(&var0), &var0);
    EXPECT_EQ(whnf(&var1), &var1);
}

TEST_F(DbuctUnifierBindMapIntegrationTest, RebindAfterPopProducesIndependentResult) {
    // Backtracking to a decision point and taking the other branch must not be
    // contaminated by the abandoned branch's bindings.
    bm.push_frame();
    ASSERT_TRUE(run_unify(fe(&var0), fe(&func_f)));
    bm.pop_frame();

    bm.push_frame();
    EXPECT_TRUE(run_unify(fe(&var0), fe(&func_g)));
    EXPECT_EQ(whnf(&var0), &func_g);
    bm.pop_frame();

    EXPECT_EQ(whnf(&var0), &var0);
}
