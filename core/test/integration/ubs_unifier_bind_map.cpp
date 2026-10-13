#include <algorithm>
#include <deque>
#include <random>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/ubs_unifier.hpp"
#include "ubs_bind_map_harness.hpp"
#include "value_objects/expr.hpp"

namespace {

using store_t = ubs_bind_map_t;
using integration_unifier_t = ubs_unifier<globalizer, store_t, store_t, store_t>;

struct integration_stack {
    globalizer globalize;
    ubs_bind_map_backing backing;
    store_t store{backing.make_map()};
    integration_unifier_t unifier{globalize, store, store, store};
};

framed_expr fe(const expr* skeleton, uint32_t frame_offset) {
    return framed_expr{skeleton, frame_offset};
}

uint32_t gkey(globalizer& globalize, uint32_t frame, uint32_t local) {
    return ubs_global_key(globalize, frame, local);
}

framed_expr resolve_local(integration_stack& stack, uint32_t frame, uint32_t local) {
    return ubs_resolve_local(stack.store, stack.globalize, frame, local);
}

bool is_var_rep(const framed_expr& resolved, uint32_t expected_global) {
    const expr::var* var = std::get_if<expr::var>(&resolved.skeleton->content);
    if (!var)
        return false;
    return var->index == expected_global && resolved.frame_offset == 0u;
}

struct naive_state {
    std::unordered_map<uint32_t, framed_expr> binding;
};

framed_expr naive_follow(const naive_state& state, framed_expr term, globalizer& globalize) {
    const expr::var* var = std::get_if<expr::var>(&term.skeleton->content);
    if (!var)
        return term;
    const uint32_t key = gkey(globalize, term.frame_offset, var->index);
    const auto entry = state.binding.find(key);
    if (entry == state.binding.end())
        return term;
    return naive_follow(state, entry->second, globalize);
}

bool naive_unify(naive_state& state, framed_expr lhs, framed_expr rhs, globalizer& globalize) {
    lhs = naive_follow(state, lhs, globalize);
    rhs = naive_follow(state, rhs, globalize);

    const expr::var* lhs_var = std::get_if<expr::var>(&lhs.skeleton->content);
    const expr::var* rhs_var = std::get_if<expr::var>(&rhs.skeleton->content);

    if (lhs_var) {
        const uint32_t key = gkey(globalize, lhs.frame_offset, lhs_var->index);
        state.binding[key] = rhs;
        return true;
    }
    if (rhs_var) {
        const uint32_t key = gkey(globalize, rhs.frame_offset, rhs_var->index);
        state.binding[key] = rhs;
        return true;
    }

    const expr::functor& lf = std::get<expr::functor>(lhs.skeleton->content);
    const expr::functor& rf = std::get<expr::functor>(rhs.skeleton->content);
    if (lf.id != rf.id || lf.args.size() != rf.args.size())
        return false;

    for (size_t i = 0; i < lf.args.size(); ++i) {
        if (!naive_unify(state, fe(lf.args[i], lhs.frame_offset), fe(rf.args[i], rhs.frame_offset), globalize))
            return false;
    }
    return true;
}

} // namespace

struct UbsUnifierBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    integration_stack stack;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr var4{expr::var{4}};
    expr var5{expr::var{5}};
    expr var6{expr::var{6}};

    expr f_inner0{expr::functor{functors.id("f"), {&var0}}};
    expr f_inner2{expr::functor{functors.id("f"), {&var2}}};
    expr lhs_nested{expr::functor{functors.id("g"), {&f_inner0, &var1}}};
    expr rhs_nested{expr::functor{functors.id("g"), {&f_inner2, &var3}}};

    expr atom_h{expr::functor{functors.id("h"), {&var5}}};
    expr atom_a{expr::functor{functors.id("a"), {}}};
    expr atom_b{expr::functor{functors.id("b"), {}}};
    expr f_atom{expr::functor{functors.id("a"), {}}};
    expr g_atom{expr::functor{functors.id("b"), {}}};

    expr f_dup{expr::functor{functors.id("pair"), {&var6, &var6}}};
    expr f_distinct{expr::functor{functors.id("pair"), {&atom_a, &atom_b}}};
    expr f_aa{expr::functor{functors.id("pair"), {&atom_a, &atom_a}}};

    expr atom_x{expr::functor{functors.id("x"), {}}};
    expr atom_y{expr::functor{functors.id("y"), {}}};
    expr f_xa{expr::functor{functors.id("p"), {&var0, &atom_x}}};
    expr f_by{expr::functor{functors.id("p"), {&atom_b, &var1}}};
    expr f_bc{expr::functor{functors.id("p"), {&atom_b, &atom_y}}};

    expr self_var{expr::var{0}};
    expr self_wrap{expr::functor{functors.id("loop"), {&self_var}}};
};

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyNestedFunctorsBindsVariables) {
    EXPECT_TRUE(stack.unifier.unify(fe(&lhs_nested, 0), fe(&rhs_nested, 0)));
    EXPECT_EQ(
        ubs_rep_index(resolve_local(stack, 0, 0u)),
        ubs_rep_index(resolve_local(stack, 0, 2u)));
    EXPECT_EQ(
        ubs_rep_index(resolve_local(stack, 0, 1u)),
        ubs_rep_index(resolve_local(stack, 0, 3u)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyIncompatibleFunctorsLeavesStoreEmpty) {
    EXPECT_FALSE(stack.unifier.unify(fe(&f_atom, 0), fe(&g_atom, 0)));
    EXPECT_TRUE(is_var_rep(resolve_local(stack, 0, 0u), 0u));
    EXPECT_EQ(stack.backing.parents.map.size(), 0u);
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyVarWithFunctorStoresBindingAtRoot) {
    EXPECT_TRUE(stack.unifier.unify(fe(&var4, 0), fe(&atom_h, 0)));
    const framed_expr resolved = resolve_local(stack, 0, 4u);
    const auto& bound = std::get<expr::functor>(resolved.skeleton->content);
    EXPECT_EQ(bound.id, functors.id("h"));
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyRepeatedVarFailsWhenSecondArgConflicts) {
    EXPECT_FALSE(stack.unifier.unify(fe(&f_dup, 0), fe(&f_distinct, 0)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyRepeatedVarSucceedsWhenArgsAgree) {
    EXPECT_TRUE(stack.unifier.unify(fe(&f_dup, 0), fe(&f_aa, 0)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyCrossArgsBindsBothToSharedAtom) {
    expr f_xy{expr::functor{functors.id("q"), {&var0, &var1}}};
    expr f_y_atom{expr::functor{functors.id("q"), {&var1, &atom_a}}};
    EXPECT_TRUE(stack.unifier.unify(fe(&f_xy, 0), fe(&f_y_atom, 0)));
    EXPECT_TRUE(std::holds_alternative<expr::functor>(resolve_local(stack, 0, 0u).skeleton->content));
    EXPECT_TRUE(std::holds_alternative<expr::functor>(resolve_local(stack, 0, 1u).skeleton->content));
}

TEST_F(UbsUnifierBindMapIntegrationTest, SecondUnifyRespectsExistingBinding) {
    EXPECT_TRUE(stack.unifier.unify(fe(&var2, 0), fe(&atom_a, 0)));
    EXPECT_FALSE(stack.unifier.unify(fe(&var2, 0), fe(&atom_b, 0)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, BoundVarsInSameClassUnifyWithoutReEquate) {
    stack.store.equate(gkey(stack.globalize, 0, 0), gkey(stack.globalize, 0, 1));
    stack.store.instantiate(gkey(stack.globalize, 0, 0), fe(&atom_a, 0));
    EXPECT_TRUE(stack.unifier.unify(fe(&var0, 0), fe(&var1, 0)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, GlobalizerUsesFramePlusLocalAsGlobalKey) {
    EXPECT_EQ(gkey(stack.globalize, 0, 4), gkey(stack.globalize, 1, 3));
    EXPECT_NE(gkey(stack.globalize, 0, 10), gkey(stack.globalize, 0, 11));
    EXPECT_TRUE(stack.unifier.unify(fe(&var0, 0), fe(&var1, 1)));
    EXPECT_EQ(ubs_rep_index(resolve_local(stack, 0, 0u)), ubs_rep_index(resolve_local(stack, 1, 1u)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, FunctorArgsUseCalleeFrameOffset) {
    expr inner_v{expr::var{0}};
    expr inner_l{expr::functor{functors.id("inner"), {&inner_v}}};
    expr inner_r{expr::functor{functors.id("inner"), {&var2}}};
    expr outer_l{expr::functor{functors.id("wrap"), {&inner_l}}};
    expr outer_r{expr::functor{functors.id("wrap"), {&inner_r}}};
    EXPECT_TRUE(stack.unifier.unify(fe(&outer_l, 5), fe(&outer_r, 0)));
    const uint32_t callee_key = gkey(stack.globalize, 5, 0);
    const framed_expr resolved = stack.store.resolve(callee_key);
    EXPECT_EQ(ubs_rep_index(resolved), ubs_rep_index(resolve_local(stack, 0, 2u)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, CalleeVarUnifiedWithCallerVarSharesClass) {
    expr callee_v{expr::var{0}};
    expr caller_v{expr::var{3}};
    EXPECT_TRUE(stack.unifier.unify(fe(&callee_v, 4), fe(&caller_v, 0)));
    EXPECT_EQ(
        ubs_rep_index(resolve_local(stack, 4, 0)),
        ubs_rep_index(resolve_local(stack, 0, 3u)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, MixedFrameClassResolvesToSmallestGlobalRep) {
    const uint32_t key_low = gkey(stack.globalize, 0, 20);
    const uint32_t key_high = gkey(stack.globalize, 4, 17);
    EXPECT_LT(key_low, key_high);
    stack.store.equate(key_low, key_high);
    EXPECT_EQ(ubs_rep_index(stack.store.resolve(key_high)), key_low);
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifyIsSymmetric) {
    integration_stack forward;
    integration_stack backward;
    EXPECT_EQ(
        forward.unifier.unify(fe(&f_xa, 0), fe(&f_bc, 0)),
        backward.unifier.unify(fe(&f_bc, 0), fe(&f_xa, 0)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, UnifySamePairTwiceIsIdempotent) {
    const ubs_map_snapshot before_maps = snapshot_ubs_maps(stack.backing);
    EXPECT_TRUE(stack.unifier.unify(fe(&var3, 0), fe(&var4, 0)));
    const ubs_map_snapshot after_first = snapshot_ubs_maps(stack.backing);
    EXPECT_FALSE(ubs_snapshots_equal(before_maps, after_first));
    EXPECT_TRUE(stack.unifier.unify(fe(&var3, 0), fe(&var4, 0)));
    EXPECT_TRUE(ubs_snapshots_equal(after_first, snapshot_ubs_maps(stack.backing)));
}

TEST_F(UbsUnifierBindMapIntegrationTest, SuccessfulUnifyMakesResolvedTermsStructurallyEqual) {
    expr a{expr::functor{functors.id("t"), {}}};
    expr left{expr::functor{functors.id("pair"), {&var0, &a}}};
    expr right{expr::functor{functors.id("pair"), {&a, &var1}}};
    EXPECT_TRUE(stack.unifier.unify(fe(&left, 0), fe(&right, 0)));
    const framed_expr res0 = resolve_local(stack, 0, 0u);
    const framed_expr res1 = resolve_local(stack, 0, 1u);
    EXPECT_EQ(std::get<expr::functor>(res0.skeleton->content).id, functors.id("t"));
    EXPECT_EQ(std::get<expr::functor>(res1.skeleton->content).id, functors.id("t"));
}

TEST_F(UbsUnifierBindMapIntegrationTest, NaiveReferenceAgreesOnRandomSmallTerms) {
    std::mt19937 rng(99);
    for (int trial = 0; trial < 30; ++trial) {
        integration_stack prod;
        naive_state naive;
        expr v0{expr::var{0}};
        expr v1{expr::var{1}};
        expr atoms[2] = {
            expr{expr::functor{functors.id("r"), {}}},
            expr{expr::functor{functors.id("s"), {}}},
        };
        const int choice = static_cast<int>(rng() % 4);
        framed_expr left = fe(&v0, 0);
        framed_expr right = fe(&v1, 0);
        if (choice == 0)
            right = fe(&atoms[0], 0);
        if (choice == 1)
            left = fe(&atoms[0], 0);
        if (choice == 2) {
            left = fe(&atoms[0], 0);
            right = fe(&atoms[1], 0);
        }
        const bool prod_ok = prod.unifier.unify(left, right);
        const bool naive_ok = naive_unify(naive, left, right, prod.globalize);
        EXPECT_EQ(prod_ok, naive_ok);
    }
}

TEST_F(UbsUnifierBindMapIntegrationTest, KnownLimitationCyclicBindingWithoutOccursCheck) {
    EXPECT_TRUE(stack.unifier.unify(fe(&var0, 0), fe(&self_wrap, 0)));
    const framed_expr resolved = resolve_local(stack, 0, 0u);
    EXPECT_TRUE(std::holds_alternative<expr::functor>(resolved.skeleton->content));
}

TEST_F(UbsUnifierBindMapIntegrationTest, BoundFunctorWithInnerVarsUnifiesLater) {
    expr inner{expr::var{0}};
    expr wrapper{expr::functor{functors.id("wrap"), {&inner}}};
    EXPECT_TRUE(stack.unifier.unify(fe(&var2, 0), fe(&wrapper, 0)));
    expr inner_atom{expr::functor{functors.id("leaf"), {}}};
    expr target{expr::functor{functors.id("wrap"), {&inner_atom}}};
    EXPECT_TRUE(stack.unifier.unify(fe(&var2, 0), fe(&target, 0)));
    EXPECT_EQ(
        std::get<expr::functor>(resolve_local(stack, 0, 2u).skeleton->content).id,
        functors.id("wrap"));
    EXPECT_EQ(
        std::get<expr::functor>(resolve_local(stack, 0, 0u).skeleton->content).id,
        functors.id("leaf"));
}

TEST_F(UbsUnifierBindMapIntegrationTest, KnownLimitationFailedUnifyLeavesPartialBindings) {
    EXPECT_FALSE(stack.unifier.unify(fe(&f_xa, 0), fe(&f_bc, 0)));
    EXPECT_TRUE(std::holds_alternative<expr::functor>(resolve_local(stack, 0, 0u).skeleton->content));
    EXPECT_EQ(
        std::get<expr::functor>(resolve_local(stack, 0, 0u).skeleton->content).id,
        functors.id("b"));
}
