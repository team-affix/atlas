#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"


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
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{f_xy, 0, 0}, moduled_expr{f_by, 0, 0}));
    EXPECT_TRUE(mirrored.unifier.unify(moduled_expr{mirror_by, 0, 0}, moduled_expr{mirror_xy, 0, 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), atom("b"));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(1u), 0u}), atom("a"));
    EXPECT_EQ(mirrored.normalizer.normalize(framed_expr{mirrored.pool.make_var(0u), 0u}), mirrored.pool.make_functor(functors.id("b"), {}));
    EXPECT_EQ(mirrored.normalizer.normalize(framed_expr{mirrored.pool.make_var(1u), 0u}), mirrored.pool.make_functor(functors.id("a"), {}));
}

TEST_F(PudUnifierExtendedIntegrationTest, SwappingFramesPreservesSuccessAndBindings) {
    const expr* local0 = v(0);
    const expr* local3 = v(3);
    pud_term_stack swapped;
    const bool ok_a = stack.unifier.unify(moduled_expr{local0, 4, 0}, moduled_expr{local3, 0, 0}).has_value();
    const bool ok_b = swapped.unifier.unify(moduled_expr{local3, 0, 0}, moduled_expr{local0, 4, 0}).has_value();
    EXPECT_EQ(ok_a, ok_b);
    const uint32_t stack_global = stack.globalize.globalize(4, 0);
    const uint32_t swapped_global = swapped.globalize.globalize(4, 0);
    EXPECT_EQ(
        std::get<expr::var>(stack.normalizer.normalize(framed_expr{stack.pool.make_var(stack_global), 0u})->content).index,
        std::get<expr::var>(swapped.normalizer.normalize(framed_expr{swapped.pool.make_var(swapped_global), 0u})->content).index);
}

TEST_F(PudUnifierExtendedIntegrationTest, FunctorMismatchPerformsZeroWrites) {
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    const expr* fa = atom("a");
    const expr* fb = atom("b");
    EXPECT_FALSE(stack.unifier.unify(moduled_expr{fa, 0, 0}, moduled_expr{fb, 0, 0}));
    pud_ubs_state after = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after};
    EXPECT_TRUE(before == after);
}

TEST_F(PudUnifierExtendedIntegrationTest, ArityMismatchPerformsZeroWrites) {
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    const uint32_t pid = functors.id("p");
    const expr* unary = stack.pool.make_functor(pid, {v(0)});
    const expr* binary = stack.pool.make_functor(pid, {v(0), v(1)});
    EXPECT_FALSE(stack.unifier.unify(moduled_expr{unary, 0, 0}, moduled_expr{binary, 0, 0}));
    pud_ubs_state after = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after};
    EXPECT_TRUE(before == after);
}

TEST_F(PudUnifierExtendedIntegrationTest, LastArgumentMismatchAfterPartialProcessing) {
    const expr* fxa = stack.pool.make_functor(functors.id("p"), {v(0), atom("a")});
    const expr* fxb = stack.pool.make_functor(functors.id("p"), {atom("b"), v(0)});
    EXPECT_FALSE(stack.unifier.unify(moduled_expr{fxa, 0, 0}, moduled_expr{fxb, 0, 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{stack.pool.make_var(0u), 0u}), atom("b"));
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
    EXPECT_TRUE(stack.unifier.unify(moduled_expr{deep, 0, 0}, moduled_expr{other_deep, 0, 0}));
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
    EXPECT_FALSE(stack.unifier.unify(moduled_expr{left, 0, 0}, moduled_expr{right, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarStructurallyEqualDifferentFramingSucceeds) {
    pud_term_stack s;
    const expr* a0 = s.pool.make_functor(functors.id("a"), {});
    const expr* wrapped = s.pool.make_functor(functors.id("w"), {a0});
    s.bind_map.instantiate(s.bind_map.find(0u), moduled_functor{std::get_if<expr::functor>(&(s.pool.make_functor(functors.id("w"), {a0}))->content), 0u, 0});
    const expr* wrap_rhs = s.pool.make_functor(functors.id("w"), {s.pool.make_var(1u)});
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 0, 0}, moduled_expr{wrap_rhs, 0, 0}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(1u), 0u}), a0);
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarWithDifferentBoundFunctorFails) {
    pud_term_stack s;
    s.bind_map.instantiate(s.bind_map.find(0u), moduled_functor{std::get_if<expr::functor>(&(s.pool.make_functor(functors.id("a"), {}))->content), 0u, 0});
    s.bind_map.instantiate(s.bind_map.find(1u), moduled_functor{std::get_if<expr::functor>(&(s.pool.make_functor(functors.id("b"), {}))->content), 0u, 0});
    EXPECT_FALSE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 0, 0},
        moduled_expr{s.pool.make_var(1u), 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarDiffersOnlyInNestedVarBindsNested) {
    pud_term_stack s;
    const expr* a_atom = s.pool.make_functor(functors.id("a"), {});
    s.bind_map.instantiate(s.bind_map.find(0u), moduled_functor{std::get_if<expr::functor>(&(s.pool.make_functor(functors.id("w"), {a_atom}))->content), 0u, 0});
    const expr* rhs = s.pool.make_functor(functors.id("w"), {s.pool.make_var(1u)});
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 0, 0}, moduled_expr{rhs, 0, 0}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(1u), 0u}), a_atom);
}

TEST_F(PudUnifierExtendedIntegrationTest, BoundVarWithUnboundVarBindsUnboundClass) {
    pud_term_stack s;
    const expr* ground = s.pool.make_functor(functors.id("g"), {});
    s.bind_map.instantiate(s.bind_map.find(0u), moduled_functor{std::get_if<expr::functor>(&(s.pool.make_functor(functors.id("g"), {}))->content), 0u, 0});
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 0, 0},
        moduled_expr{s.pool.make_var(1u), 0, 0}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(1u), 0u}), ground);
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursIndirectSecondUnifyFails) {
    pud_term_stack s;
    const expr* f_yz = s.pool.make_functor(
        functors.id("f"), {s.pool.make_var(1u), s.pool.make_var(2u)});
    const expr* g_x = s.pool.make_functor(functors.id("g"), {s.pool.make_var(0u)});
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 0, 0}, moduled_expr{f_yz, 0, 0}));
    EXPECT_FALSE(s.unifier.unify(
        moduled_expr{s.pool.make_var(2u), 0, 0}, moduled_expr{g_x, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursThreeMemberClassCycleFails) {
    pud_term_stack s;
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{v(1), 0, 0}));
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(1), 0, 0}, moduled_expr{v(2), 0, 0}));
    const expr* fx = stack.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_FALSE(s.unifier.unify(moduled_expr{v(2), 0, 0}, moduled_expr{fx, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursThroughBindingPlusMergeFails) {
    pud_term_stack s;
    const expr* fy = stack.pool.make_functor(functors.id("f"), {v(1)});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{fy, 0, 0}));
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(1), 0, 0}, moduled_expr{v(2), 0, 0}));
    const expr* gx = stack.pool.make_functor(functors.id("g"), {v(0)});
    EXPECT_FALSE(s.unifier.unify(moduled_expr{v(2), 0, 0}, moduled_expr{gx, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursExpandedTermFails) {
    pud_term_stack s;
    const expr* fa_y = stack.pool.make_functor(functors.id("p"), {atom("a"), v(1)});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{fa_y, 0, 0}));
    const expr* inner = stack.pool.make_functor(functors.id("p"), {atom("a"), v(1)});
    const expr* expanded = stack.pool.make_functor(functors.id("p"), {atom("a"), inner});
    EXPECT_FALSE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{expanded, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, OccursFailsWhereTheSameVarAloneWouldBind) {
    pud_term_stack s;
    const expr* loop = stack.pool.make_functor(functors.id("l"), {v(0)});
    EXPECT_FALSE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{loop, 0, 0}));
    pud_term_stack fresh;
    EXPECT_TRUE(fresh.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{atom("a"), 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, CallerCannotBindTermWithCalleeInSameClass) {
    pud_term_stack s;
    const uint32_t caller = s.globalize.globalize(2u, 0u);
    const uint32_t callee = s.globalize.globalize(0u, 1u);
    EXPECT_TRUE(s.unifier.unify(moduled_expr{s.pool.make_var(0u), 2, 0}, moduled_expr{s.pool.make_var(1u), 0, 0}));
    const expr* bad = s.pool.make_functor(functors.id("w"), {s.pool.make_var(1u)});
    EXPECT_FALSE(s.unifier.unify(moduled_expr{s.pool.make_var(0u), 2, 0}, moduled_expr{bad, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, DifferentClassVariablePassesOccurs) {
    pud_term_stack s;
    const expr* ok = stack.pool.make_functor(functors.id("w"), {v(1)});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{ok, 0, 0}));
}

TEST_F(PudUnifierExtendedIntegrationTest, SameSkeletonTwoOffsetsEquatesGlobals) {
    pud_term_stack s;
    const expr* fx = stack.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{fx, 0, 0}, moduled_expr{fx, 10, 0}));
    EXPECT_EQ(
        s.normalizer.normalize(framed_expr{s.pool.make_var(s.globalize.globalize(0, 0)), 0u}),
        s.normalizer.normalize(framed_expr{s.pool.make_var(s.globalize.globalize(10, 0)), 0u}));
}

TEST_F(PudUnifierExtendedIntegrationTest, FramedAgainstStoredOffsetZeroUsesCorrectFrames) {
    pud_term_stack s;
    const expr* y = s.pool.make_var(1u);
    const expr* fx = s.pool.make_functor(functors.id("w"), {y});
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 5, 0}, moduled_expr{fx, 0, 0}));
    const uint32_t bound_global = s.globalize.globalize(5u, 0u);
    const expr* stored = s.normalizer.normalize(framed_expr{s.pool.make_var(bound_global), 0u});
    EXPECT_TRUE(std::holds_alternative<expr::functor>(stored->content));
}

TEST_F(PudUnifierExtendedIntegrationTest, ThreeFramesMapToDistinctGlobals) {
    pud_term_stack s;
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 0, 0}, moduled_expr{v(0), 1, 0}));
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 2, 0}, moduled_expr{v(0), 3, 0}));
    const uint32_t g0 = s.globalize.globalize(0, 0);
    const uint32_t g1 = s.globalize.globalize(1, 0);
    const uint32_t g2 = s.globalize.globalize(2, 0);
    const uint32_t g3 = s.globalize.globalize(3, 0);
    EXPECT_NE(g0, g1);
    EXPECT_NE(g2, g3);
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(g0), 0u}), s.normalizer.normalize(framed_expr{s.pool.make_var(g1), 0u}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(g2), 0u}), s.normalizer.normalize(framed_expr{s.pool.make_var(g3), 0u}));
}

TEST_F(PudUnifierExtendedIntegrationTest, FreshVarToGroundBindsOnlyThatVar) {
    pud_term_stack s;
    const expr* a = s.pool.make_functor(functors.id("a"), {});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{s.pool.make_var(0u), 0, 0}, moduled_expr{a, 0, 0}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(0u), 0u}), a);
    EXPECT_EQ(s.bind_map.find(1u), (ubs_class{.root = 1, .size = 1, .rep = 1, .value = std::nullopt}));
}

TEST_F(PudUnifierExtendedIntegrationTest, PairwiseDistinctFreshVarsMergeIntoOneClass) {
    pud_term_stack s;
    for (uint32_t var = 1u; var < 6u; ++var)
        EXPECT_TRUE(s.unifier.unify(moduled_expr{s.pool.make_var(0u), 0, 0}, moduled_expr{s.pool.make_var(var), 0, 0}));
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
        moduled_expr{goal, 4, 0}, moduled_expr{rule_head, 0, 0}));
    const uint32_t goal_var = s.globalize.globalize(4u, 0u);
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(0u), 4}),
        s.pool.make_functor(functors.id("a"), {}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(goal_var), 0u}),
        s.pool.make_functor(functors.id("a"), {}));
}

TEST_F(PudUnifierExtendedIntegrationTest, TwoOffsetsDoNotInterfere) {
    pud_term_stack s;
    const expr* atom_x = s.pool.make_functor(functors.id("x"), {});
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 10, 0},
        moduled_expr{s.pool.make_var(0u), 0, 0}));
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 20, 0}, moduled_expr{atom_x, 0, 0}));
    const uint32_t global_10 = s.globalize.globalize(10, 0);
    const uint32_t global_20 = s.globalize.globalize(20, 0);
    const expr* rep_10 = s.normalizer.normalize(framed_expr{s.pool.make_var(global_10), 0u});
    const expr* rep_0 = s.normalizer.normalize(framed_expr{s.pool.make_var(0u), 0u});
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep_10->content));
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep_0->content));
    EXPECT_EQ(
        std::get<expr::var>(rep_10->content).index,
        std::get<expr::var>(rep_0->content).index);
    EXPECT_EQ(s.normalizer.normalize(framed_expr{s.pool.make_var(global_20), 0u}), atom_x);
}

TEST_F(PudUnifierExtendedIntegrationTest, CalleeUnboundVersusMergedWithCaller) {
    pud_term_stack s;
    const uint32_t caller = s.globalize.globalize(1, 0);
    const uint32_t callee = s.globalize.globalize(0, 2);
    EXPECT_TRUE(s.unifier.unify(moduled_expr{v(0), 1, 0}, moduled_expr{v(2), 0, 0}));
    EXPECT_EQ(s.normalizer.normalize(framed_expr{v(2), 0}), s.pool.make_var(caller));
    pud_term_stack lone;
    EXPECT_EQ(
        lone.normalizer.normalize(framed_expr{lone.pool.make_var(2u), 0}),
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
        moduled_expr{s.pool.make_var(0u), 0, 0}, moduled_expr{cell_a, 0, 0}));
    EXPECT_TRUE(s.unifier.unify(
        moduled_expr{s.pool.make_var(0u), 8, 0}, moduled_expr{list_ab, 4, 0}));
    EXPECT_EQ(
        s.normalizer.normalize(framed_expr{s.pool.make_var(0u), 0}),
        cell_a);
    EXPECT_EQ(
        s.normalizer.normalize(framed_expr{s.pool.make_var(0u), 8}),
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

    EXPECT_TRUE(s.unifier.unify(moduled_expr{goal, 0, 0}, moduled_expr{rule_step, 0, 0}));
    const expr* subgoal = s.pool.make_functor(plus_id, {sz, sz, s.pool.make_var(10u)});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{subgoal, 0, 0}, moduled_expr{rule_step, 20, 0}));
    const expr* subgoal2 = s.pool.make_functor(plus_id, {z, sz, s.pool.make_var(10u)});
    EXPECT_TRUE(s.unifier.unify(moduled_expr{subgoal2, 0, 0}, moduled_expr{rule_base, 40, 0}));
    const expr* derived = s.normalizer.normalize(framed_expr{s.pool.make_var(10u), 0});
    EXPECT_TRUE(std::holds_alternative<expr::functor>(derived->content));
    const expr::functor& outer = std::get<expr::functor>(derived->content);
    EXPECT_EQ(outer.id, s_id);
    EXPECT_EQ(outer.args[0], z);
}
