#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include <immer/map_transient.hpp>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_refuted_nodes.hpp"
#include "infrastructure/pud_specializer.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/pud_descent.hpp"

namespace {

struct test_make_node {
    test_make_node(pud_node_id_sequencer& seq,
                   pud_node_added_specializations& specs,
                   pud_node_added_body_goals& goals,
                   pud_node_added_var_count& var_counts)
        : seq_(seq), specs_(specs), goals_(goals), var_counts_(var_counts) {}

    pud_node_id make(std::vector<pud_specialization> specs,
                     std::vector<const expr*> goals,
                     uint32_t var_count) {
        const pud_node_id id = seq_.next();
        specs_.store(id, std::move(specs));
        goals_.store(id, std::move(goals));
        var_counts_.store(id, var_count);
        return id;
    }
private:
    pud_node_id_sequencer&          seq_;
    pud_node_added_specializations& specs_;
    pud_node_added_body_goals&      goals_;
    pud_node_added_var_count&       var_counts_;
};

} // namespace

struct PudPropagateQueryFullIntegrationTest : public ::testing::Test {
    using bind_map_t    = hierarchical_bind_map<globalizer, immer::map<uint32_t, framed_expr>::transient_type>;
    using unifier_t     = unifier<globalizer, bind_map_t>;
    using specializer_t = pud_specializer<expr_pool, unifier_t>;
    using normalizer_t  = normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;
    using propagator_t  = pud_descender<
        bind_map_t,
        unifier_t,
        specializer_t,
        normalizer_t,
        test_make_node,
        expr_pool,
        globalizer,
        pud_refuted_nodes,
        pud_call_sites,
        pud_node_added_specializations,
        pud_node_added_body_goals,
        pud_node_added_var_count,
        pud_node_heads>;
    using handle = pud_descent;

    test_functors                  functors;
    expr_pool                      exprs;
    globalizer                     globalize;
    pud_node_id_sequencer          sequencer_;
    pud_node_added_specializations node_specs_;
    pud_node_added_body_goals      node_goals_;
    pud_node_added_var_count       node_var_counts_;
    pud_node_heads                 node_heads_;
    pud_refuted_nodes              refuted;
    pud_call_sites                 call_sites;
    test_make_node                 node_factory_{sequencer_, node_specs_, node_goals_, node_var_counts_};
    std::unordered_set<pud_node_id> call_site_registered_;
    pud_node_id                    anchor_id_;
    propagator_t                   propagator;

    PudPropagateQueryFullIntegrationTest()
        : propagator(node_factory_, exprs, globalize, refuted, call_sites,
                     node_specs_, node_goals_, node_var_counts_, node_heads_) {
        anchor_id_ = sequencer_.next();
        // Anchor needs var_count=2 so that query_frame_offset=2 when open_query
        // is called from descent_root(anchor_id_). With frame_offset=0 and lvc=2,
        // query_frame_offset=2 and anchor_var_global=4 > 0 (query var global key),
        // satisfying the bind ordering constraint. var(0) in the query frame (global 2)
        // is linked to the caller var via the specialize chain; var(1) (global 3) stays
        // independent, so specs on child nodes can freely bind it.
        node_var_counts_.store(anchor_id_, 2u);
        node_goals_.store(anchor_id_, {});
        node_heads_.store(anchor_id_, exprs.make_var(0));
    }

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* func(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }

    pud_node_id make_node(std::vector<pud_specialization> specs,
                          std::vector<const expr*> goals,
                          uint32_t var_count) {
        const pud_node_id id = sequencer_.next();
        node_specs_.store(id, std::move(specs));
        node_goals_.store(id, std::move(goals));
        node_var_counts_.store(id, var_count);
        return id;
    }

    void link(pud_node_id child, pud_node_id parent_arg) {
        (void)child;
        if (call_site_registered_.insert(parent_arg).second)
            call_sites.store(parent_arg, std::numeric_limits<size_t>::max());
    }

    void with_call_site(pud_node_id node, size_t id) {
        if (call_site_registered_.insert(node).second)
            call_sites.store(node, id);
    }

    pud_specialization binds(uint32_t var_idx, const expr* value) {
        return pud_specialization{.var_idx = var_idx, .value = value};
    }

    bool has_goal(pud_node_id id, const expr* goal) {
        for (const expr* stored : node_goals_.get(id)) {
            if (stored == goal)
                return true;
        }
        return false;
    }

    bool has_value(pud_node_id id, const expr* value) {
        for (const pud_specialization& spec : node_specs_.get(id)) {
            if (spec.value == value)
                return true;
        }
        return false;
    }
};

TEST_F(PudPropagateQueryFullIntegrationTest, SpecMismatchOnOpenedQueryRefusesPropagation) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node_id a = make_node({binds(0, g)}, {}, 0);
    link(a, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    EXPECT_FALSE(propagator.descend(opened, a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedNodeIsRefused) {
    const expr* goal_a = func("goal-a", {});
    const pud_node_id a = make_node({}, {goal_a}, 0);
    link(a, anchor_id_);
    refuted.set_refuted(a);
    EXPECT_FALSE(propagator.descend(propagator.descent_root(anchor_id_), a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, ConflictingSpecOnAlreadyBoundVarRefusesNode) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node_id a = make_node({binds(0, f), binds(0, g)}, {}, 0);
    link(a, anchor_id_);
    EXPECT_FALSE(propagator.descend(propagator.descent_root(anchor_id_), a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, CircularSpecRefusesNode) {
    const expr* loop = func("f", {var(0)});
    const pud_node_id a = make_node({binds(0, loop)}, {}, 0);
    link(a, anchor_id_);
    EXPECT_FALSE(propagator.descend(propagator.descent_root(anchor_id_), a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, NestedSpecMismatchRefusesChild) {
    const expr* inner_g = func("g", {});
    const expr* inner_h = func("h", {});
    const expr* outer_g = func("f", {inner_g});
    const expr* outer_h = func("f", {inner_h});
    const pud_node_id a = make_node({binds(0, outer_g)}, {}, 0);
    const pud_node_id b = make_node({binds(0, outer_h)}, {}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.descend(*at_a, b).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, ChildMismatchExcludesItFromClose) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, g)}, {goal_b}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.descend(*at_a, b).has_value());
    const pud_node_id closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_FALSE(has_goal(closed, goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SecondChildMismatchExcludesItFromClose) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, f)}, {goal_b}, 0);
    const pud_node_id c = make_node({binds(0, g)}, {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_FALSE(propagator.descend(*at_b, c).has_value());
    const pud_node_id closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedNodeRefusedEvenWithMatchingSpec) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, f)}, {goal_b}, 0);
    link(a, anchor_id_);
    link(b, a);
    refuted.set_refuted(b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.descend(*at_a, b).has_value());
    const pud_node_id closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_FALSE(has_goal(closed, goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, MismatchAtThirdNodePreventsEntryToFourth) {
    const expr* p = func("p", {});
    const expr* other = func("other", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const expr* goal_d = func("goal-d", {});
    const pud_node_id a = make_node({binds(1, p)},     {goal_a}, 0);
    const pud_node_id b = make_node({},                {goal_b}, 0);
    const pud_node_id c = make_node({binds(1, other)}, {goal_c}, 0);
    const pud_node_id d = make_node({binds(1, p)},     {goal_d}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    link(d, c);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_FALSE(propagator.descend(*at_b, c).has_value());
    const pud_node_id closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_c));
    EXPECT_FALSE(has_goal(closed, goal_d));
}

TEST_F(PudPropagateQueryFullIntegrationTest, NodeRefutedAfterParentEnteredIsStillRefused) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node_id a = make_node({}, {goal_a}, 0);
    const pud_node_id b = make_node({}, {goal_b}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    refuted.set_refuted(b);
    EXPECT_FALSE(propagator.descend(*at_a, b).has_value());
    EXPECT_FALSE(has_goal(propagator.close_query(*at_a), goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, MismatchedSiblingRefusedMatchingSiblingEnters) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node_id left  = make_node({binds(0, g)}, {goal_l}, 0);
    const pud_node_id right = make_node({binds(0, f)}, {goal_r}, 0);
    link(left,  anchor_id_);
    link(right, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    EXPECT_FALSE(propagator.descend(opened, left).has_value());
    auto at_r = propagator.descend(opened, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node_id closed = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed, goal_r));
    EXPECT_FALSE(has_goal(closed, goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefutedSiblingDoesNotBlockMatchingSibling) {
    const expr* f = func("f", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node_id left  = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node_id right = make_node({binds(0, f)}, {goal_r}, 0);
    link(left,  anchor_id_);
    link(right, anchor_id_);
    refuted.set_refuted(left);
    EXPECT_FALSE(propagator.descend(propagator.descent_root(anchor_id_), left).has_value());
    auto at_r = propagator.descend(propagator.descent_root(anchor_id_), right);
    ASSERT_TRUE(at_r.has_value());
    EXPECT_FALSE(has_goal(propagator.close_query(*at_r), goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SiblingWalksHaveIndependentBindings) {
    const expr* f = func("f", {});
    const expr* h = func("h", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node_id left  = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node_id right = make_node({binds(0, h)}, {goal_r}, 0);
    link(left,  anchor_id_);
    link(right, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_l = propagator.descend(opened, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_r = propagator.descend(opened, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node_id closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_value(closed_r, h));
    EXPECT_FALSE(has_value(closed_r, f));
    EXPECT_FALSE(has_goal(closed_r, goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OnlyCompatibleSpecChildEnters) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* h = func("h", {});
    const expr* p = func("p", {});
    const expr* goal_a  = func("goal-a",  {});
    const expr* goal_1  = func("goal-1",  {});
    const expr* goal_2  = func("goal-2",  {});
    const expr* goal_3  = func("goal-3",  {});
    const expr* goal_4  = func("goal-4",  {});
    const pud_node_id a      = make_node({binds(0, f)},    {goal_a}, 0);
    const pud_node_id first  = make_node({binds(0, f)},    {goal_1}, 0);
    const pud_node_id second = make_node({binds(0, g)},    {goal_2}, 0);
    const pud_node_id third  = make_node({binds(0, h)},    {goal_3}, 0);
    const pud_node_id fourth = make_node({binds(1, p)},    {goal_4}, 0);
    link(a,      anchor_id_);
    link(first,  a);
    link(second, a);
    link(third,  a);
    link(fourth, a);
    refuted.set_refuted(first);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.descend(*at_a, first).has_value());
    EXPECT_FALSE(propagator.descend(*at_a, second).has_value());
    EXPECT_FALSE(propagator.descend(*at_a, third).has_value());
    auto at_fourth = propagator.descend(*at_a, fourth);
    ASSERT_TRUE(at_fourth.has_value());
    const pud_node_id closed = propagator.close_query(*at_fourth);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_4));
    EXPECT_FALSE(has_goal(closed, goal_1));
    EXPECT_FALSE(has_goal(closed, goal_2));
    EXPECT_FALSE(has_goal(closed, goal_3));
}

TEST_F(PudPropagateQueryFullIntegrationTest, MismatchedChildRefusedEmptySpecChildEnters) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a  = func("goal-a",  {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_a2 = func("goal-a2", {});
    const pud_node_id a  = make_node({binds(0, f)}, {goal_a},  0);
    const pud_node_id a1 = make_node({binds(0, g)}, {goal_a1}, 0);
    const pud_node_id a2 = make_node({},            {goal_a2}, 0);
    link(a,  anchor_id_);
    link(a1, a);
    link(a2, a);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(propagator.descend(*at_a, a1).has_value());
    auto at_a2 = propagator.descend(*at_a, a2);
    ASSERT_TRUE(at_a2.has_value());
    const pud_node_id closed = propagator.close_query(*at_a2);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_a2));
    EXPECT_FALSE(has_goal(closed, goal_a1));
}

TEST_F(PudPropagateQueryFullIntegrationTest, IndependentRootsHaveIsolatedWalks) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l  = func("goal-l",  {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_l2 = func("goal-l2", {});
    const expr* goal_r  = func("goal-r",  {});
    const pud_node_id left  = make_node({binds(0, f)}, {goal_l},  0);
    const pud_node_id l1    = make_node({},            {goal_l1}, 0);
    const pud_node_id l2    = make_node({binds(0, g)}, {goal_l2}, 0);
    const pud_node_id right = make_node({binds(1, f)}, {goal_r},  0);
    link(left,  anchor_id_);
    link(l1,    left);
    link(l2,    l1);
    link(right, anchor_id_);
    handle root = propagator.descent_root(anchor_id_);
    auto at_l = propagator.descend(root, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.descend(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    EXPECT_FALSE(propagator.descend(*at_l1, l2).has_value());
    const pud_node_id closed = propagator.close_query(*at_l1);
    EXPECT_TRUE(has_goal(closed, goal_l));
    EXPECT_TRUE(has_goal(closed, goal_l1));
    EXPECT_FALSE(has_goal(closed, goal_l2));
    EXPECT_FALSE(has_goal(closed, goal_r));
    auto at_r = propagator.descend(root, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node_id closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_FALSE(has_goal(closed_r, goal_l2));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenedQueryMismatchRefusesPropagation) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node_id a = make_node({binds(0, g)}, {}, 0);
    link(a, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    EXPECT_FALSE(propagator.descend(opened, a).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, RefusedSiblingDoesNotAffectMatchingSiblingClose) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_bad = func("goal-bad", {});
    const expr* goal_ok  = func("goal-ok",  {});
    const pud_node_id bad = make_node({binds(0, g)}, {goal_bad}, 0);
    const pud_node_id ok  = make_node({binds(0, f)}, {goal_ok},  0);
    link(bad, anchor_id_);
    link(ok,  anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    EXPECT_FALSE(propagator.descend(opened, bad).has_value());
    auto at_ok = propagator.descend(opened, ok);
    ASSERT_TRUE(at_ok.has_value());
    const pud_node_id closed = propagator.close_query(*at_ok);
    EXPECT_TRUE(has_goal(closed, goal_ok));
    EXPECT_FALSE(has_goal(closed, goal_bad));
}

TEST_F(PudPropagateQueryFullIntegrationTest, NodeWithNoSpecsIsAlwaysEntered) {
    const expr* goal_a = func("goal-a", {});
    const pud_node_id a = make_node({}, {goal_a}, 0);
    link(a, anchor_id_);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_a), goal_a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, AllSpecsMatchEntersNode) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node_id a = make_node({binds(0, f), binds(1, g)}, {}, 0);
    link(a, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    const pud_node_id closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, VarToVarSpecIsEntered) {
    const expr* goal_a = func("goal-a", {});
    const pud_node_id a = make_node({binds(0, var(1))}, {goal_a}, 0);
    link(a, anchor_id_);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_a), goal_a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ConsistentNestedSpecAcrossTwoNodesEntersBoth) {
    const expr* inner = func("g", {});
    const expr* outer = func("f", {inner});
    const expr* goal_b = func("goal-b", {});
    const pud_node_id a = make_node({binds(0, outer)}, {}, 0);
    const pud_node_id b = make_node({binds(0, outer)}, {goal_b}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node_id closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_value(closed, outer));
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoallessNodeSpecAppearsInClose) {
    const expr* f = func("f", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node_id a = make_node({binds(0, f)}, {}, 0);
    const pud_node_id b = make_node({}, {goal_b}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node_id closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_TRUE(has_goal(closed, goal_b));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SpeclessNodeGoalAppearsInClose) {
    const expr* goal_a = func("goal-a", {});
    const expr* f = func("f", {});
    const pud_node_id a = make_node({}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, f)}, {}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node_id closed = propagator.close_query(*at_b);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, EmptyMiddleNodePreservesEndpointContributions) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_c = func("goal-c", {});
    const expr* f = func("f", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({},            {},       0);
    const pud_node_id c = make_node({},            {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_c));
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SpecsAndGoalsFromAllNodesAccumulateInClose) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* h = func("h", {});
    const expr* goal_b   = func("goal-b",  {});
    const expr* goal_c0  = func("goal-c0", {});
    const expr* goal_c1  = func("goal-c1", {});
    const expr* goal_c2  = func("goal-c2", {});
    const pud_node_id a = make_node({binds(0, f), binds(1, g)}, {},                       0);
    const pud_node_id b = make_node({},                         {goal_b},                 0);
    const pud_node_id c = make_node({binds(3, h)},              {goal_c0, goal_c1, goal_c2}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c0));
    EXPECT_TRUE(has_goal(closed, goal_c1));
    EXPECT_TRUE(has_goal(closed, goal_c2));
    EXPECT_EQ(node_goals_.get(closed).size(), 4u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, AlternatingSpecsAndGoalsAllAccumulateInClose) {
    const expr* goal_a0 = func("goal-a0", {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* f = func("f", {});
    const pud_node_id a = make_node({},            {goal_a0, goal_a1}, 0);
    const pud_node_id b = make_node({binds(0, f)}, {},                 0);
    const pud_node_id c = make_node({},            {},                 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a0));
    EXPECT_TRUE(has_goal(closed, goal_a1));
    EXPECT_TRUE(has_value(closed, f));
    EXPECT_EQ(node_goals_.get(closed).size(), 2u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalVarResolvesToBoundTermFromAncestor) {
    const expr* f = func("f", {});
    const pud_node_id a = make_node({binds(0, f)}, {},       0);
    const pud_node_id b = make_node({},            {},       0);
    const pud_node_id c = make_node({},            {var(0)}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_c), f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalVarFollowsChainOfBindings) {
    const expr* g = func("g", {});
    const pud_node_id a = make_node({binds(0, var(1))}, {},       0);
    const pud_node_id b = make_node({binds(1, g)},       {},       0);
    const pud_node_id c = make_node({},                  {var(0)}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_c), g));
}

TEST_F(PudPropagateQueryFullIntegrationTest, GoalFunctorArgsNormalizedFromPathBindings) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* nested   = func("h", {var(0), var(1)});
    const expr* expected = func("h", {f, g});
    const pud_node_id a = make_node({binds(0, f)}, {},        0);
    const pud_node_id b = make_node({binds(1, g)}, {},        0);
    const pud_node_id c = make_node({},            {nested},  0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    EXPECT_TRUE(has_goal(propagator.close_query(*at_c), expected));
}

TEST_F(PudPropagateQueryFullIntegrationTest, RedundantSpecOnAlreadyBoundVarSucceeds) {
    const expr* p = func("p", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const expr* goal_d = func("goal-d", {});
    const pud_node_id a = make_node({binds(0, p)}, {goal_a}, 0);
    const pud_node_id b = make_node({},            {goal_b}, 0);
    const pud_node_id c = make_node({},            {goal_c}, 0);
    const pud_node_id d = make_node({binds(0, p)}, {goal_d}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    link(d, c);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    auto at_d = propagator.descend(*at_c, d);
    ASSERT_TRUE(at_d.has_value());
    const pud_node_id closed = propagator.close_query(*at_d);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
    EXPECT_TRUE(has_goal(closed, goal_d));
    EXPECT_TRUE(has_value(closed, p));
}

TEST_F(PudPropagateQueryFullIntegrationTest, CloseContainsOnlyGoalsOnChosenPath) {
    const expr* goal_a  = func("goal-a",  {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_a2 = func("goal-a2", {});
    const expr* goal_b  = func("goal-b",  {});
    const expr* goal_b1 = func("goal-b1", {});
    const pud_node_id a  = make_node({}, {goal_a},  0);
    const pud_node_id a1 = make_node({}, {goal_a1}, 0);
    const pud_node_id a2 = make_node({}, {goal_a2}, 0);
    const pud_node_id b  = make_node({}, {goal_b},  0);
    const pud_node_id b1 = make_node({}, {goal_b1}, 0);
    link(a,  anchor_id_);
    link(a1, a);
    link(a2, a);
    link(b,  anchor_id_);
    link(b1, b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a2 = propagator.descend(*at_a, a2);
    ASSERT_TRUE(at_a2.has_value());
    const pud_node_id closed = propagator.close_query(*at_a2);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_a2));
    EXPECT_FALSE(has_goal(closed, goal_a1));
    EXPECT_FALSE(has_goal(closed, goal_b));
    EXPECT_FALSE(has_goal(closed, goal_b1));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ParallelWalksHaveIsolatedBindings) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a  = func("goal-a",  {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_b  = func("goal-b",  {});
    const expr* goal_b1 = func("goal-b1", {});
    const pud_node_id a  = make_node({},            {goal_a},  0);
    const pud_node_id a1 = make_node({binds(0, f)}, {goal_a1}, 0);
    const pud_node_id b  = make_node({},            {goal_b},  0);
    const pud_node_id b1 = make_node({binds(0, g)}, {goal_b1}, 0);
    link(a,  anchor_id_);
    link(a1, a);
    link(b,  anchor_id_);
    link(b1, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a1 = propagator.descend(*at_a, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_b = propagator.descend(opened, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_b1 = propagator.descend(*at_b, b1);
    ASSERT_TRUE(at_b1.has_value());
    const pud_node_id closed_a = propagator.close_query(*at_a1);
    const pud_node_id closed_b = propagator.close_query(*at_b1);
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

TEST_F(PudPropagateQueryFullIntegrationTest, WalkDoesNotInheritCousinBranchBindings) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* h = func("h", {});
    const pud_node_id a  = make_node({binds(0, f)}, {}, 0);
    const pud_node_id a1 = make_node({binds(1, g)}, {}, 0);
    const pud_node_id b  = make_node({},            {}, 0);
    const pud_node_id b1 = make_node({binds(1, h)}, {}, 0);
    link(a,  anchor_id_);
    link(a1, a);
    link(b,  anchor_id_);
    link(b1, b);
    handle root = propagator.descent_root(anchor_id_);
    auto at_a = propagator.descend(root, a);
    ASSERT_TRUE(at_a.has_value());
    ASSERT_TRUE(propagator.descend(*at_a, a1).has_value());
    auto at_b = propagator.descend(root, b);
    ASSERT_TRUE(at_b.has_value());
    EXPECT_TRUE(propagator.descend(*at_b, b1).has_value());
}

TEST_F(PudPropagateQueryFullIntegrationTest, EachChildCloseContainsParentAndOwnGoals) {
    const expr* goal_a  = func("goal-a",  {});
    const expr* goal_1  = func("goal-1",  {});
    const expr* goal_2a = func("goal-2a", {});
    const expr* goal_2b = func("goal-2b", {});
    const expr* goal_5a = func("goal-5a", {});
    const expr* goal_5b = func("goal-5b", {});
    const expr* goal_5c = func("goal-5c", {});
    const expr* goal_5d = func("goal-5d", {});
    const expr* goal_5e = func("goal-5e", {});
    const pud_node_id a  = make_node({}, {goal_a},                                  0);
    const pud_node_id c0 = make_node({}, {},                                        0);
    const pud_node_id c1 = make_node({}, {goal_1},                                  0);
    const pud_node_id c2 = make_node({}, {goal_2a, goal_2b},                        0);
    const pud_node_id c5 = make_node({}, {goal_5a, goal_5b, goal_5c, goal_5d, goal_5e}, 0);
    link(a,  anchor_id_);
    link(c0, a);
    link(c1, a);
    link(c2, a);
    link(c5, a);
    auto at_a0 = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a0.has_value());
    auto at_0 = propagator.descend(*at_a0, c0);
    auto at_a1 = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a1.has_value());
    auto at_1 = propagator.descend(*at_a1, c1);
    auto at_a2 = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a2.has_value());
    auto at_2 = propagator.descend(*at_a2, c2);
    auto at_a5 = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a5.has_value());
    auto at_5 = propagator.descend(*at_a5, c5);
    ASSERT_TRUE(at_0.has_value());
    ASSERT_TRUE(at_1.has_value());
    ASSERT_TRUE(at_2.has_value());
    ASSERT_TRUE(at_5.has_value());
    const pud_node_id closed_0 = propagator.close_query(*at_0);
    const pud_node_id closed_1 = propagator.close_query(*at_1);
    const pud_node_id closed_2 = propagator.close_query(*at_2);
    const pud_node_id closed_5 = propagator.close_query(*at_5);
    EXPECT_TRUE(has_goal(closed_0, goal_a));
    EXPECT_FALSE(has_goal(closed_0, goal_1));
    EXPECT_TRUE(has_goal(closed_1, goal_a));
    EXPECT_TRUE(has_goal(closed_1, goal_1));
    EXPECT_FALSE(has_goal(closed_1, goal_2a));
    EXPECT_TRUE(has_goal(closed_2, goal_a));
    EXPECT_TRUE(has_goal(closed_2, goal_2a));
    EXPECT_TRUE(has_goal(closed_2, goal_2b));
    EXPECT_FALSE(has_goal(closed_2, goal_5a));
    EXPECT_TRUE(has_goal(closed_5, goal_a));
    EXPECT_TRUE(has_goal(closed_5, goal_5a));
    EXPECT_TRUE(has_goal(closed_5, goal_5e));
    EXPECT_FALSE(has_goal(closed_5, goal_1));
    EXPECT_EQ(node_goals_.get(closed_5).size(), 6u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, RedundantSpecOnBoundVarSucceedsAndSiblingIsIndependent) {
    const expr* p = func("p", {});
    const expr* q = func("q", {});
    const expr* goal_l  = func("goal-l",  {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_l2 = func("goal-l2", {});
    const expr* goal_r  = func("goal-r",  {});
    const pud_node_id left  = make_node({binds(0, p)}, {goal_l},  0);
    const pud_node_id l1    = make_node({},            {goal_l1}, 0);
    const pud_node_id l2    = make_node({binds(0, p)}, {goal_l2}, 0);
    const pud_node_id right = make_node({binds(0, q)}, {goal_r},  0);
    link(left,  anchor_id_);
    link(l1,    left);
    link(l2,    l1);
    link(right, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_l = propagator.descend(opened, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.descend(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    auto at_l2 = propagator.descend(*at_l1, l2);
    ASSERT_TRUE(at_l2.has_value());
    const pud_node_id closed = propagator.close_query(*at_l2);
    EXPECT_TRUE(has_goal(closed, goal_l));
    EXPECT_TRUE(has_goal(closed, goal_l1));
    EXPECT_TRUE(has_goal(closed, goal_l2));
    EXPECT_FALSE(has_goal(closed, goal_r));
    EXPECT_TRUE(has_value(closed, p));
    EXPECT_FALSE(has_value(closed, q));
    auto at_r = propagator.descend(opened, right);
    ASSERT_TRUE(at_r.has_value());
    EXPECT_TRUE(has_value(propagator.close_query(*at_r), q));
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoWalksFromSameNodeCloseToTheirOwnPaths) {
    const expr* goal_a   = func("goal-a",   {});
    const expr* goal_a1  = func("goal-a1",  {});
    const expr* goal_a1a = func("goal-a1a", {});
    const expr* goal_a2  = func("goal-a2",  {});
    const pud_node_id a   = make_node({}, {goal_a},   0);
    const pud_node_id a1  = make_node({}, {goal_a1},  0);
    const pud_node_id a1a = make_node({}, {goal_a1a}, 0);
    const pud_node_id a2  = make_node({}, {goal_a2},  0);
    link(a,   anchor_id_);
    link(a1,  a);
    link(a1a, a1);
    link(a2,  a);
    handle root = propagator.descent_root(anchor_id_);
    auto at_a = propagator.descend(root, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a1 = propagator.descend(*at_a, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_a1a = propagator.descend(*at_a1, a1a);
    ASSERT_TRUE(at_a1a.has_value());
    const pud_node_id closed_deep = propagator.close_query(*at_a1a);
    EXPECT_TRUE(has_goal(closed_deep, goal_a));
    EXPECT_TRUE(has_goal(closed_deep, goal_a1));
    EXPECT_TRUE(has_goal(closed_deep, goal_a1a));
    EXPECT_FALSE(has_goal(closed_deep, goal_a2));
    auto at_a_again = propagator.descend(root, a);
    ASSERT_TRUE(at_a_again.has_value());
    auto at_a2 = propagator.descend(*at_a_again, a2);
    ASSERT_TRUE(at_a2.has_value());
    const pud_node_id closed_a2 = propagator.close_query(*at_a2);
    EXPECT_TRUE(has_goal(closed_a2, goal_a));
    EXPECT_TRUE(has_goal(closed_a2, goal_a2));
    EXPECT_FALSE(has_goal(closed_a2, goal_a1a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, BindingOnOneBranchNotVisibleOnUnrelatedBranch) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_a  = func("goal-a",  {});
    const expr* goal_a1 = func("goal-a1", {});
    const expr* goal_b  = func("goal-b",  {});
    const expr* goal_b1 = func("goal-b1", {});
    const expr* goal_b2 = func("goal-b2", {});
    const pud_node_id a  = make_node({},            {goal_a},  0);
    const pud_node_id a1 = make_node({binds(0, f)}, {goal_a1}, 0);
    const pud_node_id b  = make_node({},            {goal_b},  0);
    const pud_node_id b1 = make_node({},            {goal_b1}, 0);
    const pud_node_id b2 = make_node({binds(0, g)}, {goal_b2}, 0);
    link(a,  anchor_id_);
    link(a1, a);
    link(b,  anchor_id_);
    link(b1, b);
    link(b2, b1);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_a1 = propagator.descend(*at_a, a1);
    ASSERT_TRUE(at_a1.has_value());
    auto at_b = propagator.descend(opened, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_b1 = propagator.descend(*at_b, b1);
    ASSERT_TRUE(at_b1.has_value());
    auto at_b2 = propagator.descend(*at_b1, b2);
    ASSERT_TRUE(at_b2.has_value());
    const pud_node_id closed_a = propagator.close_query(*at_a1);
    const pud_node_id closed_b = propagator.close_query(*at_b2);
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

TEST_F(PudPropagateQueryFullIntegrationTest, SameVarBindsDifferentTermsOnSeparatePaths) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node_id left  = make_node({binds(0, f)}, {goal_l}, 0);
    const pud_node_id right = make_node({binds(0, g)}, {goal_r}, 0);
    link(left,  anchor_id_);
    link(right, anchor_id_);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_l = propagator.descend(opened, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_r = propagator.descend(opened, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node_id closed_l = propagator.close_query(*at_l);
    const pud_node_id closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_value(closed_l, f));
    EXPECT_FALSE(has_value(closed_l, g));
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_TRUE(has_value(closed_r, g));
    EXPECT_FALSE(has_value(closed_r, f));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SiblingClosesHaveIndependentGoalCounts) {
    const expr* l0 = func("l0", {});
    const expr* l1 = func("l1", {});
    const expr* r0 = func("r0", {});
    const expr* r1 = func("r1", {});
    const expr* r2 = func("r2", {});
    const expr* r3 = func("r3", {});
    const expr* r4 = func("r4", {});
    const pud_node_id left   = make_node({}, {l0, l1},             0);
    const pud_node_id right  = make_node({}, {r0, r1, r2, r3, r4}, 0);
    const pud_node_id middle = make_node({}, {},                    0);
    link(left,   anchor_id_);
    link(right,  anchor_id_);
    link(middle, anchor_id_);
    handle root = propagator.descent_root(anchor_id_);
    auto at_l = propagator.descend(root, left);
    auto at_r = propagator.descend(root, right);
    auto at_m = propagator.descend(root, middle);
    ASSERT_TRUE(at_l.has_value());
    ASSERT_TRUE(at_r.has_value());
    ASSERT_TRUE(at_m.has_value());
    const pud_node_id closed_l = propagator.close_query(*at_l);
    const pud_node_id closed_r = propagator.close_query(*at_r);
    const pud_node_id closed_m = propagator.close_query(*at_m);
    EXPECT_TRUE(has_goal(closed_l, l0));
    EXPECT_TRUE(has_goal(closed_l, l1));
    EXPECT_FALSE(has_goal(closed_l, r0));
    EXPECT_EQ(node_goals_.get(closed_l).size(), 2u);
    EXPECT_EQ(node_goals_.get(closed_r).size(), 5u);
    EXPECT_FALSE(has_goal(closed_r, l0));
    EXPECT_TRUE(node_goals_.get(closed_m).empty());
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoHandlesFromRootCloseIndependently) {
    const expr* goal_l = func("goal-l", {});
    const expr* goal_r = func("goal-r", {});
    const pud_node_id left  = make_node({}, {goal_l}, 0);
    const pud_node_id right = make_node({}, {goal_r}, 0);
    link(left,  anchor_id_);
    link(right, anchor_id_);
    handle root = propagator.descent_root(anchor_id_);
    auto at_l = propagator.descend(root, left);
    auto at_r = propagator.descend(root, right);
    ASSERT_TRUE(at_l.has_value());
    ASSERT_TRUE(at_r.has_value());
    const pud_node_id closed_l = propagator.close_query(*at_l);
    const pud_node_id closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_FALSE(has_goal(closed_r, goal_l));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SharedNewVarCountedOnceAcrossMultipleGoals) {
    const pud_node_id a = make_node({}, {var(2)}, 0);
    const pud_node_id b = make_node({}, {var(2)}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    const pud_node_id closed = propagator.close_query(*at_b);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
    ASSERT_EQ(node_goals_.get(closed).size(), 2u);
    EXPECT_EQ(node_goals_.get(closed)[0], node_goals_.get(closed)[1]);
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoDistinctUnboundVarsEachCountedOnce) {
    const pud_node_id a = make_node({}, {var(2), var(4)}, 0);
    link(a, anchor_id_);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    const pud_node_id closed = propagator.close_query(*at_a);
    EXPECT_EQ(node_var_counts_.get(closed), 2u);
    ASSERT_EQ(node_goals_.get(closed).size(), 2u);
    EXPECT_NE(node_goals_.get(closed)[0], node_goals_.get(closed)[1]);
}

TEST_F(PudPropagateQueryFullIntegrationTest, WalkAfterOpenQueryMatchesQueryTerm) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, f)}, {goal_b}, 0);
    const pud_node_id c = make_node({},            {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenVarQueryAcceptsAllNodesRegardlessOfSpec) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node_id a = make_node({binds(0, var(1))}, {goal_a}, 0);
    const pud_node_id b = make_node({},                 {goal_b}, 0);
    const pud_node_id c = make_node({},                 {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenNestedFunctorQueryMatchesConsistentNodeSpecs) {
    const expr* inner = func("g", {});
    const expr* query = func("f", {inner});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node_id a = make_node({binds(0, query)}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, query)}, {goal_b}, 0);
    const pud_node_id c = make_node({},                {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), query, anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, TwoQueriesFromRootWalkIndependently) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const expr* goal_l  = func("goal-l",  {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_r  = func("goal-r",  {});
    const expr* goal_r1 = func("goal-r1", {});
    const pud_node_id left  = make_node({binds(0, f)}, {goal_l},  0);
    const pud_node_id l1    = make_node({},            {goal_l1}, 0);
    const pud_node_id right = make_node({binds(0, g)}, {goal_r},  0);
    const pud_node_id r1    = make_node({},            {goal_r1}, 0);
    link(left,  anchor_id_);
    link(l1,    left);
    link(right, anchor_id_);
    link(r1,    right);
    handle root = propagator.descent_root(anchor_id_);
    auto query_l = propagator.open_query(root, var(0), anchor_id_).value();
    auto query_r = propagator.open_query(root, var(0), anchor_id_).value();
    auto at_l = propagator.descend(query_l, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.descend(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    auto at_r = propagator.descend(query_r, right);
    ASSERT_TRUE(at_r.has_value());
    auto at_r1 = propagator.descend(*at_r, r1);
    ASSERT_TRUE(at_r1.has_value());
    const pud_node_id closed_l = propagator.close_query(*at_l1);
    const pud_node_id closed_r = propagator.close_query(*at_r1);
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_TRUE(has_goal(closed_l, goal_l1));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_FALSE(has_goal(closed_l, goal_r1));
    EXPECT_TRUE(has_value(closed_l, f));
    EXPECT_FALSE(has_value(closed_l, g));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_TRUE(has_goal(closed_r, goal_r1));
    EXPECT_FALSE(has_goal(closed_r, goal_l1));
    EXPECT_TRUE(has_value(closed_r, g));
    EXPECT_FALSE(has_value(closed_r, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, OpenQueryClosedImmediatelyIsEmpty) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    auto empty = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    const pud_node_id closed_empty = propagator.close_query(empty);
    EXPECT_TRUE(node_goals_.get(closed_empty).empty());
    EXPECT_TRUE(node_specs_.get(closed_empty).empty());
    EXPECT_EQ(node_var_counts_.get(closed_empty), 0u);
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    link(a, anchor_id_);
    auto fresh = propagator.open_query(propagator.descent_root(anchor_id_), f, anchor_id_).value();
    auto at_a = propagator.descend(fresh, a);
    ASSERT_TRUE(at_a.has_value());
    const pud_node_id closed = propagator.close_query(*at_a);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_EQ(node_goals_.get(closed).size(), 1u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, CloseAtFirstNodeOmitsDescendantGoals) {
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const pud_node_id a = make_node({}, {goal_a}, 0);
    const pud_node_id b = make_node({}, {goal_b}, 0);
    link(a, anchor_id_);
    link(b, a);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    EXPECT_FALSE(has_goal(propagator.close_query(*at_a), goal_b));
    EXPECT_TRUE(has_goal(propagator.close_query(*at_a), goal_a));
}

TEST_F(PudPropagateQueryFullIntegrationTest, SubsequentQueryDoesNotInheritPriorQueryBindings) {
    const expr* f = func("f", {});
    const expr* g = func("g", {});
    const pud_node_id left  = make_node({binds(0, f)}, {}, 0);
    const pud_node_id right = make_node({binds(0, g)}, {}, 0);
    link(left,  anchor_id_);
    link(right, anchor_id_);
    handle root = propagator.descent_root(anchor_id_);
    auto first = propagator.open_query(root, f, anchor_id_).value();
    ASSERT_TRUE(propagator.descend(first, left).has_value());
    auto second = propagator.open_query(root, g, anchor_id_).value();
    auto at_r = propagator.descend(second, right);
    ASSERT_TRUE(at_r.has_value());
    EXPECT_FALSE(has_value(propagator.close_query(*at_r), f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, MiddleNodeWithNoSpecPassesThroughWalk) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({},            {goal_b}, 0);
    const pud_node_id c = make_node({binds(0, f)}, {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto at_a = propagator.descend(propagator.descent_root(anchor_id_), a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ConsistentSpecOnAllNodesWalksFullChain) {
    const expr* f = func("f", {});
    const expr* goal_a = func("goal-a", {});
    const expr* goal_b = func("goal-b", {});
    const expr* goal_c = func("goal-c", {});
    const pud_node_id a = make_node({binds(0, f)}, {goal_a}, 0);
    const pud_node_id b = make_node({binds(0, f)}, {goal_b}, 0);
    const pud_node_id c = make_node({binds(0, f)}, {goal_c}, 0);
    link(a, anchor_id_);
    link(b, a);
    link(c, b);
    auto opened = propagator.open_query(propagator.descent_root(anchor_id_), var(0), anchor_id_).value();
    auto at_a = propagator.descend(opened, a);
    ASSERT_TRUE(at_a.has_value());
    auto at_b = propagator.descend(*at_a, b);
    ASSERT_TRUE(at_b.has_value());
    auto at_c = propagator.descend(*at_b, c);
    ASSERT_TRUE(at_c.has_value());
    const pud_node_id closed = propagator.close_query(*at_c);
    EXPECT_TRUE(has_goal(closed, goal_a));
    EXPECT_TRUE(has_goal(closed, goal_b));
    EXPECT_TRUE(has_goal(closed, goal_c));
    EXPECT_TRUE(has_value(closed, f));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ForkedWalksFromOpenQueryCloseToTheirOwnPaths) {
    const expr* goal_l  = func("goal-l",  {});
    const expr* goal_l1 = func("goal-l1", {});
    const expr* goal_r  = func("goal-r",  {});
    const pud_node_id left  = make_node({}, {goal_l},  0);
    const pud_node_id l1    = make_node({}, {goal_l1}, 0);
    const pud_node_id right = make_node({}, {goal_r},  0);
    link(left,  anchor_id_);
    link(l1,    left);
    link(right, anchor_id_);
    handle root = propagator.descent_root(anchor_id_);
    auto opened = propagator.open_query(root, func("q", {}), anchor_id_).value();
    auto at_l = propagator.descend(opened, left);
    ASSERT_TRUE(at_l.has_value());
    auto at_l1 = propagator.descend(*at_l, l1);
    ASSERT_TRUE(at_l1.has_value());
    auto at_r = propagator.descend(opened, right);
    ASSERT_TRUE(at_r.has_value());
    const pud_node_id closed_l = propagator.close_query(*at_l1);
    const pud_node_id closed_r = propagator.close_query(*at_r);
    EXPECT_TRUE(has_goal(closed_l, goal_l));
    EXPECT_TRUE(has_goal(closed_l, goal_l1));
    EXPECT_FALSE(has_goal(closed_l, goal_r));
    EXPECT_TRUE(has_goal(closed_r, goal_r));
    EXPECT_FALSE(has_goal(closed_r, goal_l));
    EXPECT_FALSE(has_goal(closed_r, goal_l1));
}

TEST_F(PudPropagateQueryFullIntegrationTest, CallerChainGoalsNotIncludedInQueryClose) {
    const expr* goal_caller = func("goal-caller", {});
    const expr* goal_query  = func("goal-query",  {});
    const pud_node_id caller_r = make_node({}, {goal_caller}, 0);
    const pud_node_id query_r  = make_node({}, {goal_query},  0);
    link(caller_r, anchor_id_);
    link(query_r,  anchor_id_);
    auto at_caller = propagator.descend(propagator.descent_root(anchor_id_), caller_r);
    ASSERT_TRUE(at_caller.has_value());
    auto opened = propagator.open_query(*at_caller, func("q", {}), anchor_id_).value();
    auto at_query = propagator.descend(opened, query_r);
    ASSERT_TRUE(at_query.has_value());
    const pud_node_id closed = propagator.close_query(*at_query);
    EXPECT_TRUE(has_goal(closed, goal_query));
    EXPECT_FALSE(has_goal(closed, goal_caller));
}

TEST_F(PudPropagateQueryFullIntegrationTest, PropagateErasesExpandedBodyGoalFromPending) {
    const expr* goal_expanded = func("goal-expanded", {});
    const expr* goal_child    = func("goal-child",    {});
    const pud_node_id parent_node = make_node({}, {goal_expanded}, 0);
    const pud_node_id child_node  = make_node({}, {goal_child},    0);
    link(parent_node, anchor_id_);
    with_call_site(parent_node, 0);
    link(child_node, parent_node);
    auto at_parent = propagator.descend(propagator.descent_root(anchor_id_), parent_node);
    ASSERT_TRUE(at_parent.has_value());
    auto at_child = propagator.descend(*at_parent, child_node);
    ASSERT_TRUE(at_child.has_value());
    const pud_node_id closed = propagator.close_query(*at_child);
    EXPECT_FALSE(has_goal(closed, goal_expanded));
    EXPECT_TRUE(has_goal(closed, goal_child));
    EXPECT_EQ(node_goals_.get(closed).size(), 1u);
}

TEST_F(PudPropagateQueryFullIntegrationTest, SiblingWalksEraseGoalsIndependently) {
    const expr* goal_parent = func("goal-parent", {});
    const expr* goal_left   = func("goal-left",   {});
    const expr* goal_right  = func("goal-right",  {});
    const pud_node_id parent_node  = make_node({}, {goal_parent}, 0);
    const pud_node_id left_child   = make_node({}, {goal_left},   0);
    const pud_node_id right_child  = make_node({}, {goal_right},  0);
    link(parent_node,  anchor_id_);
    with_call_site(parent_node, 0);
    link(left_child,   parent_node);
    link(right_child,  parent_node);
    handle root = propagator.descent_root(anchor_id_);
    auto at_parent_l = propagator.descend(root, parent_node);
    ASSERT_TRUE(at_parent_l.has_value());
    auto at_left = propagator.descend(*at_parent_l, left_child);
    ASSERT_TRUE(at_left.has_value());
    auto at_parent_r = propagator.descend(root, parent_node);
    ASSERT_TRUE(at_parent_r.has_value());
    auto at_right = propagator.descend(*at_parent_r, right_child);
    ASSERT_TRUE(at_right.has_value());
    const pud_node_id closed_l = propagator.close_query(*at_left);
    const pud_node_id closed_r = propagator.close_query(*at_right);
    EXPECT_FALSE(has_goal(closed_l, goal_parent));
    EXPECT_TRUE(has_goal(closed_l, goal_left));
    EXPECT_FALSE(has_goal(closed_l, goal_right));
    EXPECT_FALSE(has_goal(closed_r, goal_parent));
    EXPECT_TRUE(has_goal(closed_r, goal_right));
    EXPECT_FALSE(has_goal(closed_r, goal_left));
}

TEST_F(PudPropagateQueryFullIntegrationTest, ChildGoalsAccumulateAfterErasure) {
    const expr* goal_root = func("goal-root", {});
    const expr* goal_mid  = func("goal-mid",  {});
    const expr* goal_leaf = func("goal-leaf", {});
    const pud_node_id root_node = make_node({}, {goal_root}, 0);
    const pud_node_id mid_node  = make_node({}, {goal_mid},  0);
    const pud_node_id leaf_node = make_node({}, {goal_leaf}, 0);
    link(root_node, anchor_id_);
    with_call_site(root_node, 0);
    link(mid_node, root_node);
    with_call_site(mid_node, 1);
    link(leaf_node, mid_node);
    auto at_root = propagator.descend(propagator.descent_root(anchor_id_), root_node);
    ASSERT_TRUE(at_root.has_value());
    auto at_mid = propagator.descend(*at_root, mid_node);
    ASSERT_TRUE(at_mid.has_value());
    auto at_leaf = propagator.descend(*at_mid, leaf_node);
    ASSERT_TRUE(at_leaf.has_value());
    const pud_node_id closed = propagator.close_query(*at_leaf);
    EXPECT_FALSE(has_goal(closed, goal_root));
    EXPECT_FALSE(has_goal(closed, goal_mid));
    EXPECT_TRUE(has_goal(closed, goal_leaf));
    EXPECT_EQ(node_goals_.get(closed).size(), 1u);
}
