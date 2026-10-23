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

    const expr* app(uint32_t functor_id, const std::vector<const expr*>& args) {
        return pool_.make_functor(functor_id, args);
    }

    const expr* app(const char* name, const std::vector<const expr*>& args) {
        return app(functors_.id(name), args);
    }
};

void expect_resolve_structurally(
    pud_term_stack& stack,
    uint32_t global_var,
    const expr* expected_shape) {
    EXPECT_TRUE(same_structure(stack.resolve(global_var), expected_shape));
}

uint32_t gkey(globalizer& globalize, uint32_t frame, uint32_t local) {
    return globalize.globalize(frame, local);
}

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
    const pud_ubs_state before = stack.snapshot();
    EXPECT_TRUE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    EXPECT_TRUE(before == stack.snapshot());
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolVarsMergeByGlobalIndexNotPointer) {
    const expr* left = pool_a.var(0);
    const expr* right = pool_b.var(1);
    EXPECT_NE(left, right);
    EXPECT_TRUE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    const expr* rep = stack.resolve(0u);
    EXPECT_TRUE(same_structure(rep, stack.resolve(1u)));
    EXPECT_TRUE(same_structure(rep, canonical.var(0)));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolDeepMirrorBindsAllLocals) {
    const expr* left = pool_a.app("g", {
        pool_a.app("f", {pool_a.var(0)}),
        pool_a.var(1),
    });
    const expr* right = pool_b.app("g", {
        pool_b.app("f", {pool_b.var(2)}),
        pool_b.var(3),
    });
    EXPECT_NE(left, right);
    EXPECT_TRUE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    expect_resolve_structurally(stack, 0u, canonical.var(0));
    expect_resolve_structurally(stack, 1u, canonical.var(1));
    expect_resolve_structurally(stack, 2u, canonical.var(0));
    expect_resolve_structurally(stack, 3u, canonical.var(1));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, VarWithForeignFunctorStoresNormalizedNotForeignPointer) {
    const expr* foreign_head = pool_a.app("h", {pool_a.var(0), pool_a.atom("a")});
    const expr* foreign_var = pool_b.var(4);
    EXPECT_NE(foreign_var, pool_a.var(4));
    EXPECT_TRUE(stack.unifier.unify(pud_fe(foreign_var, 0), pud_fe(foreign_head, 0)));
    const expr* bound = stack.resolve(4u);
    EXPECT_NE(bound, foreign_head);
    const expr* expected = canonical.app("h", {canonical.var(0), canonical.atom("a")});
    EXPECT_TRUE(same_structure(bound, expected));
    expect_resolve_structurally(stack, 4u, expected);
}

TEST_F(PudUnifierCrossPoolIntegrationTest, ForeignFunctorLeftVarRightSameNormalization) {
    const expr* foreign_var = pool_a.var(2);
    const expr* stack_head = canonical.app("p", {canonical.var(0), canonical.atom("b")});
    const expr* foreign_head = pool_b.app("p", {pool_b.var(0), pool_b.atom("b")});
    EXPECT_TRUE(stack.unifier.unify(pud_fe(foreign_head, 0), pud_fe(foreign_var, 0)));
    const expr* bound = stack.resolve(2u);
    EXPECT_NE(bound, foreign_head);
    EXPECT_TRUE(same_structure(bound, stack_head));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, SecondUnifyReusesBindingAcrossForeignSkeletons) {
    const expr* foreign_var = pool_a.var(1);
    EXPECT_TRUE(stack.unifier.unify(pud_fe(foreign_var, 0), pud_fe(pool_b.atom("k"), 0)));
    const expr* probe = pool_b.app("wrap", {pool_b.var(1)});
    const expr* target = pool_a.app("wrap", {pool_a.atom("k")});
    EXPECT_TRUE(stack.unifier.unify(pud_fe(probe, 0), pud_fe(target, 0)));
    expect_resolve_structurally(stack, 1u, canonical.atom("k"));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolSwapSidesSameStructuralResult) {
    pud_term_stack forward;
    pud_term_stack reverse;
    cross_pool_term_builder fwd_a{foreign_a, functors};
    expr_pool foreign_c;
    cross_pool_term_builder fwd_b{foreign_c, functors};
    cross_pool_term_builder rev_a{reverse.pool, functors};
    cross_pool_term_builder rev_b{foreign_b, functors};

    const expr* fwd_left = fwd_a.app("q", {fwd_a.var(0), fwd_a.atom("t")});
    const expr* fwd_right = fwd_b.app("q", {fwd_b.atom("t"), fwd_b.var(1)});
    const expr* rev_left = rev_b.app("q", {rev_b.atom("t"), rev_b.var(1)});
    const expr* rev_right = rev_a.app("q", {rev_a.var(0), rev_a.atom("t")});

    EXPECT_TRUE(forward.unifier.unify(pud_fe(fwd_left, 0), pud_fe(fwd_right, 0)));
    EXPECT_TRUE(reverse.unifier.unify(pud_fe(rev_left, 0), pud_fe(rev_right, 0)));

    for (uint32_t global_var = 0; global_var < 4u; ++global_var) {
        const expr* forward_value = forward.resolve(global_var);
        const expr* reverse_value = reverse.resolve(global_var);
        EXPECT_TRUE(same_structure(forward_value, reverse_value));
    }
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolFrameOffsetsUseGlobalsNotPoolIdentity) {
    const expr* left = pool_a.app("f", {pool_a.var(0)});
    const expr* right = pool_b.app("f", {pool_b.var(0)});
    EXPECT_TRUE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 10)));
    const uint32_t global_zero = gkey(stack.globalize, 0, 0);
    const uint32_t global_ten = gkey(stack.globalize, 10, 0);
    EXPECT_TRUE(same_structure(
        stack.resolve(global_zero), stack.resolve(global_ten)));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, MergedClassForeignMembersSkipRedundantEquate) {
    stack.equate(2u, 5u);
    const pud_ubs_state before = stack.snapshot();
    const expr* left = pool_a.var(2);
    const expr* right = pool_b.var(5);
    EXPECT_TRUE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    EXPECT_TRUE(before == stack.snapshot());
}

TEST_F(PudUnifierCrossPoolIntegrationTest, CrossPoolMismatchBindsFirstArgThenFails) {
    const expr* left = pool_a.app("pair", {pool_a.var(0), pool_a.atom("a")});
    const expr* right = pool_b.app("pair", {pool_b.atom("b"), pool_b.var(0)});
    const pud_ubs_state before = stack.snapshot();
    EXPECT_FALSE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    expect_resolve_structurally(stack, 0u, canonical.atom("b"));
    EXPECT_FALSE(same_structure(stack.resolve(0u), canonical.atom("a")));
    const pud_ubs_state after_partial = stack.snapshot();
    EXPECT_NE(before, after_partial);
}

TEST_F(PudUnifierCrossPoolIntegrationTest, OccursFailureWithForeignCyclicRhs) {
    const expr* foreign_loop = pool_a.app("loop", {pool_a.var(0)});
    const expr* foreign_var = pool_b.var(0);
    EXPECT_FALSE(stack.unifier.unify(pud_fe(foreign_var, 0), pud_fe(foreign_loop, 0)));
}

TEST_F(PudUnifierCrossPoolIntegrationTest, WideCrossPoolArityFiftySucceedsStructurally) {
    const uint32_t pid = functors.id("wide");
    std::vector<const expr*> left_args;
    std::vector<const expr*> right_args;
    for (uint32_t arg = 0; arg < 50u; ++arg) {
        left_args.push_back(pool_a.var(arg % 4u));
        right_args.push_back(pool_b.var(arg % 4u));
    }
    const expr* left = pool_a.app(pid, left_args);
    const expr* right = pool_b.app(pid, right_args);
    EXPECT_TRUE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    for (uint32_t global_var = 0; global_var < 4u; ++global_var)
        expect_resolve_structurally(stack, global_var, canonical.var(global_var));
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
            ref_left = ref_left_builder.app(nest_id, {ref_left, ref_left_builder.var(slot)});
            ref_right = ref_right_builder.app(nest_id, {ref_right, ref_right_builder.var(slot)});
        }
        EXPECT_NE(ref_left, ref_right);
        const bool reference_ok = reference_pud_unify_terms(
            reference_stack, pud_fe(ref_left, 0), pud_fe(ref_right, 0));

        pud_term_stack cross_stack;
        expr_pool left_pool;
        expr_pool right_pool;
        cross_pool_term_builder left_builder{left_pool, functors};
        cross_pool_term_builder right_builder{right_pool, functors};
        const expr* left_term = left_builder.atom(leaf_name);
        const expr* right_term = right_builder.atom(leaf_name);
        for (uint32_t slot : var_slots) {
            left_term = left_builder.app(nest_id, {left_term, left_builder.var(slot)});
            right_term = right_builder.app(nest_id, {right_term, right_builder.var(slot)});
        }
        EXPECT_NE(left_term, right_term);
        const bool cross_ok = cross_stack.unifier.unify(pud_fe(left_term, 0), pud_fe(right_term, 0));
        EXPECT_EQ(reference_ok, cross_ok);

        if (!reference_ok)
            continue;

        for (uint32_t global_var = 0; global_var < 8u; ++global_var) {
            EXPECT_TRUE(same_structure(
                reference_stack.resolve(global_var),
                cross_stack.resolve(global_var)));
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

    const expr* one_left = one_pool.app("m", {one_pool.var(0), one_pool.atom("a")});
    const expr* one_right = stack_one.pool.make_functor(
        functors.id("m"),
        {stack_one.pool.make_var(1u), stack_one.pool.make_functor(functors.id("a"), {})});
    const expr* two_left = two_pool.app("m", {two_pool.var(0), two_pool.atom("a")});
    const expr* two_right = stack_two.pool.make_functor(
        functors.id("m"), {stack_two.pool.make_var(1u), stack_two.pool.make_functor(functors.id("a"), {})});

    EXPECT_TRUE(stack_one.unifier.unify(pud_fe(one_left, 0), pud_fe(one_right, 0)));
    EXPECT_TRUE(stack_two.unifier.unify(pud_fe(two_left, 0), pud_fe(two_right, 0)));

    for (uint32_t global_var = 0; global_var < 4u; ++global_var) {
        EXPECT_TRUE(same_structure(
            stack_one.resolve(global_var),
            stack_two.resolve(global_var)));
    }
}
