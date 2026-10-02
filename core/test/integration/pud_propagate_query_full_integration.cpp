#include <cstdint>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_node_pool.hpp"
#include "infrastructure/pud_parents.hpp"
#include "infrastructure/pud_query_propagator.hpp"
#include "infrastructure/pud_refuted_nodes.hpp"
#include "infrastructure/pud_specializer.hpp"
#include "infrastructure/unifier.hpp"

struct PudPropagateQueryFullIntegrationTest : public ::testing::Test {
    using bind_map_t = hierarchical_bind_map<globalizer, fully_persistent_array, fully_persistent_array>;
    using unifier_t = unifier<globalizer, bind_map_t>;
    using specializer_t = pud_specializer<expr_pool, unifier_t>;
    using normalizer_t = normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;
    using propagator_t = pud_query_propagator<
        bind_map_t,
        unifier_t,
        specializer_t,
        normalizer_t,
        pud_parents,
        pud_node_pool,
        order_maintenance,
        order_maintenance,
        expr_pool,
        globalizer,
        fully_persistent_array,
        fully_persistent_array,
        pud_refuted_nodes>;
    using handle = propagator_t::query_node_handle;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    order_maintenance order;
    fully_persistent_array bindings;
    pud_node_pool pool;
    pud_parents parents;
    pud_refuted_nodes refuted;
    propagator_t propagator;

    PudPropagateQueryFullIntegrationTest()
        : propagator(parents, pool, order, order, exprs, globalize, bindings, bindings, refuted) {}

    const expr* var(uint32_t index) {
        return exprs.make_var(index);
    }

    const expr* func(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }

    const pud_node* make_node(
        std::vector<pud_specialization> specs,
        std::vector<const expr*> goals,
        uint32_t var_count) {
        return pool.make(std::move(specs), std::move(goals), var_count);
    }

    void link(const pud_node* child, const pud_node* parent) {
        parents.store(child, parent);
    }

    pud_specialization binds(uint32_t var_idx, const expr* value) {
        return pud_specialization{.var_idx = var_idx, .value = value};
    }

    bool has_goal(const pud_node* node, const expr* goal) {
        for (const expr* stored : node->added_body_goals) {
            if (stored == goal)
                return true;
        }
        return false;
    }

    bool has_value(const pud_node* node, const expr* value) {
        for (const pud_specialization& spec : node->added_specializations) {
            if (spec.value == value)
                return true;
        }
        return false;
    }
};

TEST_F(PudPropagateQueryFullIntegrationTest, ADoesNotMatchAndCIsNotReachedFromRoot) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, g)}, {}, 0);
    const pud_node* b = make_node({binds(0, f)}, {}, 0);
    const pud_node* c = make_node({}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    handle root = propagator.root();
    auto opened = propagator.open_query(root, f);
    EXPECT_FALSE(propagator.propagate(opened, a).has_value());
    EXPECT_FALSE(propagator.propagate(root, c).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedFirstEdgeIsRefused) {
    const expr* goal_a = func("goal-a", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    link(a, nullptr);
    refuted.set_refuted(a);
    EXPECT_FALSE(propagator.propagate(propagator.root(), a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, SecondSpecializationFailsAfterTheFirstMatches) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node* a = make_node({binds(0, f), binds(0, g)}, {}, 0);
    link(a, nullptr);
    EXPECT_FALSE(propagator.propagate(propagator.root(), a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, SpecOfAVarInsideItsOwnFunctorIsRefused) {
    const expr* loop = func("f", {var(0)});
    const pud_node* a = make_node({binds(0, loop)}, {}, 0);
    link(a, nullptr);
    EXPECT_FALSE(propagator.propagate(propagator.root(), a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, NestedSpecializationMismatchIsRefused) {
    const expr* inner_g = func("g", {});
    const expr* inner_h = func("h", {});
    const expr* outer_g = func("f", {inner_g});
    const expr* outer_h = func("f", {inner_h});
    const pud_node* a = make_node({binds(0, outer_g)}, {}, 0);
    const pud_node* b = make_node({binds(0, outer_h)}, {}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, b).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, StopWhenBDoesNotMatchCloseIsOnlyA) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, g)}, {goal_b}, 0);
    const pud_node* c = make_node({binds(0, f)}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, b).has_value());
    const pud_node* closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_FALSE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, StopWhenCDoesNotMatchCloseIsAAndB) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, f)}, {goal_b}, 0);
    const pud_node* c = make_node({binds(0, g)}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_FALSE(propagator.propagate(*at_b, c).has_value());
    const pud_node* closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedBRefusesAMatchingEdge) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, f)}, {goal_b}, 0);
    link(a, nullptr);
    link(b, a);
    refuted.set_refuted(b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, b).has_value());
    const pud_node* closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_FALSE(has_goal(closed, goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, DepthFourStopsAtCSoDIsAbsent) {
    const expr* p = func("p", {});
    const expr* other = func("other", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const expr* goal_d = func("goal-d", {});
    const pud_node* a = make_node({binds(1, p)}, {goal_a}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* c = make_node({binds(1, other)}, {goal_c}, 0);
    const pud_node* d = make_node({binds(1, p)}, {goal_d}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    link(d, c);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_FALSE(propagator.propagate(*at_b, c).has_value());
    const pud_node* closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_c));
    EXPECT_FALSE(has_goal(closed, goal_d));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedGrandchildAfterTheParentWasEntered) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    refuted.set_refuted(b);
    EXPECT_FALSE(propagator.propagate(*at_a, b).has_value());
    EXPECT_FALSE(has_goal(propagator.close_query(*at_a), goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, LDoesNotMatchAndRFromTheSameRootDoes) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({binds(0, g)}, {goal_l}, 0);
    const pud_node* right = make_node({binds(0, f)}, {goal_r}, 0);
    link(left, nullptr);
    link(right, nullptr);
    handle root = propagator.root();
    auto opened = propagator.open_query(root, f);
    EXPECT_FALSE(propagator.propagate(opened, left).has_value());
    auto at_r = propagator.propagate(opened, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node* closed = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed, goal_r));
    EXPECT_FALSE(has_goal(closed, goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedLWouldHaveMatchedAndRIsEntered) {
    const expr* f = func("f", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node* right = make_node({binds(0, f)}, {goal_r}, 0);
    link(left, nullptr);
    link(right, nullptr);
    refuted.set_refuted(left);
    handle root = propagator.root();
    EXPECT_FALSE(propagator.propagate(root, left).has_value());
    auto at_r = propagator.propagate(root, right);
    ASSERT_TRUE(at_r.has_value());
    EXPECT_FALSE(has_goal(propagator.close_query(*at_r), goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RDoesNotSeeTheBindingMadeOnL) {
    const expr* f = func("f", {});
    const expr* h = func("h", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node* right = make_node({binds(0, h)}, {goal_r}, 0);
    link(left, nullptr);
    link(right, nullptr);
    handle root = propagator.root();
    auto at_l = propagator.propagate(root, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_r = propagator.propagate(root, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node* closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_value(closed_r, h));
    EXPECT_FALSE(has_value(closed_r, f));
    EXPECT_FALSE(has_goal(closed_r, goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, FromATheFirstThreeChildrenAreRefused) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* h = func("h", {});
    const expr* p = func("p", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_1 = func("goal-1", {});
    const expr* goal_2 = func("goal-2", {});
    const expr* goal_3 = func("goal-3", {});
    const expr* goal_4 = func("goal-4", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* first = make_node({binds(0, f)}, {goal_1}, 0);
    const pud_node* second = make_node({binds(0, g)}, {goal_2}, 0);
    const pud_node* third = make_node({binds(0, h)}, {goal_3}, 0);
    const pud_node* fourth = make_node({binds(1, p)}, {goal_4}, 0);
    link(a, nullptr);
    link(first, a);
    link(second, a);
    link(third, a);
    link(fourth, a);
    refuted.set_refuted(first);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, first).has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, second).has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, third).has_value());
    auto at_fourth = propagator.propagate(*at_a, fourth);
    ASSERT_TRUE(at_fourth.has_value());
    const pud_node* closed = propagator.close_query(*at_fourth);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_4));
    EXPECT_FALSE(has_goal(closed, goal_1));
    EXPECT_FALSE(has_goal(closed, goal_2));
    EXPECT_FALSE(has_goal(closed, goal_3));
}

TEST_F(PudPropagateQueryFullIntegrationTest, FromARefuseA1AndEnterA2) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_a2 = func("goal-a2", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* a1 = make_node({binds(0, g)}, {goal_a1}, 0);
    const pud_node* a2 = make_node({}, {goal_a2}, 0);
    link(a, nullptr);
    link(a1, a);
    link(a2, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.propagate(*at_a, a1).has_value());
    auto at_a2 = propagator.propagate(*at_a, a2);
    ASSERT_TRUE(at_a2.has_value());
    const pud_node* closed = propagator.close_query(*at_a2);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_a2));
    EXPECT_FALSE(has_goal(closed, goal_a1));
}

TEST_F(PudPropagateQueryFullIntegrationTest, EdgeL1ToL2RefusedThenRIsItsOwnWalk) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_l2 = func("goal-l2", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node* l1 = make_node({}, {goal_l1}, 0);
    const pud_node* l2 = make_node({binds(0, g)}, {goal_l2}, 0);
    const pud_node* right = make_node({binds(1, f)}, {goal_r}, 0);
    link(left, nullptr);
    link(l1, left);
    link(l2, l1);
    link(right, nullptr);
    handle root = propagator.root();
    auto at_l = propagator.propagate(root, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.propagate(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    EXPECT_FALSE(propagator.propagate(*at_l1, l2).has_value());
    const pud_node* closed = propagator.close_query(*at_l1);
    EXPECT_TRUE(has_goal(closed, goal_l));
    EXPECT_TRUE(has_goal(closed, goal_l1));
    EXPECT_FALSE(has_goal(closed, goal_l2));
    EXPECT_FALSE(has_goal(closed, goal_r));
    auto at_r = propagator.propagate(root, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node* closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_FALSE(has_goal(closed_r, goal_l2));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenedQueryExprDoesNotMatchA) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node* a = make_node({binds(0, g)}, {}, 0);
    link(a, nullptr);
    auto opened = propagator.open_query(propagator.root(), f);
    EXPECT_FALSE(propagator.propagate(opened, a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, SiblingAfterARefusedEdgeIsASeparateWalk) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_bad = func("goal-bad", {});
    const expr* goal_ok = func("goal-ok", {});
    const pud_node* bad = make_node({binds(0, g)}, {goal_bad}, 0);
    const pud_node* ok = make_node({binds(0, f)}, {goal_ok}, 0);
    link(bad, nullptr);
    link(ok, nullptr);
    handle root = propagator.root();
    EXPECT_FALSE(propagator.propagate(root, bad).has_value());
    auto at_ok = propagator.propagate(root, ok);
    ASSERT_TRUE(at_ok.has_value());
    const pud_node* closed = propagator.close_query(*at_ok);
    EXPECT_TRUE(has_goal(closed, goal_ok));
    EXPECT_FALSE(has_goal(closed, goal_bad));
}

TEST_F(PudPropagateQueryFullIntegrationTest, EmptySpecializationsAreEntered) {
    const expr* goal_a = func("goal-a", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    link(a, nullptr);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_a), goal_a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoSpecializationsBothMatch) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node* a = make_node({binds(0, f), binds(1, g)}, {}, 0);
    link(a, nullptr);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    const pud_node* closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_TRUE(has_value(closed, g));
}

TEST_F(PudPropagateQueryFullIntegrationTest, VarToVarSpecializationIsEntered) {
    const expr* goal_a = func("goal-a", {});
    const pud_node* a = make_node({binds(0, var(1))}, {goal_a}, 0);
    link(a, nullptr);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_a), goal_a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, NestedSpecializationMatchIsEntered) {
    const expr* inner = func("g", {});
    const expr* outer = func("f", {inner});
    const expr* goal_b = func("goal-b", {});
    const pud_node* a = make_node({binds(0, outer)}, {}, 0);
    const pud_node* b = make_node({binds(0, outer)}, {goal_b}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_b), goal_b));
    EXPECT_TRUE(has_value(propagator.close_query(*at_b), outer));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ZeroGoalStepStillContributesItsSpecialization) {
    const expr* f = func("f", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node* a = make_node({binds(0, f)}, {}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node* closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_TRUE(has_goal(closed, goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ZeroSpecStepStillContributesItsGoal) {
    const expr* goal_a = func("goal-a", {});
    const expr* f = func("f", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, f)}, {}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node* closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, MiddleNodeWithNeitherDoesNotDropTheEnds) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_c = func("goal-c", {});
    const expr* f = func("f", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({}, {}, 0);
    const pud_node* c = make_node({}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_c));
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalCountsZeroOneThreeAndSpecCountsTwoZeroOne) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* h = func("h", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c0 = func("goal-c0", {});
    const expr* goal_c1 = func("goal-c1", {});
    const expr* goal_c2 = func("goal-c2", {});
    const pud_node* a = make_node({binds(0, f), binds(1, g)}, {}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* c = make_node({binds(2, h)}, {goal_c0, goal_c1, goal_c2}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_TRUE(has_value(closed, g));
    EXPECT_TRUE(has_value(closed, h));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c0));
    EXPECT_TRUE(has_goal(closed, goal_c1));
    EXPECT_TRUE(has_goal(closed, goal_c2));
    EXPECT_EQ(closed->added_body_goals.size(), 4u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalsWithoutSpecsAndSpecsWithoutGoals) {
    const expr* goal_a0 = func("goal-a0", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* f = func("f", {});
    const pud_node* a = make_node({}, {goal_a0, goal_a1}, 0);
    const pud_node* b = make_node({binds(0, f)}, {}, 0);
    const pud_node* c = make_node({}, {}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a0));
    EXPECT_TRUE(has_goal(closed, goal_a1));
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_EQ(closed->added_body_goals.size(), 2u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalOnCIsTheTermBoundOnA) {
    const expr* f = func("f", {});
    const pud_node* a = make_node({binds(0, f)}, {}, 0);
    const pud_node* b = make_node({}, {}, 0);
    const pud_node* c = make_node({}, {var(0)}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_c), f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalOnCIsTheTermBoundThroughAThenB) {
    const expr* g = func("g", {});
    const pud_node* a = make_node({binds(0, var(1))}, {}, 0);
    const pud_node* b = make_node({binds(1, g)}, {}, 0);
    const pud_node* c = make_node({}, {var(0)}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_c), g));
}

TEST_F(PudPropagateQueryFullIntegrationTest, NestedGoalArgumentsComeFromThePathAbove) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* nested = func("h", {var(0), var(1)});
    const expr* expected = func("h", {f, g});
    const pud_node* a = make_node({binds(0, f)}, {}, 0);
    const pud_node* b = make_node({binds(1, g)}, {}, 0);
    const pud_node* c = make_node({}, {nested}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_c), expected));
}

TEST_F(PudPropagateQueryFullIntegrationTest, DepthFourVarSpecializedOnAIsWhatDMatches) {
    const expr* p = func("p", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const expr* goal_d = func("goal-d", {});
    const pud_node* a = make_node({binds(1, p)}, {goal_a}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* c = make_node({}, {goal_c}, 0);
    const pud_node* d = make_node({binds(1, p)}, {goal_d}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    link(d, c);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    auto at_d = propagator.propagate(*at_c, d);
    ASSERT_TRUE(at_d.has_value());
    const pud_node* closed = propagator.close_query(*at_d);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
    EXPECT_TRUE(has_goal(closed, goal_d));
    EXPECT_TRUE(has_value(closed, p));
}

TEST_F(PudPropagateQueryFullIntegrationTest, WalkToA2OmitsA1AndTheOtherBranch) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_a2 = func("goal-a2", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_b1 = func("goal-b1", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* a1 = make_node({}, {goal_a1}, 0);
    const pud_node* a2 = make_node({}, {goal_a2}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* b1 = make_node({}, {goal_b1}, 0);
    link(a, nullptr);
    link(a1, a);
    link(a2, a);
    link(b, nullptr);
    link(b1, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a2 = propagator.propagate(*at_a, a2);
    ASSERT_TRUE(at_a2.has_value());
    const pud_node* closed = propagator.close_query(*at_a2);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_a2));
    EXPECT_FALSE(has_goal(closed, goal_a1));
    EXPECT_FALSE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_b1));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SeparateWalksToA1AndB1DoNotShareBindings) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_b1 = func("goal-b1", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* a1 = make_node({binds(0, f)}, {goal_a1}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* b1 = make_node({binds(0, g)}, {goal_b1}, 0);
    link(a, nullptr);
    link(a1, a);
    link(b, nullptr);
    link(b1, b);
    handle root = propagator.root();
    auto at_a = propagator.propagate(root, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a1 = propagator.propagate(*at_a, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_b = propagator.propagate(root, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_b1 = propagator.propagate(*at_b, b1);
    ASSERT_TRUE(at_b1.has_value());
    const pud_node* closed_a = propagator.close_query(*at_a1);
    const pud_node* closed_b = propagator.close_query(*at_b1);
    EXPECT_TRUE(has_goal(closed_a, goal_a));
    EXPECT_TRUE(has_goal(closed_a, goal_a1));
    EXPECT_FALSE(has_goal(closed_a, goal_b));
    EXPECT_TRUE(has_value(closed_a, f));
    EXPECT_FALSE(has_value(closed_a, g));
    EXPECT_TRUE(has_goal(closed_b, goal_b));
    EXPECT_TRUE(has_goal(closed_b, goal_b1));
    EXPECT_FALSE(has_goal(closed_b, goal_a1));
    EXPECT_TRUE(has_value(closed_b, g));
}

TEST_F(PudPropagateQueryFullIntegrationTest, B1WouldFailIfItSawA1sBinding) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* h = func("h", {});
    const pud_node* a = make_node({binds(0, f)}, {}, 0);
    const pud_node* a1 = make_node({binds(1, g)}, {}, 0);
    const pud_node* b = make_node({}, {}, 0);
    const pud_node* b1 = make_node({binds(1, h)}, {}, 0);
    link(a, nullptr);
    link(a1, a);
    link(b, nullptr);
    link(b1, b);
    handle root = propagator.root();
    auto at_a = propagator.propagate(root, a);
    ASSERT_TRUE(at_a.has_value());
    ASSERT_TRUE(propagator.propagate(*at_a, a1).has_value());
    auto at_b = propagator.propagate(root, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_TRUE(propagator.propagate(*at_b, b1).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, FourChildrenEachCloseIsAPlusThatChild) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_0 = func("goal-0", {});
    const expr* goal_1 = func("goal-1", {});
    const expr* goal_2a = func("goal-2a", {});
    const expr* goal_2b = func("goal-2b", {});
    const expr* goal_5a = func("goal-5a", {});
    const expr* goal_5b = func("goal-5b", {});
    const expr* goal_5c = func("goal-5c", {});
    const expr* goal_5d = func("goal-5d", {});
    const expr* goal_5e = func("goal-5e", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* c0 = make_node({}, {}, 0);
    const pud_node* c1 = make_node({}, {goal_1}, 0);
    const pud_node* c2 = make_node({}, {goal_2a, goal_2b}, 0);
    const pud_node* c5 = make_node({}, {goal_5a, goal_5b, goal_5c, goal_5d, goal_5e}, 0);
    link(a, nullptr);
    link(c0, a);
    link(c1, a);
    link(c2, a);
    link(c5, a);
    auto at_a0 = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a0.has_value());
    auto at_0 = propagator.propagate(*at_a0, c0);
    auto at_a1 = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a1.has_value());
    auto at_1 = propagator.propagate(*at_a1, c1);
    auto at_a2 = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a2.has_value());
    auto at_2 = propagator.propagate(*at_a2, c2);
    auto at_a5 = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a5.has_value());
    auto at_5 = propagator.propagate(*at_a5, c5);
    ASSERT_TRUE(at_0.has_value());
    ASSERT_TRUE(at_1.has_value());
    ASSERT_TRUE(at_2.has_value());
    ASSERT_TRUE(at_5.has_value());
    const pud_node* closed_0 = propagator.close_query(*at_0);
    const pud_node* closed_1 = propagator.close_query(*at_1);
    const pud_node* closed_2 = propagator.close_query(*at_2);
    const pud_node* closed_5 = propagator.close_query(*at_5);
    EXPECT_TRUE(has_goal(closed_0, goal_a));
    EXPECT_FALSE(has_goal(closed_0, goal_1));
    EXPECT_TRUE(has_goal(closed_1, goal_a));
    EXPECT_TRUE(has_goal(closed_1, goal_1));
    EXPECT_FALSE(has_goal(closed_1, goal_2a));
    EXPECT_TRUE(has_goal(closed_2, goal_2a));
    EXPECT_TRUE(has_goal(closed_2, goal_2b));
    EXPECT_FALSE(has_goal(closed_2, goal_5a));
    EXPECT_TRUE(has_goal(closed_5, goal_5a));
    EXPECT_TRUE(has_goal(closed_5, goal_5e));
    EXPECT_FALSE(has_goal(closed_5, goal_1));
    EXPECT_EQ(closed_5->added_body_goals.size(), 6u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, WalkToL2OmitsRAndL2MatchesTheTermBoundAtL) {
    const expr* p = func("p", {});
    const expr* q = func("q", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_l2 = func("goal-l2", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({binds(0, p)}, {goal_l}, 0);
    const pud_node* l1 = make_node({}, {goal_l1}, 0);
    const pud_node* l2 = make_node({binds(0, p)}, {goal_l2}, 0);
    const pud_node* right = make_node({binds(0, q)}, {goal_r}, 0);
    link(left, nullptr);
    link(l1, left);
    link(l2, l1);
    link(right, nullptr);
    handle root = propagator.root();
    auto at_l = propagator.propagate(root, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.propagate(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    auto at_l2 = propagator.propagate(*at_l1, l2);
    ASSERT_TRUE(at_l2.has_value());
    const pud_node* closed = propagator.close_query(*at_l2);
    EXPECT_TRUE(has_goal(closed, goal_l));
    EXPECT_TRUE(has_goal(closed, goal_l1));
    EXPECT_TRUE(has_goal(closed, goal_l2));
    EXPECT_FALSE(has_goal(closed, goal_r));
    EXPECT_TRUE(has_value(closed, p));
    EXPECT_FALSE(has_value(closed, q));
    auto at_r = propagator.propagate(root, right);
    ASSERT_TRUE(at_r.has_value());
    EXPECT_TRUE(has_value(propagator.close_query(*at_r), q));
}

TEST_F(PudPropagateQueryFullIntegrationTest, WalkToA1aAndASeparateWalkToA2) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_a1a = func("goal-a1a", {});
    const expr* goal_a2 = func("goal-a2", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* a1 = make_node({}, {goal_a1}, 0);
    const pud_node* a1a = make_node({}, {goal_a1a}, 0);
    const pud_node* a2 = make_node({}, {goal_a2}, 0);
    link(a, nullptr);
    link(a1, a);
    link(a1a, a1);
    link(a2, a);
    handle root = propagator.root();
    auto at_a = propagator.propagate(root, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a1 = propagator.propagate(*at_a, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_a1a = propagator.propagate(*at_a1, a1a);
    ASSERT_TRUE(at_a1a.has_value());
    const pud_node* closed_deep = propagator.close_query(*at_a1a);
    EXPECT_TRUE(has_goal(closed_deep, goal_a));
    EXPECT_TRUE(has_goal(closed_deep, goal_a1));
    EXPECT_TRUE(has_goal(closed_deep, goal_a1a));
    EXPECT_FALSE(has_goal(closed_deep, goal_a2));
    auto at_a_again = propagator.propagate(root, a);
    ASSERT_TRUE(at_a_again.has_value());
    auto at_a2 = propagator.propagate(*at_a_again, a2);
    ASSERT_TRUE(at_a2.has_value());
    const pud_node* closed_a2 = propagator.close_query(*at_a2);
    EXPECT_TRUE(has_goal(closed_a2, goal_a));
    EXPECT_TRUE(has_goal(closed_a2, goal_a2));
    EXPECT_FALSE(has_goal(closed_a2, goal_a1a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, CousinBindingAtA1IsNotVisibleAtB2) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_b1 = func("goal-b1", {});
    const expr* goal_b2 = func("goal-b2", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* a1 = make_node({binds(0, f)}, {goal_a1}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* b1 = make_node({}, {goal_b1}, 0);
    const pud_node* b2 = make_node({binds(0, g)}, {goal_b2}, 0);
    link(a, nullptr);
    link(a1, a);
    link(b, nullptr);
    link(b1, b);
    link(b2, b1);
    handle root = propagator.root();
    auto at_a = propagator.propagate(root, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a1 = propagator.propagate(*at_a, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_b = propagator.propagate(root, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_b1 = propagator.propagate(*at_b, b1);
    ASSERT_TRUE(at_b1.has_value());
    auto at_b2 = propagator.propagate(*at_b1, b2);
    ASSERT_TRUE(at_b2.has_value());
    const pud_node* closed_a = propagator.close_query(*at_a1);
    const pud_node* closed_b = propagator.close_query(*at_b2);
    EXPECT_TRUE(has_goal(closed_a, goal_a));
    EXPECT_TRUE(has_goal(closed_a, goal_a1));
    EXPECT_FALSE(has_goal(closed_a, goal_b2));
    EXPECT_TRUE(has_goal(closed_b, goal_b));
    EXPECT_TRUE(has_goal(closed_b, goal_b1));
    EXPECT_TRUE(has_goal(closed_b, goal_b2));
    EXPECT_FALSE(has_goal(closed_b, goal_a));
    EXPECT_TRUE(has_value(closed_b, g));
    EXPECT_FALSE(has_value(closed_b, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SameCallerVarSpecializedToDifferentFunctors) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node* right = make_node({binds(0, g)}, {goal_r}, 0);
    link(left, nullptr);
    link(right, nullptr);
    handle root = propagator.root();
    auto at_l = propagator.propagate(root, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_r = propagator.propagate(root, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node* closed_l = propagator.close_query(*at_l);
    const pud_node* closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_value(closed_l, f));
    EXPECT_FALSE(has_value(closed_l, g));
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_TRUE(has_value(closed_r, g));
    EXPECT_FALSE(has_value(closed_r, f));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ThreeSiblingsKeepTheirOwnGoalCounts) {
    const expr* l0 = func("l0", {});
    const expr* l1 = func("l1", {});
    const expr* r0 = func("r0", {});
    const expr* r1 = func("r1", {});
    const expr* r2 = func("r2", {});
    const expr* r3 = func("r3", {});
    const expr* r4 = func("r4", {});
    const pud_node* left = make_node({}, {l0, l1}, 0);
    const pud_node* right = make_node({}, {r0, r1, r2, r3, r4}, 0);
    const pud_node* middle = make_node({}, {}, 0);
    link(left, nullptr);
    link(right, nullptr);
    link(middle, nullptr);
    handle root = propagator.root();
    auto at_l = propagator.propagate(root, left);
    auto at_r = propagator.propagate(root, right);
    auto at_m = propagator.propagate(root, middle);
    ASSERT_TRUE(at_l.has_value());
    ASSERT_TRUE(at_r.has_value());
    ASSERT_TRUE(at_m.has_value());
    const pud_node* closed_l = propagator.close_query(*at_l);
    const pud_node* closed_r = propagator.close_query(*at_r);
    const pud_node* closed_m = propagator.close_query(*at_m);
    EXPECT_TRUE(has_goal(closed_l, l0));
    EXPECT_TRUE(has_goal(closed_l, l1));
    EXPECT_FALSE(has_goal(closed_l, r0));
    EXPECT_EQ(closed_l->added_body_goals.size(), 2u);
    EXPECT_EQ(closed_r->added_body_goals.size(), 5u);
    EXPECT_FALSE(has_goal(closed_r, l0));
    EXPECT_TRUE(closed_m->added_body_goals.empty());
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoHandlesFromRootCloseOnTheirOwnNodes) {
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({}, {goal_l}, 0);
    const pud_node* right = make_node({}, {goal_r}, 0);
    link(left, nullptr);
    link(right, nullptr);
    handle root = propagator.root();
    auto at_l = propagator.propagate(root, left);
    auto at_r = propagator.propagate(root, right);
    ASSERT_TRUE(at_l.has_value());
    ASSERT_TRUE(at_r.has_value());
    const pud_node* closed_l = propagator.close_query(*at_l);
    const pud_node* closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_FALSE(has_goal(closed_r, goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, CallerVarBelowTheFrameStaysThatVar) {
    const pud_node* a = make_node({}, {}, 3);
    const pud_node* child = make_node({}, {var(0)}, 0);
    link(a, nullptr);
    link(child, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto opened = propagator.open_query(*at_a, var(1));
    auto at_child = propagator.propagate(opened, child);
    ASSERT_TRUE(at_child.has_value());
    const pud_node* closed = propagator.close_query(*at_child);
    EXPECT_TRUE(has_goal(closed, var(1)));
    EXPECT_EQ(closed->added_var_count, 0u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, OneNewVarSharedByTwoGoalsCountsOnce) {
    const pud_node* a = make_node({}, {var(2)}, 0);
    const pud_node* b = make_node({}, {var(2)}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node* closed = propagator.close_query(*at_b);
    EXPECT_EQ(closed->added_var_count, 1u);
    ASSERT_EQ(closed->added_body_goals.size(), 2u);
    EXPECT_EQ(closed->added_body_goals[0], closed->added_body_goals[1]);
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoDistinctNewVarsCountAsTwo) {
    const pud_node* a = make_node({}, {var(2), var(4)}, 0);
    link(a, nullptr);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    const pud_node* closed = propagator.close_query(*at_a);
    EXPECT_EQ(closed->added_var_count, 2u);
    ASSERT_EQ(closed->added_body_goals.size(), 2u);
    EXPECT_NE(closed->added_body_goals[0], closed->added_body_goals[1]);
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenThenWalkMatchesTheQueryExpr) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, f)}, {goal_b}, 0);
    const pud_node* c = make_node({}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.root(), f);
    auto at_a = propagator.propagate(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenVarQueryWalksThreeDeep) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, var(1))}, {goal_a}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* c = make_node({}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.root(), var(1));
    auto at_a = propagator.propagate(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenNestedFunctorQueryWalksThreeDeep) {
    const expr* inner = func("g", {});
    const expr* query = func("f", {inner});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, query)}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, query)}, {goal_b}, 0);
    const pud_node* c = make_node({}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.root(), query);
    auto at_a = propagator.propagate(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, InnerQueryAfterLiveVarsOmitsTheSibling) {
    const expr* f = func("f", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_a1a = func("goal-a1a", {});
    const expr* goal_sib = func("goal-sib", {});
    const pud_node* a = make_node({}, {}, 4);
    const pud_node* a1 = make_node({binds(0, f)}, {goal_a1}, 0);
    const pud_node* a1a = make_node({}, {goal_a1a}, 0);
    const pud_node* sibling = make_node({}, {goal_sib}, 0);
    link(a, nullptr);
    link(a1, a);
    link(a1a, a1);
    link(sibling, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto opened = propagator.open_query(*at_a, f);
    auto at_a1 = propagator.propagate(opened, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_a1a = propagator.propagate(*at_a1, a1a);
    ASSERT_TRUE(at_a1a.has_value());
    const pud_node* closed = propagator.close_query(*at_a1a);
    EXPECT_TRUE(has_goal(closed, goal_a1));
    EXPECT_TRUE(has_goal(closed, goal_a1a));
    EXPECT_FALSE(has_goal(closed, goal_sib));
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoQueriesFromRootStayOnTheirOwnPaths) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_r = func("goal-r", {});
    const expr* goal_r1 = func("goal-r1", {});
    const pud_node* left = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node* l1 = make_node({binds(1, f)}, {goal_l1}, 0);
    const pud_node* right = make_node({binds(0, g)}, {goal_r}, 0);
    const pud_node* r1 = make_node({binds(1, g)}, {goal_r1}, 0);
    link(left, nullptr);
    link(l1, left);
    link(right, nullptr);
    link(r1, right);
    handle root = propagator.root();
    auto query_l = propagator.open_query(root, f);
    auto query_r = propagator.open_query(root, g);
    auto at_l = propagator.propagate(query_l, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.propagate(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    auto at_r = propagator.propagate(query_r, right);
    ASSERT_TRUE(at_r.has_value());
    auto at_r1 = propagator.propagate(*at_r, r1);
    ASSERT_TRUE(at_r1.has_value());
    const pud_node* closed_l = propagator.close_query(*at_l1);
    const pud_node* closed_r = propagator.close_query(*at_r1);
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_TRUE(has_goal(closed_l, goal_l1));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_FALSE(has_goal(closed_l, goal_r1));
    EXPECT_TRUE(has_value(closed_l, f));
    EXPECT_FALSE(has_value(closed_l, g));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_TRUE(has_goal(closed_r, goal_r1));
    EXPECT_FALSE(has_goal(closed_r, goal_l1));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenWithNothingThenAFreshOneEdgeWalk) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    auto empty = propagator.open_query(propagator.root(), f);
    const pud_node* closed_empty = propagator.close_query(empty);
    EXPECT_TRUE(closed_empty->added_body_goals.empty());
    EXPECT_TRUE(closed_empty->added_specializations.empty());
    EXPECT_EQ(closed_empty->added_var_count, 0u);
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    link(a, nullptr);
    auto fresh = propagator.open_query(propagator.root(), f);
    auto at_a = propagator.propagate(fresh, a);
    ASSERT_TRUE(at_a.has_value());
    const pud_node* closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_EQ(closed->added_body_goals.size(), 1u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, CloseAtTheFirstNodeDoesNotContainTheNext) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node* a = make_node({}, {goal_a}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    link(a, nullptr);
    link(b, a);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(has_goal(propagator.close_query(*at_a), goal_b));
    EXPECT_TRUE(has_goal(propagator.close_query(*at_a), goal_a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SecondQueryDoesNotSeeTheFirstQuerysWalk) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node* left = make_node({binds(0, f)}, {}, 0);
    const pud_node* right = make_node({binds(0, g)}, {}, 0);
    link(left, nullptr);
    link(right, nullptr);
    handle root = propagator.root();
    auto first = propagator.open_query(root, f);
    ASSERT_TRUE(propagator.propagate(first, left).has_value());
    auto second = propagator.open_query(root, g);
    auto at_r = propagator.propagate(second, right);
    ASSERT_TRUE(at_r.has_value());
    EXPECT_FALSE(has_value(propagator.close_query(*at_r), f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ThreeDeepMiddleHasNoSpecializations) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({}, {goal_b}, 0);
    const pud_node* c = make_node({binds(0, f)}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, HappyWalkRootToCContainsAAndBAndC) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node* a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node* b = make_node({binds(0, f)}, {goal_b}, 0);
    const pud_node* c = make_node({binds(0, f)}, {goal_c}, 0);
    link(a, nullptr);
    link(b, a);
    link(c, b);
    auto at_a = propagator.propagate(propagator.root(), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.propagate(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.propagate(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node* closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, HappyForkCloseOfL1IsLAndL1AndCloseOfRIsR) {
    const expr* goal_l = func("goal-l", {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node* left = make_node({}, {goal_l}, 0);
    const pud_node* l1 = make_node({}, {goal_l1}, 0);
    const pud_node* right = make_node({}, {goal_r}, 0);
    link(left, nullptr);
    link(l1, left);
    link(right, nullptr);
    handle root = propagator.root();
    auto opened = propagator.open_query(root, func("q", {}));
    auto at_l = propagator.propagate(opened, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.propagate(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    auto at_r = propagator.propagate(opened, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node* closed_l = propagator.close_query(*at_l1);
    const pud_node* closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_TRUE(has_goal(closed_l, goal_l1));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_FALSE(has_goal(closed_r, goal_l));
    EXPECT_FALSE(has_goal(closed_r, goal_l1));
}
