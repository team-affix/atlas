#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_unifier_bind_map_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

uint32_t gkey(globalizer& globalize, uint32_t frame, uint32_t local) {
    return globalize.globalize(frame, local);
}

} // namespace

struct PudUnifierExtendedIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_unifier_bind_map_stack stack;

    const expr* v(uint32_t local) { return stack.bind_backing.pool.make_var(local); }
    const expr* atom(const char* name) {
        return stack.bind_backing.pool.make_functor(functors.id(name), {});
    }
};

TEST_F(PudUnifierExtendedIntegrationTest, CrossArgMirrorBindsIdentically) {
    const expr* f_xy = stack.bind_backing.pool.make_functor(functors.id("p"), {v(0), atom("a")});
    const expr* f_by = stack.bind_backing.pool.make_functor(functors.id("p"), {atom("b"), v(1)});
    pud_unifier_bind_map_stack mirrored;
    const expr* mirror_xy = mirrored.bind_backing.pool.make_functor(
        functors.id("p"),
        {mirrored.bind_backing.pool.make_var(0u), mirrored.bind_backing.pool.make_functor(functors.id("a"), {})});
    const expr* mirror_by = mirrored.bind_backing.pool.make_functor(
        functors.id("p"),
        {mirrored.bind_backing.pool.make_functor(functors.id("b"), {}), mirrored.bind_backing.pool.make_var(1u)});
    EXPECT_TRUE(stack.framed_unifier.unify(pud_fe(f_xy, 0), pud_fe(f_by, 0)));
    EXPECT_TRUE(mirrored.framed_unifier.unify(pud_fe(mirror_by, 0), pud_fe(mirror_xy, 0)));
    EXPECT_EQ(stack.bind_map.resolve(0u), atom("b"));
    EXPECT_EQ(stack.bind_map.resolve(1u), atom("a"));
    EXPECT_EQ(mirrored.bind_map.resolve(0u), mirrored.bind_backing.pool.make_functor(functors.id("b"), {}));
    EXPECT_EQ(mirrored.bind_map.resolve(1u), mirrored.bind_backing.pool.make_functor(functors.id("a"), {}));
}

TEST_F(PudUnifierExtendedIntegrationTest, SwappingFramesPreservesSuccessAndBindings) {
    const expr* local0 = v(0);
    const expr* local3 = v(3);
    pud_unifier_bind_map_stack swapped;
    const bool ok_a = stack.framed_unifier.unify(pud_fe(local0, 4), pud_fe(local3, 0));
    const bool ok_b = swapped.framed_unifier.unify(pud_fe(local3, 0), pud_fe(local0, 4));
    EXPECT_EQ(ok_a, ok_b);
    const uint32_t stack_global = gkey(stack.globalize, 4, 0);
    const uint32_t swapped_global = gkey(swapped.globalize, 4, 0);
    EXPECT_EQ(
        std::get<expr::var>(stack.bind_map.resolve(stack_global)->content).index,
        std::get<expr::var>(swapped.bind_map.resolve(swapped_global)->content).index);
}

TEST_F(PudUnifierExtendedIntegrationTest, FunctorMismatchPerformsZeroWrites) {
    const ubs_map_snapshot before = snapshot_bind_backing(stack.bind_backing);
    const expr* fa = atom("a");
    const expr* fb = atom("b");
    EXPECT_FALSE(stack.framed_unifier.unify(pud_fe(fa, 0), pud_fe(fb, 0)));
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot_bind_backing(stack.bind_backing)));
}

TEST_F(PudUnifierExtendedIntegrationTest, ArityMismatchPerformsZeroWrites) {
    const ubs_map_snapshot before = snapshot_bind_backing(stack.bind_backing);
    const uint32_t pid = functors.id("p");
    const expr* unary = stack.bind_backing.pool.make_functor(pid, {v(0)});
    const expr* binary = stack.bind_backing.pool.make_functor(pid, {v(0), v(1)});
    EXPECT_FALSE(stack.framed_unifier.unify(pud_fe(unary, 0), pud_fe(binary, 0)));
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot_bind_backing(stack.bind_backing)));
}

TEST_F(PudUnifierExtendedIntegrationTest, LastArgumentMismatchAfterPartialProcessing) {
    const expr* fxa = stack.bind_backing.pool.make_functor(functors.id("p"), {v(0), atom("a")});
    const expr* fxb = stack.bind_backing.pool.make_functor(functors.id("p"), {atom("b"), v(0)});
    EXPECT_FALSE(stack.framed_unifier.unify(pud_fe(fxa, 0), pud_fe(fxb, 0)));
    EXPECT_EQ(stack.bind_map.resolve(0u), atom("b"));
}

TEST_F(PudUnifierExtendedIntegrationTest, DeepIdenticalStructureDepthFiftySucceeds) {
    const uint32_t fid = functors.id("nest");
    const expr* leaf = atom("z");
    const expr* deep = leaf;
    for (int depth = 0; depth < 50; ++depth)
        deep = stack.bind_backing.pool.make_functor(fid, {deep});
    pud_unifier_bind_map_stack other;
    const expr* other_deep = leaf;
    for (int depth = 0; depth < 50; ++depth)
        other_deep = other.bind_backing.pool.make_functor(fid, {other_deep});
    EXPECT_TRUE(stack.framed_unifier.unify(pud_fe(deep, 0), pud_fe(other_deep, 0)));
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
    const expr* left = stack.bind_backing.pool.make_functor(pid, left_args);
    const expr* right = stack.bind_backing.pool.make_functor(pid, right_args);
    EXPECT_FALSE(stack.framed_unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarStructurallyEqualDifferentFramingSucceeds) {
    pud_unifier_bind_map_stack s;
    const expr* a0 = s.bind_backing.pool.make_functor(functors.id("a"), {});
    const expr* wrapped = s.bind_backing.pool.make_functor(functors.id("w"), {a0});
    s.bind_map.instantiate(0u, functors.id("w"), {a0});
    const expr* wrap_rhs = s.bind_backing.pool.make_functor(functors.id("w"), {s.bind_backing.pool.make_var(1u)});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 0), pud_fe(wrap_rhs, 0)));
    EXPECT_EQ(s.bind_map.resolve(1u), a0);
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarWithDifferentBoundFunctorFails) {
    pud_unifier_bind_map_stack s;
    s.bind_map.instantiate(0u, functors.id("a"), {});
    s.bind_map.instantiate(1u, functors.id("b"), {});
    EXPECT_FALSE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 0),
        pud_fe(s.bind_backing.pool.make_var(1u), 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarDiffersOnlyInNestedVarBindsNested) {
    pud_unifier_bind_map_stack s;
    const expr* a_atom = s.bind_backing.pool.make_functor(functors.id("a"), {});
    s.bind_map.instantiate(0u, functors.id("w"), {a_atom});
    const expr* rhs = s.bind_backing.pool.make_functor(functors.id("w"), {s.bind_backing.pool.make_var(1u)});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 0), pud_fe(rhs, 0)));
    EXPECT_EQ(s.bind_map.resolve(1u), a_atom);
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarWithUnboundVarBindsUnboundClass) {
    pud_unifier_bind_map_stack s;
    const expr* ground = s.bind_backing.pool.make_functor(functors.id("g"), {});
    s.bind_map.instantiate(0u, functors.id("g"), {});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 0),
        pud_fe(s.bind_backing.pool.make_var(1u), 0)));
    EXPECT_EQ(s.bind_map.resolve(1u), ground);
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursIndirectSecondUnifyFails) {
    pud_unifier_bind_map_stack s;
    const expr* f_yz = s.bind_backing.pool.make_functor(
        functors.id("f"), {s.bind_backing.pool.make_var(1u), s.bind_backing.pool.make_var(2u)});
    const expr* g_x = s.bind_backing.pool.make_functor(functors.id("g"), {s.bind_backing.pool.make_var(0u)});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 0), pud_fe(f_yz, 0)));
    EXPECT_FALSE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(2u), 0), pud_fe(g_x, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursThreeMemberClassCycleFails) {
    pud_unifier_bind_map_stack s;
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(v(1), 0)));
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(1), 0), pud_fe(v(2), 0)));
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_FALSE(s.framed_unifier.unify(pud_fe(v(2), 0), pud_fe(fx, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursThroughBindingPlusMergeFails) {
    pud_unifier_bind_map_stack s;
    const expr* fy = stack.bind_backing.pool.make_functor(functors.id("f"), {v(1)});
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(fy, 0)));
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(1), 0), pud_fe(v(2), 0)));
    const expr* gx = stack.bind_backing.pool.make_functor(functors.id("g"), {v(0)});
    EXPECT_FALSE(s.framed_unifier.unify(pud_fe(v(2), 0), pud_fe(gx, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursExpandedTermFails) {
    pud_unifier_bind_map_stack s;
    const expr* fa_y = stack.bind_backing.pool.make_functor(functors.id("p"), {atom("a"), v(1)});
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(fa_y, 0)));
    const expr* inner = stack.bind_backing.pool.make_functor(functors.id("p"), {atom("a"), v(1)});
    const expr* expanded = stack.bind_backing.pool.make_functor(functors.id("p"), {atom("a"), inner});
    EXPECT_FALSE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(expanded, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, FailedOccursLeavesClassUnbound) {
    pud_unifier_bind_map_stack s;
    const expr* loop = stack.bind_backing.pool.make_functor(functors.id("l"), {v(0)});
    EXPECT_FALSE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(loop, 0)));
    const expr* resolved = s.bind_map.resolve(0u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(resolved->content));
}

TEST_F(PudUnifierExtendedIntegrationTest, AfterFailedOccursCanBindLegally) {
    pud_unifier_bind_map_stack s;
    const expr* loop = stack.bind_backing.pool.make_functor(functors.id("l"), {v(0)});
    EXPECT_FALSE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(loop, 0)));
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(atom("a"), 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, CallerCannotBindTermWithCalleeInSameClass) {
    pud_unifier_bind_map_stack s;
    const uint32_t caller = gkey(s.globalize, 2u, 0u);
    const uint32_t callee = gkey(s.globalize, 0u, 1u);
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(s.bind_backing.pool.make_var(0u), 2), pud_fe(s.bind_backing.pool.make_var(1u), 0)));
    const expr* bad = s.bind_backing.pool.make_functor(functors.id("w"), {s.bind_backing.pool.make_var(1u)});
    EXPECT_FALSE(s.framed_unifier.unify(pud_fe(s.bind_backing.pool.make_var(0u), 2), pud_fe(bad, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, DifferentClassVariablePassesOccurs) {
    pud_unifier_bind_map_stack s;
    const expr* ok = stack.bind_backing.pool.make_functor(functors.id("w"), {v(1)});
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(ok, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, SameSkeletonTwoOffsetsEquatesGlobals) {
    pud_unifier_bind_map_stack s;
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(fx, 0), pud_fe(fx, 10)));
    EXPECT_EQ(
        s.bind_map.resolve(gkey(s.globalize, 0, 0)),
        s.bind_map.resolve(gkey(s.globalize, 10, 0)));
}

TEST_F(PudUnifierExtendedIntegrationTest, FramedAgainstStoredOffsetZeroUsesCorrectFrames) {
    pud_unifier_bind_map_stack s;
    const expr* y = s.bind_backing.pool.make_var(1u);
    const expr* fx = s.bind_backing.pool.make_functor(functors.id("w"), {y});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 5), pud_fe(fx, 0)));
    const uint32_t bound_global = s.globalize.globalize(5u, 0u);
    const expr* stored = s.bind_map.resolve(bound_global);
    EXPECT_TRUE(std::holds_alternative<expr::functor>(stored->content));
}

TEST_F(PudUnifierExtendedIntegrationTest, ThreeFramesMapToDistinctGlobals) {
    pud_unifier_bind_map_stack s;
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(v(0), 1)));
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 2), pud_fe(v(0), 3)));
    const uint32_t g0 = gkey(s.globalize, 0, 0);
    const uint32_t g1 = gkey(s.globalize, 1, 0);
    const uint32_t g2 = gkey(s.globalize, 2, 0);
    const uint32_t g3 = gkey(s.globalize, 3, 0);
    EXPECT_NE(g0, g1);
    EXPECT_NE(g2, g3);
    EXPECT_EQ(s.bind_map.resolve(g0), s.bind_map.resolve(g1));
    EXPECT_EQ(s.bind_map.resolve(g2), s.bind_map.resolve(g3));
}

TEST_F(PudUnifierExtendedIntegrationTest, FreshVarToGroundPerformsOneInstantiate) {
    pud_unifier_bind_map_stack s;
    const ubs_map_snapshot before = snapshot_bind_backing(s.bind_backing);
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(atom("a"), 0)));
    const ubs_map_snapshot after = snapshot_bind_backing(s.bind_backing);
    EXPECT_EQ(after.values.size(), before.values.size() + 1u);
}

TEST_F(PudUnifierExtendedIntegrationTest, PairwiseDistinctFreshVarsPerformNMinusOneMerges) {
    pud_unifier_bind_map_stack s;
    const ubs_map_snapshot before = snapshot_bind_backing(s.bind_backing);
    for (uint32_t var = 1u; var < 6u; ++var)
        EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 0), pud_fe(v(var), 0)));
    const ubs_map_snapshot after = snapshot_bind_backing(s.bind_backing);
    EXPECT_EQ(after.parents.size(), before.parents.size() + 5u);
}

TEST_F(PudUnifierExtendedIntegrationTest, RuleApplicationThenNormalizeGoal) {
    pud_unifier_bind_map_stack s;
    const uint32_t f = functors.id("head");
    const expr* rule_head = s.bind_backing.pool.make_functor(
        f, {s.bind_backing.pool.make_var(0u), s.bind_backing.pool.make_functor(functors.id("a"), {})});
    const expr* goal = s.bind_backing.pool.make_functor(
        f, {s.bind_backing.pool.make_functor(functors.id("b"), {}), s.bind_backing.pool.make_var(0u)});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(goal, 4), pud_fe(rule_head, 0)));
    const uint32_t goal_var = s.globalize.globalize(4u, 0u);
    EXPECT_EQ(s.normalizer.normalize(pud_fe(s.bind_backing.pool.make_var(0u), 4)),
        s.bind_backing.pool.make_functor(functors.id("a"), {}));
    EXPECT_EQ(s.bind_map.resolve(goal_var),
        s.bind_backing.pool.make_functor(functors.id("a"), {}));
}

TEST_F(PudUnifierExtendedIntegrationTest, TwoOffsetsDoNotInterfere) {
    pud_unifier_bind_map_stack s;
    const expr* atom_x = s.bind_backing.pool.make_functor(functors.id("x"), {});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 10),
        pud_fe(s.bind_backing.pool.make_var(0u), 0)));
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 20), pud_fe(atom_x, 0)));
    const uint32_t global_10 = gkey(s.globalize, 10, 0);
    const uint32_t global_20 = gkey(s.globalize, 20, 0);
    const expr* rep_10 = s.bind_map.resolve(global_10);
    const expr* rep_0 = s.bind_map.resolve(0u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep_10->content));
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep_0->content));
    EXPECT_EQ(
        std::get<expr::var>(rep_10->content).index,
        std::get<expr::var>(rep_0->content).index);
    EXPECT_EQ(s.bind_map.resolve(global_20), atom_x);
}

TEST_F(PudUnifierExtendedIntegrationTest, CalleeUnboundVersusMergedWithCaller) {
    pud_unifier_bind_map_stack s;
    const uint32_t caller = gkey(s.globalize, 1, 0);
    const uint32_t callee = gkey(s.globalize, 0, 2);
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(v(0), 1), pud_fe(v(2), 0)));
    EXPECT_EQ(s.normalizer.normalize(pud_fe(v(2), 0)), s.bind_backing.pool.make_var(caller));
    pud_unifier_bind_map_stack lone;
    EXPECT_EQ(
        lone.normalizer.normalize(pud_fe(lone.bind_backing.pool.make_var(2u), 0)),
        lone.bind_backing.pool.make_var(2u));
}

TEST_F(PudUnifierExtendedIntegrationTest, AppendStyleListAtIncreasingOffsets) {
    pud_unifier_bind_map_stack s;
    const uint32_t cons = functors.id("cons");
    const uint32_t nil = functors.id("nil");
    const expr* nil_node = s.bind_backing.pool.make_functor(nil, {});
    const expr* atom_a = s.bind_backing.pool.make_functor(functors.id("a"), {});
    const expr* atom_b = s.bind_backing.pool.make_functor(functors.id("b"), {});
    const expr* cell_a = s.bind_backing.pool.make_functor(cons, {atom_a, nil_node});
    const expr* cell_b = s.bind_backing.pool.make_functor(cons, {atom_b, nil_node});
    const expr* list_ab = s.bind_backing.pool.make_functor(cons, {atom_a, cell_b});
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 0), pud_fe(cell_a, 0)));
    EXPECT_TRUE(s.framed_unifier.unify(
        pud_fe(s.bind_backing.pool.make_var(0u), 8), pud_fe(list_ab, 4)));
    EXPECT_EQ(
        s.normalizer.normalize(pud_fe(s.bind_backing.pool.make_var(0u), 0)),
        cell_a);
    EXPECT_EQ(
        s.normalizer.normalize(pud_fe(s.bind_backing.pool.make_var(0u), 8)),
        list_ab);
}

TEST_F(PudUnifierExtendedIntegrationTest, PeanoPlusDerivesExpectedResult) {
    pud_unifier_bind_map_stack s;
    const uint32_t z_id = functors.id("z");
    const uint32_t s_id = functors.id("s");
    const uint32_t plus_id = functors.id("plus");
    const expr* z = s.bind_backing.pool.make_functor(z_id, {});
    const expr* sz = s.bind_backing.pool.make_functor(s_id, {z});
    const expr* ssz = s.bind_backing.pool.make_functor(s_id, {s.bind_backing.pool.make_functor(s_id, {z})});
    const expr* sssz = s.bind_backing.pool.make_functor(
        s_id, {s.bind_backing.pool.make_functor(s_id, {s.bind_backing.pool.make_functor(s_id, {z})})});
    const expr* goal = s.bind_backing.pool.make_functor(
        plus_id, {ssz, sz, s.bind_backing.pool.make_var(10u)});
    const expr* rule_step = s.bind_backing.pool.make_functor(
        plus_id,
        {s.bind_backing.pool.make_functor(s_id, {s.bind_backing.pool.make_var(1u)}),
         s.bind_backing.pool.make_var(2u),
         s.bind_backing.pool.make_functor(s_id, {s.bind_backing.pool.make_var(3u)})});
    const expr* rule_base = s.bind_backing.pool.make_functor(
        plus_id, {z, s.bind_backing.pool.make_var(4u), s.bind_backing.pool.make_var(4u)});

    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(goal, 0), pud_fe(rule_step, 0)));
    const expr* subgoal = s.bind_backing.pool.make_functor(plus_id, {sz, sz, s.bind_backing.pool.make_var(10u)});
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(subgoal, 0), pud_fe(rule_step, 20)));
    const expr* subgoal2 = s.bind_backing.pool.make_functor(plus_id, {z, sz, s.bind_backing.pool.make_var(10u)});
    EXPECT_TRUE(s.framed_unifier.unify(pud_fe(subgoal2, 0), pud_fe(rule_base, 40)));
    const expr* derived = s.normalizer.normalize(pud_fe(s.bind_backing.pool.make_var(10u), 0));
    EXPECT_TRUE(std::holds_alternative<expr::functor>(derived->content));
    const expr::functor& outer = std::get<expr::functor>(derived->content);
    EXPECT_EQ(outer.id, s_id);
    EXPECT_EQ(outer.args[0], z);
}
