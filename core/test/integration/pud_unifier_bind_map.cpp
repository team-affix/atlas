#include <algorithm>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"
#include "reference_pud_unify.hpp"

struct PudUnifierBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;

    const expr* var(uint32_t index) { return stack.pool.make_var(index); }
    const expr* atom(const char* name) { return stack.pool.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.pool.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(PudUnifierBindMapIntegrationTest, VarVarUnifySharesRepPointer) {
    EXPECT_TRUE(stack.unifier.unify(framed_expr{var(0), 0}, framed_expr{var(1), 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u}));
}

TEST_F(PudUnifierBindMapIntegrationTest, VarFunctorBindsNormalizedPointer) {
    const expr* h = stack.pool.make_functor(functors.id("h"), {});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{var(4), 0}, framed_expr{h, 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(4u), 0u}), h);
}

TEST_F(PudUnifierBindMapIntegrationTest, IdenticalSkeletonsNoBindMapWrites) {
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    EXPECT_TRUE(stack.unifier.unify(framed_expr{atom("a"), 0}, framed_expr{atom("a"), 0}));
    pud_ubs_state after = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after};
    EXPECT_TRUE(before == after);
}

TEST_F(PudUnifierBindMapIntegrationTest, MismatchedIdsAndArityFail) {
    const expr* fa = stack.pool.make_functor(functors.id("a"), {});
    const expr* gb = stack.pool.make_functor(functors.id("b"), {});
    EXPECT_FALSE(stack.unifier.unify(framed_expr{fa, 0}, framed_expr{gb, 0}));
    const expr* f1 = stack.pool.make_functor(functors.id("p"), {var(0)});
    const expr* f2 = stack.pool.make_functor(functors.id("p"), {var(0), var(1)});
    EXPECT_FALSE(stack.unifier.unify(framed_expr{f1, 0}, framed_expr{f2, 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, NestedUnifyBindsInnerVars) {
    const expr* fi0 = stack.pool.make_functor(functors.id("f"), {var(0)});
    const expr* fi2 = stack.pool.make_functor(functors.id("f"), {var(2)});
    const expr* lhs = stack.pool.make_functor(functors.id("g"), {fi0, var(1)});
    const expr* rhs = stack.pool.make_functor(functors.id("g"), {fi2, var(3)});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{lhs, 0}, framed_expr{rhs, 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), stack.normalizer.normalize(framed_expr{stack.pool.make_var(2u), 0u}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u}), stack.normalizer.normalize(framed_expr{stack.pool.make_var(3u), 0u}));
}

TEST_F(PudUnifierBindMapIntegrationTest, RepeatedVarFailsOrSucceeds) {
    EXPECT_FALSE(stack.unifier.unify(framed_expr{functor("pair", {var(6), var(6)}), 0}, framed_expr{functor("pair", {atom("a"), atom("b")}), 0}));
    pud_term_stack ok;
    const expr* dup = ok.pool.make_functor(functors.id("pair"), {
        ok.pool.make_var(6u), ok.pool.make_var(6u)});
    const expr* aa = ok.pool.make_functor(functors.id("pair"), {
        ok.pool.make_functor(functors.id("a"), {}),
        ok.pool.make_functor(functors.id("a"), {})});
    EXPECT_TRUE(ok.unifier.unify(framed_expr{dup, 0}, framed_expr{aa, 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, CrossArgBindsBothToAtom) {
    const expr* f_xy = stack.pool.make_functor(functors.id("q"), {var(0), var(1)});
    const expr* f_y_a = stack.pool.make_functor(functors.id("q"), {var(1), atom("a")});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{f_xy, 0}, framed_expr{f_y_a, 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), atom("a"));
}

TEST_F(PudUnifierBindMapIntegrationTest, LaterUnifyRespectsBinding) {
    EXPECT_TRUE(stack.unifier.unify(framed_expr{var(2), 0}, framed_expr{atom("a"), 0}));
    EXPECT_FALSE(stack.unifier.unify(framed_expr{var(2), 0}, framed_expr{atom("b"), 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, EqualBoundFunctorsUnifyDifferentOnesFail) {
    pud_term_stack stack_a;
    pud_term_stack stack_b;
    EXPECT_TRUE(stack_a.unifier.unify(framed_expr{stack_a.pool.make_var(0u), 0}, framed_expr{atom("a"), 0}));
    EXPECT_TRUE(stack_b.unifier.unify(framed_expr{stack_b.pool.make_var(0u), 0}, framed_expr{atom("a"), 0}));
    pud_term_stack merged;
    const expr* bound_a = merged.pool.make_functor(functors.id("a"), {});
    const expr* bound_b = merged.pool.make_functor(functors.id("b"), {});
    merged.bind_map.equate(merged.bind_map.find(0u), merged.bind_map.find(1u));
    merged.bind_map.instantiate(merged.bind_map.find(0u), framed_functor_from_framed_expr(framed_expr{merged.pool.make_functor(functors.id("a"), {}), 0u}));
    EXPECT_TRUE(merged.unifier.unify(framed_expr{merged.pool.make_var(0u), 0}, framed_expr{merged.pool.make_var(1u), 0}));
    merged.bind_map.instantiate(merged.bind_map.find(2u), framed_functor_from_framed_expr(framed_expr{merged.pool.make_functor(functors.id("b"), {}), 0u}));
    EXPECT_FALSE(merged.unifier.unify(framed_expr{merged.pool.make_var(0u), 0}, framed_expr{merged.pool.make_var(2u), 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, StoredValueIsFullyNormalized) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* a_atom = s.pool.make_functor(functors.id("a"), {});
    s.bind_map.instantiate(s.bind_map.find(1u), framed_functor_from_framed_expr(framed_expr{s.pool.make_functor(functors.id("a"), {}), 0u}));
    const expr* fx = s.pool.make_functor(functors.id("wrap"), {y});
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{fx, 0}));
    const expr* expected = s.normalizer.normalize(framed_expr{fx, 0});
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(0u), 0u}), expected);
}

TEST_F(PudUnifierBindMapIntegrationTest, LooksThroughLaterBinding) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* fx = s.pool.make_functor(functors.id("wrap"), {y});
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{fx, 0}));
    const expr* a_atom = s.pool.make_functor(functors.id("a"), {});
    const expr* b_atom = s.pool.make_functor(functors.id("b"), {});
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(1u), 0}, framed_expr{a_atom, 0}));
    const expr* fa = s.pool.make_functor(functors.id("wrap"), {a_atom});
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{fa, 0}));
    const expr* fb = s.pool.make_functor(functors.id("wrap"), {b_atom});
    EXPECT_FALSE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{fb, 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursDirectCycleBothOrders) {
    pud_term_stack s;
    const expr* loop = s.pool.make_functor(functors.id("loop"), {s.pool.make_var(0u)});
    EXPECT_FALSE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{loop, 0}));
    pud_term_stack reversed;
    const expr* reversed_loop = reversed.pool.make_functor(
        functors.id("loop"), {reversed.pool.make_var(0u)});
    EXPECT_FALSE(reversed.unifier.unify(framed_expr{reversed_loop, 0}, framed_expr{reversed.pool.make_var(0u), 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursIndirectAndSameClass) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* fx = s.pool.make_functor(functors.id("f"), {y});
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{fx, 0}));
    EXPECT_FALSE(s.unifier.unify(framed_expr{s.pool.make_var(1u), 0}, framed_expr{s.pool.make_var(0u), 0}));
    pud_term_stack s2;
    const expr* fy = s2.pool.make_functor(functors.id("f"), {s2.pool.make_var(1u)});
    const expr* gx = s2.pool.make_functor(functors.id("g"), {s2.pool.make_var(0u)});
    EXPECT_TRUE(s2.unifier.unify(framed_expr{s2.pool.make_var(0u), 0}, framed_expr{fy, 0}));
    EXPECT_FALSE(s2.unifier.unify(framed_expr{s2.pool.make_var(1u), 0}, framed_expr{gx, 0}));
    pud_term_stack s3;
    EXPECT_TRUE(s3.unifier.unify(framed_expr{s3.pool.make_var(0u), 0}, framed_expr{s3.pool.make_var(1u), 0}));
    const expr* f1 = s3.pool.make_functor(functors.id("h"), {s3.pool.make_var(1u)});
    EXPECT_FALSE(s3.unifier.unify(framed_expr{s3.pool.make_var(0u), 0}, framed_expr{f1, 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursDuplicateArgIsNotACycle) {
    pud_term_stack s;
    const expr* yy = s.pool.make_functor(functors.id("pair"), {s.pool.make_var(1u), s.pool.make_var(1u)});
    const expr* fyy = s.pool.make_functor(functors.id("f"), {yy});
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 0}, framed_expr{fyy, 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursRespectsGlobalIndicesAndFrames) {
    pud_term_stack s;
    const expr* f10_0 = s.pool.make_functor(
        functors.id("p"), {s.pool.make_var(10u), s.pool.make_var(0u)});
    EXPECT_FALSE(s.unifier.unify(framed_expr{var(10), 0}, framed_expr{f10_0, 0}));
    pud_term_stack fresh;
    const expr* f0 = fresh.pool.make_functor(functors.id("p"), {fresh.pool.make_var(0u)});
    EXPECT_TRUE(fresh.unifier.unify(framed_expr{fresh.pool.make_var(10u), 0}, framed_expr{f0, 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, CalleeAndCallerShareClassRegardlessOfOrder) {
    pud_term_stack s;
    EXPECT_TRUE(s.unifier.unify(framed_expr{s.pool.make_var(0u), 4}, framed_expr{s.pool.make_var(3u), 0}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(4u), 0u}), s.normalizer.normalize(framed_expr{s.pool.make_var(3u), 0u}));
    pud_term_stack reverse;
    EXPECT_TRUE(reverse.unifier.unify(framed_expr{reverse.pool.make_var(3u), 0}, framed_expr{reverse.pool.make_var(0u), 4}));
    EXPECT_EQ(reverse.normalizer.normalize(framed_expr{reverse.pool.make_var(3u), 0u}), reverse.normalizer.normalize(framed_expr{reverse.pool.make_var(4u), 0u}));
}

TEST_F(PudUnifierBindMapIntegrationTest, MultiFrameClassResolvesToOutermostRep) {
    const uint32_t low = stack.globalize.globalize(0, 20);
    const uint32_t high = stack.globalize.globalize(4, 17);
    stack.bind_map.equate(stack.bind_map.find(low), stack.bind_map.find(high));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(high), 0u}), stack.pool.make_var(low));
}

TEST_F(PudUnifierBindMapIntegrationTest, SymmetryAndIdempotence) {
    pud_term_stack forward;
    pud_term_stack backward;
    const expr* fxa = forward.pool.make_functor(functors.id("p"), {
        forward.pool.make_var(0u), atom("x")});
    const expr* fbc = forward.pool.make_functor(functors.id("p"), {
        atom("b"), atom("y")});
    EXPECT_EQ(
        forward.unifier.unify(framed_expr{fxa, 0}, framed_expr{fbc, 0}).has_value(),
        backward.unifier.unify(framed_expr{fbc, 0}, framed_expr{fxa, 0}).has_value());
    pud_term_stack twice;
    pud_ubs_state before = std::move(twice.bind_map).persist();
    twice.bind_map = ubs_bind_map{before};
    EXPECT_TRUE(twice.unifier.unify(framed_expr{twice.pool.make_var(3u), 0}, framed_expr{twice.pool.make_var(4u), 0}));
    pud_ubs_state mid = std::move(twice.bind_map).persist();
    twice.bind_map = ubs_bind_map{mid};
    EXPECT_FALSE(before == mid);
    EXPECT_TRUE(twice.unifier.unify(framed_expr{twice.pool.make_var(3u), 0}, framed_expr{twice.pool.make_var(4u), 0}));
    pud_ubs_state end = std::move(twice.bind_map).persist();
    twice.bind_map = ubs_bind_map{end};
    EXPECT_TRUE(mid == end);
}

TEST_F(PudUnifierBindMapIntegrationTest, SuccessNormalizesToSamePointer) {
    const expr* left = stack.pool.make_functor(functors.id("pair"), {var(0), atom("a")});
    const expr* right = stack.pool.make_functor(functors.id("pair"), {atom("a"), var(1)});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{left, 0}, framed_expr{right, 0}));
    EXPECT_EQ(
        stack.normalizer.normalize(framed_expr{var(0), 0}),
        stack.normalizer.normalize(framed_expr{var(1), 0}));
}

TEST_F(PudUnifierBindMapIntegrationTest, NaiveReferenceWithOccursCheck) {
    std::mt19937 rng(7);
    for (int trial = 0; trial < 25; ++trial) {
        pud_term_stack prod;
        pud_term_stack ref;
        const expr* vars[2] = {
            prod.pool.make_var(0u),
            prod.pool.make_var(1u),
        };
        const expr* atoms[2] = {
            prod.pool.make_functor(functors.id("r"), {}),
            prod.pool.make_functor(functors.id("s"), {}),
        };
        const expr* ref_vars[2] = {
            ref.pool.make_var(0u),
            ref.pool.make_var(1u),
        };
        const expr* ref_atoms[2] = {
            ref.pool.make_functor(functors.id("r"), {}),
            ref.pool.make_functor(functors.id("s"), {}),
        };
        framed_expr left = framed_expr{vars[0], 0};
        framed_expr right = framed_expr{vars[1], 0};
        framed_expr ref_left = framed_expr{ref_vars[0], 0};
        framed_expr ref_right = framed_expr{ref_vars[1], 0};
        const int choice = static_cast<int>(rng() % 4);
        if (choice == 0) {
            right = framed_expr{atoms[0], 0};
            ref_right = framed_expr{ref_atoms[0], 0};
        }
        if (choice == 1) {
            left = framed_expr{atoms[0], 0};
            ref_left = framed_expr{ref_atoms[0], 0};
        }
        if (choice == 2) {
            left = framed_expr{atoms[0], 0};
            right = framed_expr{atoms[1], 0};
            ref_left = framed_expr{ref_atoms[0], 0};
            ref_right = framed_expr{ref_atoms[1], 0};
        }
        EXPECT_EQ(prod.unifier.unify(left, right).has_value(), reference_pud_unify_terms(ref, ref_left, ref_right));
    }
}

TEST_F(PudUnifierBindMapIntegrationTest, KnownLimitationPartialBindingsOnFailure) {
    const expr* fxa = stack.pool.make_functor(functors.id("p"), {var(0), atom("x")});
    const expr* fbc = stack.pool.make_functor(functors.id("p"), {atom("b"), atom("y")});
    EXPECT_FALSE(stack.unifier.unify(framed_expr{fxa, 0}, framed_expr{fbc, 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), atom("b"));
}
