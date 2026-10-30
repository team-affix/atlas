#include <cstdint>
#include <random>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_expr_structure.hpp"
#include "pud_term_stack.hpp"
#include "reference_pud_unify.hpp"

namespace {

struct cross_pool_term_builder {
    expr_pool& pool_;
    test_functors& functors_;

    const expr* var(uint32_t local_index) {
        return pool_.make_var(local_index);
    }

    const expr* atom(const char* name) {
        return pool_.make_functor(functors_.id(name), {});
    }

};

struct PudUnifierCrossPoolIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;
    expr_pool foreign_a;
    expr_pool foreign_b;
    cross_pool_term_builder pool_a{foreign_a, functors};
    cross_pool_term_builder pool_b{foreign_b, functors};
    cross_pool_term_builder canonical{stack.pool, functors};
};

}

TEST_F(PudUnifierCrossPoolIntegrationTest, DistinctPoolsNeverShareExprPointersForSameShape) {
    const expr* stack_atom = canonical.atom("z");
    const expr* foreign_atom = pool_a.atom("z");
    EXPECT_NE(stack_atom, foreign_atom);
    EXPECT_TRUE(same_structure(stack_atom, foreign_atom));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolGroundAtomsUnifyWithoutBindMapWrites) {
    const expr* left = pool_a.atom("a");
    const expr* right = pool_b.atom("a");
    EXPECT_NE(left, right);
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
    pud_ubs_state after_snap = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after_snap};
    EXPECT_TRUE(before == after_snap);
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolVarsMergeByGlobalIndexNotPointer) {
    const expr* left = pool_a.var(0);
    const expr* right = pool_b.var(1);
    EXPECT_NE(left, right);
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
    const expr* rep = stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u});
    EXPECT_TRUE(same_structure(rep, stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u})));
    EXPECT_TRUE(same_structure(rep, canonical.var(0)));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolDeepMirrorBindsAllLocals) {
    const expr* left = foreign_a.make_functor(functors.id("g"), {
        foreign_a.make_functor(functors.id("f"), {pool_a.var(0)}),
        pool_a.var(1),
    });
    const expr* right = foreign_b.make_functor(functors.id("g"), {
        foreign_b.make_functor(functors.id("f"), {pool_b.var(2)}),
        pool_b.var(3),
    });
    EXPECT_NE(left, right);
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), canonical.var(0)));
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u}), canonical.var(1)));
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(2u), 0u}), canonical.var(0)));
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(3u), 0u}), canonical.var(1)));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, VarWithForeignFunctorStoresNormalizedNotForeignPointer) {
    const expr* foreign_head = foreign_a.make_functor(functors.id("h"), {pool_a.var(0), pool_a.atom("a")});
    const expr* foreign_var = pool_b.var(4);
    EXPECT_NE(foreign_var, pool_a.var(4));
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{foreign_var, 0, 0}, moduled_expr{foreign_head, 0, 0}));
    const expr* bound = stack.normalizer.normalize(framed_expr{stack.pool.make_var(4u), 0u});
    EXPECT_NE(bound, foreign_head);
    const expr* expected = stack.pool.make_functor(functors.id("h"), {canonical.var(0), canonical.atom("a")});
    EXPECT_TRUE(same_structure(bound, expected));
    EXPECT_TRUE(same_structure(stack.normalizer.normalize(framed_expr{stack.pool.make_var(4u), 0u}), expected));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, ForeignFunctorLeftVarRightSameNormalization) {
    const expr* foreign_var = pool_a.var(2);
    const expr* stack_head = stack.pool.make_functor(functors.id("p"), {canonical.var(0), canonical.atom("b")});
    const expr* foreign_head = foreign_b.make_functor(functors.id("p"), {pool_b.var(0), pool_b.atom("b")});
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{foreign_head, 0, 0}, moduled_expr{foreign_var, 0, 0}));
    const expr* bound = stack.normalizer.normalize(framed_expr{stack.pool.make_var(2u), 0u});
    EXPECT_NE(bound, foreign_head);
    EXPECT_TRUE(same_structure(bound, stack_head));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, SecondUnifyReusesBindingAcrossForeignSkeletons) {
    const expr* foreign_var = pool_a.var(1);
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{foreign_var, 0, 0}, moduled_expr{pool_b.atom("k"), 0, 0}));
    const expr* probe = foreign_b.make_functor(functors.id("wrap"), {pool_b.var(1)});
    const expr* target = foreign_a.make_functor(functors.id("wrap"), {pool_a.atom("k")});
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{probe, 0, 0}, moduled_expr{target, 0, 0}));
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u}), canonical.atom("k")));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolSwapSidesSameStructuralResult) {
    pud_term_stack forward;
    pud_term_stack reverse;
    cross_pool_term_builder fwd_a{foreign_a, functors};
    expr_pool foreign_c;
    cross_pool_term_builder fwd_b{foreign_c, functors};
    cross_pool_term_builder rev_a{reverse.pool, functors};
    cross_pool_term_builder rev_b{foreign_b, functors};

    const expr* fwd_left = foreign_a.make_functor(functors.id("q"), {fwd_a.var(0), fwd_a.atom("t")});
    const expr* fwd_right = foreign_c.make_functor(functors.id("q"), {fwd_b.atom("t"), fwd_b.var(1)});
    const expr* rev_left = foreign_b.make_functor(functors.id("q"), {rev_b.atom("t"), rev_b.var(1)});
    const expr* rev_right = reverse.pool.make_functor(functors.id("q"), {rev_a.var(0), rev_a.atom("t")});

    EXPECT_TRUE(forward.unifier.unify(moduled_expr{fwd_left, 0, 0}, moduled_expr{fwd_right, 0, 0}));
    EXPECT_TRUE(reverse.unifier.unify(moduled_expr{rev_left, 0, 0}, moduled_expr{rev_right, 0, 0}));

    for (uint32_t global_var = 0; global_var < 4u; ++global_var) {
        const expr* forward_value = forward.normalizer.normalize(
            framed_expr{forward.pool.make_var(global_var), 0u});
        const expr* reverse_value = reverse.normalizer.normalize(
            framed_expr{reverse.pool.make_var(global_var), 0u});
        EXPECT_TRUE(same_structure(forward_value, reverse_value));
    }
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolFrameOffsetsUseGlobalsNotPoolIdentity) {
    const expr* left = foreign_a.make_functor(functors.id("f"), {pool_a.var(0)});
    const expr* right = foreign_b.make_functor(functors.id("f"), {pool_b.var(0)});
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 10, 0}));
    const uint32_t global_zero = stack.globalize.globalize(0, 0);
    const uint32_t global_ten = stack.globalize.globalize(10, 0);
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(global_zero), 0u}),
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(global_ten), 0u})));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, MergedClassForeignMembersSkipRedundantEquate) {
    stack.bind_map.equate(stack.bind_map.find(2u), stack.bind_map.find(5u));
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    const expr* left = pool_a.var(2);
    const expr* right = pool_b.var(5);
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
    pud_ubs_state after_snap = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after_snap};
    EXPECT_TRUE(before == after_snap);
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolMismatchBindsFirstArgThenFails) {
    const expr* left = foreign_a.make_functor(functors.id("pair"), {pool_a.var(0), pool_a.atom("a")});
    const expr* right = foreign_b.make_functor(functors.id("pair"), {pool_b.atom("b"), pool_b.var(0)});
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    EXPECT_FALSE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
    EXPECT_TRUE(same_structure(
        stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), canonical.atom("b")));
    EXPECT_FALSE(same_structure(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), canonical.atom("a")));
    pud_ubs_state after_partial = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after_partial};
    EXPECT_NE(before, after_partial);
}

TEST_F(PudUnifierCrossPoolIntegrationTest, OccursFailureWithForeignCyclicRhs) {
    const expr* foreign_loop = foreign_a.make_functor(functors.id("loop"), {pool_a.var(0)});
    const expr* foreign_var = pool_b.var(0);
    EXPECT_FALSE(stack.unifier.unify(moduled_expr{foreign_var, 0, 0}, moduled_expr{foreign_loop, 0, 0}));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, WideCrossPoolArityFiftySucceedsStructurally) {
    const uint32_t pid = functors.id("wide");
    std::vector<const expr*> left_args;
    std::vector<const expr*> right_args;
    for (uint32_t arg = 0; arg < 50u; ++arg) {
        left_args.push_back(pool_a.var(arg % 4u));
        right_args.push_back(pool_b.var(arg % 4u));
    }
    const expr* left = foreign_a.make_functor(pid, left_args);
    const expr* right = foreign_b.make_functor(pid, right_args);
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
    for (uint32_t global_var = 0; global_var < 4u; ++global_var)
        EXPECT_TRUE(same_structure(
            stack.normalizer.normalize(framed_expr{stack.pool.make_var(global_var), 0u}),
            canonical.var(global_var)));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, RandomMirroredCrossPoolMatchesSinglePoolReference) {
    std::mt19937 rng(202u);
    const uint32_t nest_id = functors.id("nest");
    for (int trial = 0; trial < 40; ++trial) {
        const uint32_t depth = static_cast<uint32_t>(rng() % 5u);
        const char* leaf_name = rng() % 2u == 0u ? "a" : "b";
        std::vector<uint32_t> var_slots;
        for (uint32_t layer = 0; layer < depth; ++layer)
            var_slots.push_back(static_cast<uint32_t>(rng() % 3u));

        pud_term_stack reference_stack;
        expr_pool ref_left_pool;
        expr_pool ref_right_pool;
        cross_pool_term_builder ref_left_builder{ref_left_pool, functors};
        cross_pool_term_builder ref_right_builder{ref_right_pool, functors};
        const expr* ref_left = ref_left_builder.atom(leaf_name);
        const expr* ref_right = ref_right_builder.atom(leaf_name);
        for (uint32_t slot : var_slots) {
            ref_left = ref_left_pool.make_functor(nest_id, {ref_left, ref_left_builder.var(slot)});
            ref_right = ref_right_pool.make_functor(nest_id, {ref_right, ref_right_builder.var(slot)});
        }
        EXPECT_NE(ref_left, ref_right);
        const bool reference_ok = reference_pud_unify_terms(
            reference_stack, framed_expr{ref_left, 0}, framed_expr{ref_right, 0});

        pud_term_stack cross_stack;
        expr_pool left_pool;
        expr_pool right_pool;
        cross_pool_term_builder left_builder{left_pool, functors};
        cross_pool_term_builder right_builder{right_pool, functors};
        const expr* left_term = left_builder.atom(leaf_name);
        const expr* right_term = right_builder.atom(leaf_name);
        for (uint32_t slot : var_slots) {
            left_term = left_pool.make_functor(nest_id, {left_term, left_builder.var(slot)});
            right_term = right_pool.make_functor(nest_id, {right_term, right_builder.var(slot)});
        }
        EXPECT_NE(left_term, right_term);
        const bool cross_ok = cross_stack.unifier.unify(moduled_expr{left_term, 0, 0}, moduled_expr{right_term, 0, 0}).has_value();
        EXPECT_EQ(reference_ok, cross_ok);

        if (!reference_ok)
            continue;

        for (uint32_t global_var = 0; global_var < 8u; ++global_var) {
            EXPECT_TRUE(same_structure(
                reference_stack.normalizer.normalize(
                    framed_expr{reference_stack.pool.make_var(global_var), 0u}),
                cross_stack.normalizer.normalize(
                    framed_expr{cross_stack.pool.make_var(global_var), 0u})));
        }
    }
}

TEST_F(PudUnifierCrossPoolIntegrationTest, IndependentStacksWithCrossPoolInputsStayConsistent) {
    pud_term_stack stack_one;
    pud_term_stack stack_two;
    expr_pool one_foreign;
    expr_pool two_foreign;
    cross_pool_term_builder one_pool{one_foreign, functors};
    cross_pool_term_builder two_pool{two_foreign, functors};

    const expr* one_left = one_foreign.make_functor(functors.id("m"), {one_pool.var(0), one_pool.atom("a")});
    const expr* one_right = stack_one.pool.make_functor(
        functors.id("m"),
        {stack_one.pool.make_var(1u), stack_one.pool.make_functor(functors.id("a"), {})});
    const expr* two_left = two_foreign.make_functor(functors.id("m"), {two_pool.var(0), two_pool.atom("a")});
    const expr* two_right = stack_two.pool.make_functor(
        functors.id("m"), {stack_two.pool.make_var(1u), stack_two.pool.make_functor(functors.id("a"), {})});

    EXPECT_TRUE(stack_one.unifier.unify(moduled_expr{one_left, 0, 0}, moduled_expr{one_right, 0, 0}));
    EXPECT_TRUE(stack_two.unifier.unify(moduled_expr{two_left, 0, 0}, moduled_expr{two_right, 0, 0}));

    for (uint32_t global_var = 0; global_var < 4u; ++global_var) {
        EXPECT_TRUE(same_structure(
            stack_one.normalizer.normalize(
                framed_expr{stack_one.pool.make_var(global_var), 0u}),
            stack_two.normalizer.normalize(
                framed_expr{stack_two.pool.make_var(global_var), 0u})));
    }
}
