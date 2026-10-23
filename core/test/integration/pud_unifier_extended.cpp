#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"

namespace {

uint32_t gkey(globalizer& globalize, uint32_t frame, uint32_t local) {
    return globalize.globalize(frame, local);
}

}

struct PudUnifierExtendedIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;

    const expr* v(uint32_t local) { return stack.pool.make_var(local); }
    const expr* atom(const char* name) {
        return stack.pool.make_functor(functors.id(name), {});
    }
};

TEST_F(PudUnifierExtendedIntegrationTest, CrossArgMirrorBindsIdentically) {
    const expr* f_xy = stack.pool.make_functor(functors.id("p"), {v(0), atom("a")});
    const expr* f_by = stack.pool.make_functor(functors.id("p"), {atom("b"), v(1)});
    pud_term_stack mirrored;
    const expr* mirror_xy = mirrored.pool.make_functor(
        functors.id("p"),
        {mirrored.pool.make_var(0u), mirrored.pool.make_functor(functors.id("a"), {})});
    const expr* mirror_by = mirrored.pool.make_functor(
        functors.id("p"),
        {mirrored.pool.make_functor(functors.id("b"), {}), mirrored.pool.make_var(1u)});
    EXPECT_TRUE(stack.unifier.unify(pud_fe(f_xy, 0), pud_fe(f_by, 0)));
    EXPECT_TRUE(mirrored.unifier.unify(pud_fe(mirror_by, 0), pud_fe(mirror_xy, 0)));
    EXPECT_EQ(stack.resolve(0u), atom("b"));
    EXPECT_EQ(stack.resolve(1u), atom("a"));
    EXPECT_EQ(mirrored.resolve(0u), mirrored.pool.make_functor(functors.id("b"), {}));
    EXPECT_EQ(mirrored.resolve(1u), mirrored.pool.make_functor(functors.id("a"), {}));
}

TEST_F(PudUnifierExtendedIntegrationTest, SwappingFramesPreservesSuccessAndBindings) {
    const expr* local0 = v(0);
    const expr* local3 = v(3);
    pud_term_stack swapped;
    const bool ok_a = stack.unifier.unify(pud_fe(local0, 4), pud_fe(local3, 0));
    const bool ok_b = swapped.unifier.unify(pud_fe(local3, 0), pud_fe(local0, 4));
    EXPECT_EQ(ok_a, ok_b);
    const uint32_t stack_global = gkey(stack.globalize, 4, 0);
    const uint32_t swapped_global = gkey(swapped.globalize, 4, 0);
    EXPECT_EQ(
        std::get<expr::var>(stack.resolve(stack_global)->content).index,
        std::get<expr::var>(swapped.resolve(swapped_global)->content).index);
}

TEST_F(PudUnifierExtendedIntegrationTest, FunctorMismatchPerformsZeroWrites) {
    const pud_ubs_state before = stack.snapshot();
    const expr* fa = atom("a");
    const expr* fb = atom("b");
    EXPECT_FALSE(stack.unifier.unify(pud_fe(fa, 0), pud_fe(fb, 0)));
    EXPECT_TRUE(before == stack.snapshot());
}

TEST_F(PudUnifierExtendedIntegrationTest, ArityMismatchPerformsZeroWrites) {
    const pud_ubs_state before = stack.snapshot();
    const uint32_t pid = functors.id("p");
    const expr* unary = stack.pool.make_functor(pid, {v(0)});
    const expr* binary = stack.pool.make_functor(pid, {v(0), v(1)});
    EXPECT_FALSE(stack.unifier.unify(pud_fe(unary, 0), pud_fe(binary, 0)));
    EXPECT_TRUE(before == stack.snapshot());
}

TEST_F(PudUnifierExtendedIntegrationTest, LastArgumentMismatchAfterPartialProcessing) {
    const expr* fxa = stack.pool.make_functor(functors.id("p"), {v(0), atom("a")});
    const expr* fxb = stack.pool.make_functor(functors.id("p"), {atom("b"), v(0)});
    EXPECT_FALSE(stack.unifier.unify(pud_fe(fxa, 0), pud_fe(fxb, 0)));
    EXPECT_EQ(stack.resolve(0u), atom("b"));
}

TEST_F(PudUnifierExtendedIntegrationTest, DeepIdenticalStructureDepthFiftySucceeds) {
    const uint32_t fid = functors.id("nest");
    const expr* leaf = atom("z");
    const expr* deep = leaf;
    for (int depth = 0; depth < 50; ++depth)
        deep = stack.pool.make_functor(fid, {deep});
    pud_term_stack other;
    const expr* other_deep = leaf;
    for (int depth = 0; depth < 50; ++depth)
        other_deep = other.pool.make_functor(fid, {other_deep});
    EXPECT_TRUE(stack.unifier.unify(pud_fe(deep, 0), pud_fe(other_deep, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, WideArityHundredLastMismatchFails) {
    const uint32_t pid = functors.id("wide");
    std::vector<const expr*> left_args;
    std::vector<const expr*> right_args;
    for (int arg = 0; arg < 100; ++arg) {
        left_args.push_back(v(static_cast<uint32_t>(arg)));
        right_args.push_back(v(static_cast<uint32_t>(arg)));
    }
    right_args.back() = atom("b");
    left_args.back() = atom("a");
    const expr* left = stack.pool.make_functor(pid, left_args);
    const expr* right = stack.pool.make_functor(pid, right_args);
    EXPECT_FALSE(stack.unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarStructurallyEqualDifferentFramingSucceeds) {
    pud_term_stack s;
    const expr* a0 = s.pool.make_functor(functors.id("a"), {});
    const expr* wrapped = s.pool.make_functor(functors.id("w"), {a0});
    s.instantiate(0u, framed_expr{s.pool.make_functor(functors.id("w"), {a0}), 0u});
    const expr* wrap_rhs = s.pool.make_functor(functors.id("w"), {s.pool.make_var(1u)});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 0), pud_fe(wrap_rhs, 0)));
    EXPECT_EQ(s.resolve(1u), a0);
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarWithDifferentBoundFunctorFails) {
    pud_term_stack s;
    s.instantiate(0u, framed_expr{s.pool.make_functor(functors.id("a"), {}), 0u});
    s.instantiate(1u, framed_expr{s.pool.make_functor(functors.id("b"), {}), 0u});
    EXPECT_FALSE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 0),
        pud_fe(s.pool.make_var(1u), 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarDiffersOnlyInNestedVarBindsNested) {
    pud_term_stack s;
    const expr* a_atom = s.pool.make_functor(functors.id("a"), {});
    s.instantiate(0u, framed_expr{s.pool.make_functor(functors.id("w"), {a_atom}), 0u});
    const expr* rhs = s.pool.make_functor(functors.id("w"), {s.pool.make_var(1u)});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 0), pud_fe(rhs, 0)));
    EXPECT_EQ(s.resolve(1u), a_atom);
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarWithUnboundVarBindsUnboundClass) {
    pud_term_stack s;
    const expr* ground = s.pool.make_functor(functors.id("g"), {});
    s.instantiate(0u, framed_expr{s.pool.make_functor(functors.id("g"), {}), 0u});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 0),
        pud_fe(s.pool.make_var(1u), 0)));
    EXPECT_EQ(s.resolve(1u), ground);
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursIndirectSecondUnifyFails) {
    pud_term_stack s;
    const expr* f_yz = s.pool.make_functor(
        functors.id("f"), {s.pool.make_var(1u), s.pool.make_var(2u)});
    const expr* g_x = s.pool.make_functor(functors.id("g"), {s.pool.make_var(0u)});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 0), pud_fe(f_yz, 0)));
    EXPECT_FALSE(s.unifier.unify(
        pud_fe(s.pool.make_var(2u), 0), pud_fe(g_x, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursThreeMemberClassCycleFails) {
    pud_term_stack s;
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(v(1), 0)));
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(1), 0), pud_fe(v(2), 0)));
    const expr* fx = stack.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_FALSE(s.unifier.unify(pud_fe(v(2), 0), pud_fe(fx, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursThroughBindingPlusMergeFails) {
    pud_term_stack s;
    const expr* fy = stack.pool.make_functor(functors.id("f"), {v(1)});
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(fy, 0)));
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(1), 0), pud_fe(v(2), 0)));
    const expr* gx = stack.pool.make_functor(functors.id("g"), {v(0)});
    EXPECT_FALSE(s.unifier.unify(pud_fe(v(2), 0), pud_fe(gx, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursExpandedTermFails) {
    pud_term_stack s;
    const expr* fa_y = stack.pool.make_functor(functors.id("p"), {atom("a"), v(1)});
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(fa_y, 0)));
    const expr* inner = stack.pool.make_functor(functors.id("p"), {atom("a"), v(1)});
    const expr* expanded = stack.pool.make_functor(functors.id("p"), {atom("a"), inner});
    EXPECT_FALSE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(expanded, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursFailsWhereTheSameVarAloneWouldBind) {
    pud_term_stack s;
    const expr* loop = stack.pool.make_functor(functors.id("l"), {v(0)});
    EXPECT_FALSE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(loop, 0)));
    pud_term_stack fresh;
    EXPECT_TRUE(fresh.unifier.unify(pud_fe(v(0), 0), pud_fe(atom("a"), 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, CallerCannotBindTermWithCalleeInSameClass) {
    pud_term_stack s;
    const uint32_t caller = gkey(s.globalize, 2u, 0u);
    const uint32_t callee = gkey(s.globalize, 0u, 1u);
    EXPECT_TRUE(s.unifier.unify(pud_fe(s.pool.make_var(0u), 2), pud_fe(s.pool.make_var(1u), 0)));
    const expr* bad = s.pool.make_functor(functors.id("w"), {s.pool.make_var(1u)});
    EXPECT_FALSE(s.unifier.unify(pud_fe(s.pool.make_var(0u), 2), pud_fe(bad, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, DifferentClassVariablePassesOccurs) {
    pud_term_stack s;
    const expr* ok = stack.pool.make_functor(functors.id("w"), {v(1)});
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(ok, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, SameSkeletonTwoOffsetsEquatesGlobals) {
    pud_term_stack s;
    const expr* fx = stack.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_TRUE(s.unifier.unify(pud_fe(fx, 0), pud_fe(fx, 10)));
    EXPECT_EQ(
        s.resolve(gkey(s.globalize, 0, 0)),
        s.resolve(gkey(s.globalize, 10, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, FramedAgainstStoredOffsetZeroUsesCorrectFrames) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* fx = s.pool.make_functor(functors.id("w"), {y});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 5), pud_fe(fx, 0)));
    const uint32_t bound_global = s.globalize.globalize(5u, 0u);
    const expr* stored = s.resolve(bound_global);
    EXPECT_TRUE(std::holds_alternative<expr::functor>(stored->content));
}

TEST_F(PudUnifierExtendedIntegrationTest, ThreeFramesMapToDistinctGlobals) {
    pud_term_stack s;
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 0), pud_fe(v(0), 1)));
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 2), pud_fe(v(0), 3)));
    const uint32_t g0 = gkey(s.globalize, 0, 0);
    const uint32_t g1 = gkey(s.globalize, 1, 0);
    const uint32_t g2 = gkey(s.globalize, 2, 0);
    const uint32_t g3 = gkey(s.globalize, 3, 0);
    EXPECT_NE(g0, g1);
    EXPECT_NE(g2, g3);
    EXPECT_EQ(s.resolve(g0), s.resolve(g1));
    EXPECT_EQ(s.resolve(g2), s.resolve(g3));
}

TEST_F(PudUnifierExtendedIntegrationTest, FreshVarToGroundBindsOnlyThatVar) {
    pud_term_stack s;
    const expr* a = s.pool.make_functor(functors.id("a"), {});
    EXPECT_TRUE(s.unifier.unify(pud_fe(s.pool.make_var(0u), 0), pud_fe(a, 0)));
    EXPECT_EQ(s.resolve(0u), a);
    EXPECT_EQ(s.bind_map.find(1u), (ubs_class{.root = 1, .size = 1, .rep = 1, .value = std::nullopt}));
}

TEST_F(PudUnifierExtendedIntegrationTest, PairwiseDistinctFreshVarsMergeIntoOneClass) {
    pud_term_stack s;
    for (uint32_t var = 1u; var < 6u; ++var)
        EXPECT_TRUE(s.unifier.unify(pud_fe(s.pool.make_var(0u), 0), pud_fe(s.pool.make_var(var), 0)));
    for (uint32_t var = 0u; var < 6u; ++var) {
        EXPECT_EQ(s.bind_map.find(var).rep, 0u);
        EXPECT_EQ(s.bind_map.find(var).size, 6u);
    }
}

TEST_F(PudUnifierExtendedIntegrationTest, RuleApplicationThenNormalizeGoal) {
    pud_term_stack s;
    const uint32_t f = functors.id("head");
    const expr* rule_head = s.pool.make_functor(
        f, {s.pool.make_var(0u), s.pool.make_functor(functors.id("a"), {})});
    const expr* goal = s.pool.make_functor(
        f, {s.pool.make_functor(functors.id("b"), {}), s.pool.make_var(0u)});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(goal, 4), pud_fe(rule_head, 0)));
    const uint32_t goal_var = s.globalize.globalize(4u, 0u);
    EXPECT_EQ(s.normalizer.normalize(pud_fe(s.pool.make_var(0u), 4)),
        s.pool.make_functor(functors.id("a"), {}));
    EXPECT_EQ(s.resolve(goal_var),
        s.pool.make_functor(functors.id("a"), {}));
}

TEST_F(PudUnifierExtendedIntegrationTest, TwoOffsetsDoNotInterfere) {
    pud_term_stack s;
    const expr* atom_x = s.pool.make_functor(functors.id("x"), {});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 10),
        pud_fe(s.pool.make_var(0u), 0)));
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 20), pud_fe(atom_x, 0)));
    const uint32_t global_10 = gkey(s.globalize, 10, 0);
    const uint32_t global_20 = gkey(s.globalize, 20, 0);
    const expr* rep_10 = s.resolve(global_10);
    const expr* rep_0 = s.resolve(0u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep_10->content));
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep_0->content));
    EXPECT_EQ(
        std::get<expr::var>(rep_10->content).index,
        std::get<expr::var>(rep_0->content).index);
    EXPECT_EQ(s.resolve(global_20), atom_x);
}

TEST_F(PudUnifierExtendedIntegrationTest, CalleeUnboundVersusMergedWithCaller) {
    pud_term_stack s;
    const uint32_t caller = gkey(s.globalize, 1, 0);
    const uint32_t callee = gkey(s.globalize, 0, 2);
    EXPECT_TRUE(s.unifier.unify(pud_fe(v(0), 1), pud_fe(v(2), 0)));
    EXPECT_EQ(s.normalizer.normalize(pud_fe(v(2), 0)), s.pool.make_var(caller));
    pud_term_stack lone;
    EXPECT_EQ(
        lone.normalizer.normalize(pud_fe(lone.pool.make_var(2u), 0)),
        lone.pool.make_var(2u));
}

TEST_F(PudUnifierExtendedIntegrationTest, AppendStyleListAtIncreasingOffsets) {
    pud_term_stack s;
    const uint32_t cons = functors.id("cons");
    const uint32_t nil = functors.id("nil");
    const expr* nil_node = s.pool.make_functor(nil, {});
    const expr* atom_a = s.pool.make_functor(functors.id("a"), {});
    const expr* atom_b = s.pool.make_functor(functors.id("b"), {});
    const expr* cell_a = s.pool.make_functor(cons, {atom_a, nil_node});
    const expr* cell_b = s.pool.make_functor(cons, {atom_b, nil_node});
    const expr* list_ab = s.pool.make_functor(cons, {atom_a, cell_b});
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 0), pud_fe(cell_a, 0)));
    EXPECT_TRUE(s.unifier.unify(
        pud_fe(s.pool.make_var(0u), 8), pud_fe(list_ab, 4)));
    EXPECT_EQ(
        s.normalizer.normalize(pud_fe(s.pool.make_var(0u), 0)),
        cell_a);
    EXPECT_EQ(
        s.normalizer.normalize(pud_fe(s.pool.make_var(0u), 8)),
        list_ab);
}

TEST_F(PudUnifierExtendedIntegrationTest, PeanoPlusDerivesExpectedResult) {
    pud_term_stack s;
    const uint32_t z_id = functors.id("z");
    const uint32_t s_id = functors.id("s");
    const uint32_t plus_id = functors.id("plus");
    const expr* z = s.pool.make_functor(z_id, {});
    const expr* sz = s.pool.make_functor(s_id, {z});
    const expr* ssz = s.pool.make_functor(s_id, {s.pool.make_functor(s_id, {z})});
    const expr* sssz = s.pool.make_functor(
        s_id, {s.pool.make_functor(s_id, {s.pool.make_functor(s_id, {z})})});
    const expr* goal = s.pool.make_functor(
        plus_id, {ssz, sz, s.pool.make_var(10u)});
    const expr* rule_step = s.pool.make_functor(
        plus_id,
        {s.pool.make_functor(s_id, {s.pool.make_var(1u)}),
         s.pool.make_var(2u),
         s.pool.make_functor(s_id, {s.pool.make_var(3u)})});
    const expr* rule_base = s.pool.make_functor(
        plus_id, {z, s.pool.make_var(4u), s.pool.make_var(4u)});

    EXPECT_TRUE(s.unifier.unify(pud_fe(goal, 0), pud_fe(rule_step, 0)));
    const expr* subgoal = s.pool.make_functor(plus_id, {sz, sz, s.pool.make_var(10u)});
    EXPECT_TRUE(s.unifier.unify(pud_fe(subgoal, 0), pud_fe(rule_step, 20)));
    const expr* subgoal2 = s.pool.make_functor(plus_id, {z, sz, s.pool.make_var(10u)});
    EXPECT_TRUE(s.unifier.unify(pud_fe(subgoal2, 0), pud_fe(rule_base, 40)));
    const expr* derived = s.normalizer.normalize(pud_fe(s.pool.make_var(10u), 0));
    EXPECT_TRUE(std::holds_alternative<expr::functor>(derived->content));
    const expr::functor& outer = std::get<expr::functor>(derived->content);
    EXPECT_EQ(outer.id, s_id);
    EXPECT_EQ(outer.args[0], z);
}
