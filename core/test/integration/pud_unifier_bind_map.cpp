#include <algorithm>
#include <random>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_unifier_bind_map_stack.hpp"

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
    pud_unifier_bind_map_stack& stack,
    framed_expr lhs,
    framed_expr rhs) {
    if (const expr::var* var = std::get_if<expr::var>(&lhs.skeleton->content))
        lhs = {stack.bind_map.resolve(gkey(stack.globalize, lhs.frame_offset, var->index)), 0u};
    if (const expr::var* var = std::get_if<expr::var>(&rhs.skeleton->content))
        rhs = {stack.bind_map.resolve(gkey(stack.globalize, rhs.frame_offset, var->index)), 0u};

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

} // namespace

struct PudUnifierBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_unifier_bind_map_stack stack;

    const expr* v0() { return stack.bind_backing.pool.make_var(0u); }
    const expr* v1() { return stack.bind_backing.pool.make_var(1u); }
    const expr* v2() { return stack.bind_backing.pool.make_var(2u); }
    const expr* v3() { return stack.bind_backing.pool.make_var(3u); }
    const expr* v4() { return stack.bind_backing.pool.make_var(4u); }
    const expr* v6() { return stack.bind_backing.pool.make_var(6u); }
    const expr* v10() { return stack.bind_backing.pool.make_var(10u); }

    const expr* atom_a() { return stack.bind_backing.pool.make_functor(functors.id("a"), {}); }
    const expr* atom_b() { return stack.bind_backing.pool.make_functor(functors.id("b"), {}); }
    const expr* atom_x() { return stack.bind_backing.pool.make_functor(functors.id("x"), {}); }
    const expr* atom_y() { return stack.bind_backing.pool.make_functor(functors.id("y"), {}); }

    const expr* f_dup() { return stack.bind_backing.pool.make_functor(functors.id("pair"), {v6(), v6()}); }
    const expr* f_aa() { return stack.bind_backing.pool.make_functor(functors.id("pair"), {atom_a(), atom_a()}); }
    const expr* f_ab() { return stack.bind_backing.pool.make_functor(functors.id("pair"), {atom_a(), atom_b()}); }
};

TEST_F(PudUnifierBindMapIntegrationTest, VarVarUnifySharesRepPointer) {
    EXPECT_TRUE(stack.framed_unifier.unify(fe(v0(), 0), fe(v1(), 0)));
    EXPECT_EQ(stack.bind_map.resolve(0u), stack.bind_map.resolve(1u));
}

TEST_F(PudUnifierBindMapIntegrationTest, VarFunctorBindsNormalizedPointer) {
    const expr* h = stack.bind_backing.pool.make_functor(functors.id("h"), {});
    EXPECT_TRUE(stack.framed_unifier.unify(fe(v4(), 0), fe(h, 0)));
    EXPECT_EQ(stack.bind_map.resolve(4u), h);
}

TEST_F(PudUnifierBindMapIntegrationTest, IdenticalSkeletonsNoBindMapWrites) {
    const ubs_map_snapshot before = snapshot_bind_backing(stack.bind_backing);
    EXPECT_TRUE(stack.framed_unifier.unify(fe(atom_a(), 0), fe(atom_a(), 0)));
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot_bind_backing(stack.bind_backing)));
}

TEST_F(PudUnifierBindMapIntegrationTest, MismatchedIdsAndArityFail) {
    const expr* fa = stack.bind_backing.pool.make_functor(functors.id("a"), {});
    const expr* gb = stack.bind_backing.pool.make_functor(functors.id("b"), {});
    EXPECT_FALSE(stack.framed_unifier.unify(fe(fa, 0), fe(gb, 0)));
    const expr* f1 = stack.bind_backing.pool.make_functor(functors.id("p"), {v0()});
    const expr* f2 = stack.bind_backing.pool.make_functor(functors.id("p"), {v0(), v1()});
    EXPECT_FALSE(stack.framed_unifier.unify(fe(f1, 0), fe(f2, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, NestedUnifyBindsInnerVars) {
    const expr* fi0 = stack.bind_backing.pool.make_functor(functors.id("f"), {v0()});
    const expr* fi2 = stack.bind_backing.pool.make_functor(functors.id("f"), {v2()});
    const expr* lhs = stack.bind_backing.pool.make_functor(functors.id("g"), {fi0, v1()});
    const expr* rhs = stack.bind_backing.pool.make_functor(functors.id("g"), {fi2, v3()});
    EXPECT_TRUE(stack.framed_unifier.unify(fe(lhs, 0), fe(rhs, 0)));
    EXPECT_EQ(stack.bind_map.resolve(0u), stack.bind_map.resolve(2u));
    EXPECT_EQ(stack.bind_map.resolve(1u), stack.bind_map.resolve(3u));
}

TEST_F(PudUnifierBindMapIntegrationTest, RepeatedVarFailsOrSucceeds) {
    EXPECT_FALSE(stack.framed_unifier.unify(fe(f_dup(), 0), fe(f_ab(), 0)));
    pud_unifier_bind_map_stack ok;
    const expr* dup = ok.bind_backing.pool.make_functor(functors.id("pair"), {
        ok.bind_backing.pool.make_var(6u), ok.bind_backing.pool.make_var(6u)});
    const expr* aa = ok.bind_backing.pool.make_functor(functors.id("pair"), {
        ok.bind_backing.pool.make_functor(functors.id("a"), {}),
        ok.bind_backing.pool.make_functor(functors.id("a"), {})});
    EXPECT_TRUE(ok.framed_unifier.unify(fe(dup, 0), fe(aa, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, CrossArgBindsBothToAtom) {
    const expr* f_xy = stack.bind_backing.pool.make_functor(functors.id("q"), {v0(), v1()});
    const expr* f_y_a = stack.bind_backing.pool.make_functor(functors.id("q"), {v1(), atom_a()});
    EXPECT_TRUE(stack.framed_unifier.unify(fe(f_xy, 0), fe(f_y_a, 0)));
    EXPECT_EQ(stack.bind_map.resolve(0u), stack.bind_map.resolve(1u));
    EXPECT_EQ(stack.bind_map.resolve(0u), atom_a());
}

TEST_F(PudUnifierBindMapIntegrationTest, LaterUnifyRespectsBinding) {
    EXPECT_TRUE(stack.framed_unifier.unify(fe(v2(), 0), fe(atom_a(), 0)));
    EXPECT_FALSE(stack.framed_unifier.unify(fe(v2(), 0), fe(atom_b(), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, EqualBoundFunctorsUnifyDifferentOnesFail) {
    pud_unifier_bind_map_stack stack_a;
    pud_unifier_bind_map_stack stack_b;
    EXPECT_TRUE(stack_a.framed_unifier.unify(fe(stack_a.bind_backing.pool.make_var(0u), 0), fe(atom_a(), 0)));
    EXPECT_TRUE(stack_b.framed_unifier.unify(fe(stack_b.bind_backing.pool.make_var(0u), 0), fe(atom_a(), 0)));
    pud_unifier_bind_map_stack merged;
    const expr* bound_a = merged.bind_backing.pool.make_functor(functors.id("a"), {});
    const expr* bound_b = merged.bind_backing.pool.make_functor(functors.id("b"), {});
    merged.bind_map.equate(0u, 1u);
    merged.bind_map.instantiate(0u, functors.id("a"), {});
    EXPECT_TRUE(merged.framed_unifier.unify(fe(merged.bind_backing.pool.make_var(0u), 0), fe(merged.bind_backing.pool.make_var(1u), 0)));
    merged.bind_map.instantiate(2u, functors.id("b"), {});
    EXPECT_FALSE(merged.framed_unifier.unify(fe(merged.bind_backing.pool.make_var(0u), 0), fe(merged.bind_backing.pool.make_var(2u), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, StoredValueIsFullyNormalized) {
    pud_unifier_bind_map_stack s;
    const expr* y = s.bind_backing.pool.make_var(1u);
    const expr* a_atom = s.bind_backing.pool.make_functor(functors.id("a"), {});
    s.bind_map.instantiate(1u, functors.id("a"), {});
    const expr* fx = s.bind_backing.pool.make_functor(functors.id("wrap"), {y});
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(fx, 0)));
    const expr* expected = s.normalizer.normalize(fe(fx, 0));
    EXPECT_EQ(s.bind_map.resolve(0u), expected);
}

TEST_F(PudUnifierBindMapIntegrationTest, LooksThroughLaterBinding) {
    pud_unifier_bind_map_stack s;
    const expr* y = s.bind_backing.pool.make_var(1u);
    const expr* fx = s.bind_backing.pool.make_functor(functors.id("wrap"), {y});
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(fx, 0)));
    const expr* a_atom = s.bind_backing.pool.make_functor(functors.id("a"), {});
    const expr* b_atom = s.bind_backing.pool.make_functor(functors.id("b"), {});
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(1u), 0), fe(a_atom, 0)));
    const expr* fa = s.bind_backing.pool.make_functor(functors.id("wrap"), {a_atom});
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(fa, 0)));
    const expr* fb = s.bind_backing.pool.make_functor(functors.id("wrap"), {b_atom});
    EXPECT_FALSE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(fb, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursDirectCycleBothOrders) {
    pud_unifier_bind_map_stack s;
    const expr* loop = s.bind_backing.pool.make_functor(functors.id("loop"), {s.bind_backing.pool.make_var(0u)});
    EXPECT_FALSE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(loop, 0)));
    EXPECT_FALSE(s.framed_unifier.unify(fe(loop, 0), fe(s.bind_backing.pool.make_var(0u), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursIndirectAndSameClass) {
    pud_unifier_bind_map_stack s;
    const expr* y = s.bind_backing.pool.make_var(1u);
    const expr* fx = s.bind_backing.pool.make_functor(functors.id("f"), {y});
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(fx, 0)));
    EXPECT_FALSE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(1u), 0), fe(s.bind_backing.pool.make_var(0u), 0)));
    pud_unifier_bind_map_stack s2;
    const expr* fy = s2.bind_backing.pool.make_functor(functors.id("f"), {s2.bind_backing.pool.make_var(1u)});
    const expr* gx = s2.bind_backing.pool.make_functor(functors.id("g"), {s2.bind_backing.pool.make_var(0u)});
    EXPECT_TRUE(s2.framed_unifier.unify(fe(s2.bind_backing.pool.make_var(0u), 0), fe(fy, 0)));
    EXPECT_FALSE(s2.framed_unifier.unify(fe(s2.bind_backing.pool.make_var(1u), 0), fe(gx, 0)));
    pud_unifier_bind_map_stack s3;
    EXPECT_TRUE(s3.framed_unifier.unify(fe(s3.bind_backing.pool.make_var(0u), 0), fe(s3.bind_backing.pool.make_var(1u), 0)));
    const expr* f1 = s3.bind_backing.pool.make_functor(functors.id("h"), {s3.bind_backing.pool.make_var(1u)});
    EXPECT_FALSE(s3.framed_unifier.unify(fe(s3.bind_backing.pool.make_var(0u), 0), fe(f1, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursDuplicateArgIsNotACycle) {
    pud_unifier_bind_map_stack s;
    const expr* yy = s.bind_backing.pool.make_functor(functors.id("pair"), {s.bind_backing.pool.make_var(1u), s.bind_backing.pool.make_var(1u)});
    const expr* fyy = s.bind_backing.pool.make_functor(functors.id("f"), {yy});
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 0), fe(fyy, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, OccursRespectsGlobalIndicesAndFrames) {
    pud_unifier_bind_map_stack s;
    const expr* f10_0 = s.bind_backing.pool.make_functor(
        functors.id("p"), {s.bind_backing.pool.make_var(10u), s.bind_backing.pool.make_var(0u)});
    EXPECT_FALSE(s.framed_unifier.unify(fe(v10(), 0), fe(f10_0, 0)));
    const expr* f0 = s.bind_backing.pool.make_functor(functors.id("p"), {s.bind_backing.pool.make_var(0u)});
    EXPECT_TRUE(s.framed_unifier.unify(fe(v10(), 0), fe(f0, 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, CalleeAndCallerShareClassRegardlessOfOrder) {
    pud_unifier_bind_map_stack s;
    EXPECT_TRUE(s.framed_unifier.unify(fe(s.bind_backing.pool.make_var(0u), 4), fe(s.bind_backing.pool.make_var(3u), 0)));
    EXPECT_EQ(s.bind_map.resolve(4u), s.bind_map.resolve(3u));
    pud_unifier_bind_map_stack reverse;
    EXPECT_TRUE(reverse.framed_unifier.unify(fe(reverse.bind_backing.pool.make_var(3u), 0), fe(reverse.bind_backing.pool.make_var(0u), 4)));
    EXPECT_EQ(reverse.bind_map.resolve(3u), reverse.bind_map.resolve(4u));
}

TEST_F(PudUnifierBindMapIntegrationTest, MultiFrameClassResolvesToOutermostRep) {
    const uint32_t low = gkey(stack.globalize, 0, 20);
    const uint32_t high = gkey(stack.globalize, 4, 17);
    stack.bind_map.equate(low, high);
    EXPECT_EQ(stack.bind_map.resolve(high), stack.bind_backing.pool.make_var(low));
}

TEST_F(PudUnifierBindMapIntegrationTest, SymmetryAndIdempotence) {
    pud_unifier_bind_map_stack forward;
    pud_unifier_bind_map_stack backward;
    const expr* fxa = forward.bind_backing.pool.make_functor(functors.id("p"), {
        forward.bind_backing.pool.make_var(0u), atom_x()});
    const expr* fbc = forward.bind_backing.pool.make_functor(functors.id("p"), {
        atom_b(), atom_y()});
    EXPECT_EQ(
        forward.framed_unifier.unify(fe(fxa, 0), fe(fbc, 0)),
        backward.framed_unifier.unify(fe(fbc, 0), fe(fxa, 0)));
    pud_unifier_bind_map_stack twice;
    const ubs_map_snapshot before = snapshot_bind_backing(twice.bind_backing);
    EXPECT_TRUE(twice.framed_unifier.unify(fe(twice.bind_backing.pool.make_var(3u), 0), fe(twice.bind_backing.pool.make_var(4u), 0)));
    const ubs_map_snapshot mid = snapshot_bind_backing(twice.bind_backing);
    EXPECT_FALSE(ubs_snapshots_equal(before, mid));
    EXPECT_TRUE(twice.framed_unifier.unify(fe(twice.bind_backing.pool.make_var(3u), 0), fe(twice.bind_backing.pool.make_var(4u), 0)));
    EXPECT_TRUE(ubs_snapshots_equal(mid, snapshot_bind_backing(twice.bind_backing)));
}

TEST_F(PudUnifierBindMapIntegrationTest, SuccessNormalizesToSamePointer) {
    const expr* left = stack.bind_backing.pool.make_functor(functors.id("pair"), {v0(), atom_a()});
    const expr* right = stack.bind_backing.pool.make_functor(functors.id("pair"), {atom_a(), v1()});
    EXPECT_TRUE(stack.framed_unifier.unify(fe(left, 0), fe(right, 0)));
    EXPECT_EQ(
        stack.normalizer.normalize(fe(v0(), 0)),
        stack.normalizer.normalize(fe(v1(), 0)));
}

TEST_F(PudUnifierBindMapIntegrationTest, NaiveReferenceWithOccursCheck) {
    std::mt19937 rng(7);
    for (int trial = 0; trial < 25; ++trial) {
        pud_unifier_bind_map_stack prod;
        naive_state naive;
        const expr* vars[2] = {
            prod.bind_backing.pool.make_var(0u),
            prod.bind_backing.pool.make_var(1u),
        };
        const expr* atoms[2] = {
            prod.bind_backing.pool.make_functor(functors.id("r"), {}),
            prod.bind_backing.pool.make_functor(functors.id("s"), {}),
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
        EXPECT_EQ(prod.framed_unifier.unify(left, right), naive_unify(naive, prod, left, right));
    }
}

TEST_F(PudUnifierBindMapIntegrationTest, KnownLimitationPartialBindingsOnFailure) {
    const expr* fxa = stack.bind_backing.pool.make_functor(functors.id("p"), {v0(), atom_x()});
    const expr* fbc = stack.bind_backing.pool.make_functor(functors.id("p"), {atom_b(), atom_y()});
    EXPECT_FALSE(stack.framed_unifier.unify(fe(fxa, 0), fe(fbc, 0)));
    EXPECT_EQ(stack.bind_map.resolve(0u), atom_b());
}
