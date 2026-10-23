#include <algorithm>
#include <random>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"

namespace {

framed_expr fe(const expr* skeleton, uint32_t frame_offset) {
    return framed_expr{skeleton, frame_offset};
}

uint32_t gkey(globalizer& globalize, uint32_t frame, uint32_t local) {
    return globalize.globalize(frame, local);
}

struct naive_state {
    std::unordered_map<uint32_t, const expr*> subst;
};

bool naive_occurs(uint32_t var_index, const expr* term) {
    if (const expr::var* var = std::get_if<expr::var>(&term->content))
        return var->index == var_index;
    const expr::functor& functor = std::get<expr::functor>(term->content);
    for (const expr* arg : functor.args)
        if (naive_occurs(var_index, arg))
            return true;
    return false;
}

const expr* naive_follow(const naive_state& state, const expr* term) {
    if (const expr::var* var = std::get_if<expr::var>(&term->content)) {
        const auto entry = state.subst.find(var->index);
        if (entry == state.subst.end())
            return term;
        return naive_follow(state, entry->second);
    }
    return term;
}

bool naive_unify(
    naive_state& state,
    pud_term_stack& stack,
    framed_expr lhs,
    framed_expr rhs) {
    if (const expr::var* var = std::get_if<expr::var>(&lhs.skeleton->content))
        lhs = {stack.resolve(gkey(stack.globalize, lhs.frame_offset, var->index)), 0u};
    if (const expr::var* var = std::get_if<expr::var>(&rhs.skeleton->content))
        rhs = {stack.resolve(gkey(stack.globalize, rhs.frame_offset, var->index)), 0u};

    const expr::var* lhs_var = std::get_if<expr::var>(&lhs.skeleton->content);
    const expr::var* rhs_var = std::get_if<expr::var>(&rhs.skeleton->content);

    const uint32_t lhs_global = lhs_var ? gkey(stack.globalize, lhs.frame_offset, lhs_var->index) : 0;
    const uint32_t rhs_global = rhs_var ? gkey(stack.globalize, rhs.frame_offset, rhs_var->index) : 0;

    if (lhs_var && rhs_var && lhs_global == rhs_global)
        return true;

    if (lhs_var && rhs_var) {
        state.subst[lhs_var->index] = rhs.skeleton;
        state.subst[rhs_var->index] = rhs.skeleton;
        return true;
    }
    if (lhs_var) {
        const expr* value = stack.normalizer.normalize(rhs);
        if (naive_occurs(lhs_var->index, value))
            return false;
        state.subst[lhs_var->index] = value;
        return true;
    }
    if (rhs_var) {
        const expr* value = stack.normalizer.normalize(lhs);
        if (naive_occurs(rhs_var->index, value))
            return false;
        state.subst[rhs_var->index] = value;
        return true;
    }

    const expr::functor& lf = std::get<expr::functor>(lhs.skeleton->content);
    const expr::functor& rf = std::get<expr::functor>(rhs.skeleton->content);
    if (lf.id != rf.id || lf.args.size() != rf.args.size())
        return false;
    for (size_t i = 0; i < lf.args.size(); ++i) {
        if (!naive_unify(state, stack, fe(lf.args[i], lhs.frame_offset), fe(rf.args[i], rhs.frame_offset)))
            return false;
    }
    return true;
}

}

struct PudUnifierBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;

    const expr* v0() { return stack.pool.make_var(0u); }
    const expr* v1() { return stack.pool.make_var(1u); }
    const expr* v2() { return stack.pool.make_var(2u); }
    const expr* v3() { return stack.pool.make_var(3u); }
    const expr* v4() { return stack.pool.make_var(4u); }
    const expr* v6() { return stack.pool.make_var(6u); }
    const expr* v10() { return stack.pool.make_var(10u); }

    const expr* atom_a() { return stack.pool.make_functor(functors.id("a"), {}); }
    const expr* atom_b() { return stack.pool.make_functor(functors.id("b"), {}); }
    const expr* atom_x() { return stack.pool.make_functor(functors.id("x"), {}); }
    const expr* atom_y() { return stack.pool.make_functor(functors.id("y"), {}); }

    const expr* f_dup() { return stack.pool.make_functor(functors.id("pair"), {v6(), v6()}); }
    const expr* f_aa() { return stack.pool.make_functor(functors.id("pair"), {atom_a(), atom_a()}); }
    const expr* f_ab() { return stack.pool.make_functor(functors.id("pair"), {atom_a(), atom_b()}); }
};

TEST_F(PudUnifierBindMapIntegrationTest, VarVarUnifySharesRepPointer) {
    EXPECT_TRUE(stack.unifier.unify(fe(v0(), 0), fe(v1(), 0)));
    EXPECT_EQ(stack.resolve(0u), stack.resolve(1u));
}

TEST_F(PudUnifierBindMapIntegrationTest, VarFunctorBindsNormalizedPointer) {
    const expr* h = stack.pool.make_functor(functors.id("h"), {});
    EXPECT_TRUE(stack.unifier.unify(fe(v4(), 0), fe(h, 0)));
    EXPECT_EQ(stack.resolve(4u), h);
}

TEST_F(PudUnifierBindMapIntegrationTest, IdenticalSkeletonsNoBindMapWrites) {
    const pud_ubs_state before = stack.snapshot();
    EXPECT_TRUE(stack.unifier.unify(fe(atom_a(), 0), fe(atom_a(), 0)));
    EXPECT_TRUE(before == stack.snapshot());
}

TEST_F(PudUnifierBindMapIntegrationTest, MismatchedIdsAndArityFail) {
    const expr* fa = stack.pool.make_functor(functors.id("a"), {});
    const expr* gb = stack.pool.make_functor(functors.id("b"), {});
    EXPECT_FALSE(stack.unifier.unify(fe(fa, 0), fe(gb, 0)));
    const expr* f1 = stack.pool.make_functor(functors.id("p"), {v0()});
    const expr* f2 = stack.pool.make_functor(functors.id("p"), {v0(), v1()});
    EXPECT_FALSE(stack.unifier.unify(fe(f1, 0), fe(f2, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, NestedUnifyBindsInnerVars) {
    const expr* fi0 = stack.pool.make_functor(functors.id("f"), {v0()});
    const expr* fi2 = stack.pool.make_functor(functors.id("f"), {v2()});
    const expr* lhs = stack.pool.make_functor(functors.id("g"), {fi0, v1()});
    const expr* rhs = stack.pool.make_functor(functors.id("g"), {fi2, v3()});
    EXPECT_TRUE(stack.unifier.unify(fe(lhs, 0), fe(rhs, 0)));
    EXPECT_EQ(stack.resolve(0u), stack.resolve(2u));
    EXPECT_EQ(stack.resolve(1u), stack.resolve(3u));
}

TEST_F(PudUnifierBindMapIntegrationTest, RepeatedVarFailsOrSucceeds) {
    EXPECT_FALSE(stack.unifier.unify(fe(f_dup(), 0), fe(f_ab(), 0)));
    pud_term_stack ok;
    const expr* dup = ok.pool.make_functor(functors.id("pair"), {
        ok.pool.make_var(6u), ok.pool.make_var(6u)});
    const expr* aa = ok.pool.make_functor(functors.id("pair"), {
        ok.pool.make_functor(functors.id("a"), {}),
        ok.pool.make_functor(functors.id("a"), {})});
    EXPECT_TRUE(ok.unifier.unify(fe(dup, 0), fe(aa, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, CrossArgBindsBothToAtom) {
    const expr* f_xy = stack.pool.make_functor(functors.id("q"), {v0(), v1()});
    const expr* f_y_a = stack.pool.make_functor(functors.id("q"), {v1(), atom_a()});
    EXPECT_TRUE(stack.unifier.unify(fe(f_xy, 0), fe(f_y_a, 0)));
    EXPECT_EQ(stack.resolve(0u), stack.resolve(1u));
    EXPECT_EQ(stack.resolve(0u), atom_a());
}

TEST_F(PudUnifierBindMapIntegrationTest, LaterUnifyRespectsBinding) {
    EXPECT_TRUE(stack.unifier.unify(fe(v2(), 0), fe(atom_a(), 0)));
    EXPECT_FALSE(stack.unifier.unify(fe(v2(), 0), fe(atom_b(), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, EqualBoundFunctorsUnifyDifferentOnesFail) {
    pud_term_stack stack_a;
    pud_term_stack stack_b;
    EXPECT_TRUE(stack_a.unifier.unify(fe(stack_a.pool.make_var(0u), 0), fe(atom_a(), 0)));
    EXPECT_TRUE(stack_b.unifier.unify(fe(stack_b.pool.make_var(0u), 0), fe(atom_a(), 0)));
    pud_term_stack merged;
    const expr* bound_a = merged.pool.make_functor(functors.id("a"), {});
    const expr* bound_b = merged.pool.make_functor(functors.id("b"), {});
    merged.equate(0u, 1u);
    merged.instantiate(0u, framed_expr{merged.pool.make_functor(functors.id("a"), {}), 0u});
    EXPECT_TRUE(merged.unifier.unify(fe(merged.pool.make_var(0u), 0), fe(merged.pool.make_var(1u), 0)));
    merged.instantiate(2u, framed_expr{merged.pool.make_functor(functors.id("b"), {}), 0u});
    EXPECT_FALSE(merged.unifier.unify(fe(merged.pool.make_var(0u), 0), fe(merged.pool.make_var(2u), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, StoredValueIsFullyNormalized) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* a_atom = s.pool.make_functor(functors.id("a"), {});
    s.instantiate(1u, framed_expr{s.pool.make_functor(functors.id("a"), {}), 0u});
    const expr* fx = s.pool.make_functor(functors.id("wrap"), {y});
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(fx, 0)));
    const expr* expected = s.normalizer.normalize(fe(fx, 0));
    EXPECT_EQ(s.resolve(0u), expected);
}

TEST_F(PudUnifierBindMapIntegrationTest, LooksThroughLaterBinding) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* fx = s.pool.make_functor(functors.id("wrap"), {y});
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(fx, 0)));
    const expr* a_atom = s.pool.make_functor(functors.id("a"), {});
    const expr* b_atom = s.pool.make_functor(functors.id("b"), {});
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(1u), 0), fe(a_atom, 0)));
    const expr* fa = s.pool.make_functor(functors.id("wrap"), {a_atom});
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(fa, 0)));
    const expr* fb = s.pool.make_functor(functors.id("wrap"), {b_atom});
    EXPECT_FALSE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(fb, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursDirectCycleBothOrders) {
    pud_term_stack s;
    const expr* loop = s.pool.make_functor(functors.id("loop"), {s.pool.make_var(0u)});
    EXPECT_FALSE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(loop, 0)));
    pud_term_stack reversed;
    const expr* reversed_loop = reversed.pool.make_functor(
        functors.id("loop"), {reversed.pool.make_var(0u)});
    EXPECT_FALSE(reversed.unifier.unify(fe(reversed_loop, 0), fe(reversed.pool.make_var(0u), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursIndirectAndSameClass) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* fx = s.pool.make_functor(functors.id("f"), {y});
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(fx, 0)));
    EXPECT_FALSE(s.unifier.unify(fe(s.pool.make_var(1u), 0), fe(s.pool.make_var(0u), 0)));
    pud_term_stack s2;
    const expr* fy = s2.pool.make_functor(functors.id("f"), {s2.pool.make_var(1u)});
    const expr* gx = s2.pool.make_functor(functors.id("g"), {s2.pool.make_var(0u)});
    EXPECT_TRUE(s2.unifier.unify(fe(s2.pool.make_var(0u), 0), fe(fy, 0)));
    EXPECT_FALSE(s2.unifier.unify(fe(s2.pool.make_var(1u), 0), fe(gx, 0)));
    pud_term_stack s3;
    EXPECT_TRUE(s3.unifier.unify(fe(s3.pool.make_var(0u), 0), fe(s3.pool.make_var(1u), 0)));
    const expr* f1 = s3.pool.make_functor(functors.id("h"), {s3.pool.make_var(1u)});
    EXPECT_FALSE(s3.unifier.unify(fe(s3.pool.make_var(0u), 0), fe(f1, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursDuplicateArgIsNotACycle) {
    pud_term_stack s;
    const expr* yy = s.pool.make_functor(functors.id("pair"), {s.pool.make_var(1u), s.pool.make_var(1u)});
    const expr* fyy = s.pool.make_functor(functors.id("f"), {yy});
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(0u), 0), fe(fyy, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursRespectsGlobalIndicesAndFrames) {
    pud_term_stack s;
    const expr* f10_0 = s.pool.make_functor(
        functors.id("p"), {s.pool.make_var(10u), s.pool.make_var(0u)});
    EXPECT_FALSE(s.unifier.unify(fe(v10(), 0), fe(f10_0, 0)));
    pud_term_stack fresh;
    const expr* f0 = fresh.pool.make_functor(functors.id("p"), {fresh.pool.make_var(0u)});
    EXPECT_TRUE(fresh.unifier.unify(fe(fresh.pool.make_var(10u), 0), fe(f0, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, CalleeAndCallerShareClassRegardlessOfOrder) {
    pud_term_stack s;
    EXPECT_TRUE(s.unifier.unify(fe(s.pool.make_var(0u), 4), fe(s.pool.make_var(3u), 0)));
    EXPECT_EQ(s.resolve(4u), s.resolve(3u));
    pud_term_stack reverse;
    EXPECT_TRUE(reverse.unifier.unify(fe(reverse.pool.make_var(3u), 0), fe(reverse.pool.make_var(0u), 4)));
    EXPECT_EQ(reverse.resolve(3u), reverse.resolve(4u));
}

TEST_F(PudUnifierBindMapIntegrationTest, MultiFrameClassResolvesToOutermostRep) {
    const uint32_t low = gkey(stack.globalize, 0, 20);
    const uint32_t high = gkey(stack.globalize, 4, 17);
    stack.equate(low, high);
    EXPECT_EQ(stack.resolve(high), stack.pool.make_var(low));
}

TEST_F(PudUnifierBindMapIntegrationTest, SymmetryAndIdempotence) {
    pud_term_stack forward;
    pud_term_stack backward;
    const expr* fxa = forward.pool.make_functor(functors.id("p"), {
        forward.pool.make_var(0u), atom_x()});
    const expr* fbc = forward.pool.make_functor(functors.id("p"), {
        atom_b(), atom_y()});
    EXPECT_EQ(
        forward.unifier.unify(fe(fxa, 0), fe(fbc, 0)),
        backward.unifier.unify(fe(fbc, 0), fe(fxa, 0)));
    pud_term_stack twice;
    const pud_ubs_state before = twice.snapshot();
    EXPECT_TRUE(twice.unifier.unify(fe(twice.pool.make_var(3u), 0), fe(twice.pool.make_var(4u), 0)));
    const pud_ubs_state mid = twice.snapshot();
    EXPECT_FALSE(before == mid);
    EXPECT_TRUE(twice.unifier.unify(fe(twice.pool.make_var(3u), 0), fe(twice.pool.make_var(4u), 0)));
    EXPECT_TRUE(mid == twice.snapshot());
}

TEST_F(PudUnifierBindMapIntegrationTest, SuccessNormalizesToSamePointer) {
    const expr* left = stack.pool.make_functor(functors.id("pair"), {v0(), atom_a()});
    const expr* right = stack.pool.make_functor(functors.id("pair"), {atom_a(), v1()});
    EXPECT_TRUE(stack.unifier.unify(fe(left, 0), fe(right, 0)));
    EXPECT_EQ(
        stack.normalizer.normalize(fe(v0(), 0)),
        stack.normalizer.normalize(fe(v1(), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, NaiveReferenceWithOccursCheck) {
    std::mt19937 rng(7);
    for (int trial = 0; trial < 25; ++trial) {
        pud_term_stack prod;
        naive_state naive;
        const expr* vars[2] = {
            prod.pool.make_var(0u),
            prod.pool.make_var(1u),
        };
        const expr* atoms[2] = {
            prod.pool.make_functor(functors.id("r"), {}),
            prod.pool.make_functor(functors.id("s"), {}),
        };
        framed_expr left = fe(vars[0], 0);
        framed_expr right = fe(vars[1], 0);
        const int choice = static_cast<int>(rng() % 4);
        if (choice == 0)
            right = fe(atoms[0], 0);
        if (choice == 1)
            left = fe(atoms[0], 0);
        if (choice == 2) {
            left = fe(atoms[0], 0);
            right = fe(atoms[1], 0);
        }
        EXPECT_EQ(prod.unifier.unify(left, right), naive_unify(naive, prod, left, right));
    }
}

TEST_F(PudUnifierBindMapIntegrationTest, KnownLimitationPartialBindingsOnFailure) {
    const expr* fxa = stack.pool.make_functor(functors.id("p"), {v0(), atom_x()});
    const expr* fbc = stack.pool.make_functor(functors.id("p"), {atom_b(), atom_y()});
    EXPECT_FALSE(stack.unifier.unify(fe(fxa, 0), fe(fbc, 0)));
    EXPECT_EQ(stack.resolve(0u), atom_b());
}
