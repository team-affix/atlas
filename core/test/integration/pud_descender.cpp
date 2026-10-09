// pud_descender on real axiom heads, unification, and exploration graph wiring.
// Fixture holds only infrastructure; each test states its rules and expectations inline.

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map_transient.hpp>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/pud_leaves.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_refuted_nodes.hpp"
#include "infrastructure/pud_roots.hpp"
#include "infrastructure/pud_specializer.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_descent.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
struct PudDescenderIntegrationTest : public ::testing::Test {
    using bind_map_t    = hierarchical_bind_map<globalizer, immer::map<uint32_t, framed_expr>::transient_type>;
    using unifier_t     = unifier<globalizer, bind_map_t>;
    using specializer_t = pud_specializer<expr_pool, unifier_t>;
    using normalizer_t  = normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;
    using descender_t   = pud_descender<
        bind_map_t,
        unifier_t,
        specializer_t,
        normalizer_t,
        pud_node_id_sequencer,
        expr_pool,
        globalizer,
        pud_refuted_nodes,
        pud_call_sites,
        pud_node_added_specializations,
        pud_node_added_body_goals,
        pud_node_added_var_count,
        pud_node_heads,
        pud_node_added_specializations,
        pud_node_added_body_goals,
        pud_node_added_var_count>;
    using initializer_t = pud_axiom_initializer<
        pud_node_id_sequencer,
        pud_node_heads,
        pud_node_added_body_goals,
        pud_node_added_var_count,
        pud_roots>;

    test_functors                  functors;
    expr_pool                      exprs;
    globalizer                     globalize;
    pud_node_id_sequencer          sequencer_;
    pud_node_heads                 node_heads_;
    pud_node_added_specializations node_specs_;
    pud_node_added_body_goals      node_body_goals_;
    pud_node_added_var_count       node_var_counts_;
    pud_roots                      roots_;
    pud_refuted_nodes              refuted_;
    pud_call_sites                 call_sites_;
    pud_children                   children_;
    pud_leaves                     leaves_;
    descender_t                    descender_;
    initializer_t                  axiom_initializer_;

    PudDescenderIntegrationTest()
        : descender_(sequencer_, exprs, globalize, refuted_, call_sites_,
                     node_specs_, node_body_goals_, node_var_counts_, node_heads_,
                     node_specs_, node_body_goals_, node_var_counts_)
        , axiom_initializer_(sequencer_, node_heads_, node_body_goals_, node_var_counts_, roots_) {}

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* nil() { return exprs.make_functor(k_nil_functor_id, {}); }

    const expr* cons(const expr* head, const expr* tail) {
        return exprs.make_functor(k_cons_functor_id, {head, tail});
    }

    const expr* list(std::vector<const expr*> elems) {
        const expr* current = nil();
        for (auto it = elems.rbegin(); it != elems.rend(); ++it)
            current = cons(*it, current);
        return current;
    }

    const expr* nat(uint32_t value) {
        const expr* current = functor("z", {});
        for (uint32_t step = 0; step < value; ++step)
            current = functor("s", {current});
        return current;
    }

    pud_node_id register_axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        return axiom_initializer_.initialize_axiom(rule(head, std::move(body), var_count));
    }

    const expr* normalize_local_var(const pud_descent& descent, uint32_t local_var_index) {
        const uint32_t global_key = globalize.globalize(descent.frame_offset, local_var_index);
        auto bindings_transient = descent.bindings.transient();
        bind_map_t bind_map{globalize, bindings_transient};
        normalizer_t normalizer{globalize, exprs, exprs, bind_map};
        const uint32_t cutoff = descent.frame_offset + descent.lvc;
        std::unordered_map<uint32_t, uint32_t> translation;
        framed_expr raw{exprs.make_var(global_key), 0};
        framed_expr reduced = bind_map.whnf(raw);
        return normalizer.normalize(reduced, cutoff, translation);
    }
};

TEST_F(PudDescenderIntegrationTest, GroundHeadUnifiesWithMatchingQuery) {
    const pud_node_id root = register_axiom(functor("a", {}), {}, 0);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(at_root, functor("a", {}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 0u);
    EXPECT_EQ(opened->lvc, 0u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_body_goals_.get(closed).empty());
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderIntegrationTest, GroundHeadRejectsMismatchedFunctor) {
    const pud_node_id root = register_axiom(functor("a", {}), {}, 0);
    pud_descent at_root = descender_.descent_root(root);
    EXPECT_FALSE(descender_.open_query(at_root, functor("b", {}), root).has_value());
}

TEST_F(PudDescenderIntegrationTest, GroundHeadBindsQueryVariable) {
    const pud_node_id root = register_axiom(functor("a", {}), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(at_root, var(0), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, functor("a", {}));
}

TEST_F(PudDescenderIntegrationTest, FunctorOrArityMismatchRejectsQuery) {
    const pud_node_id root = register_axiom(functor("f", {var(0), var(1)}), {}, 2);
    pud_descent at_root = descender_.descent_root(root);
    EXPECT_FALSE(descender_.open_query(at_root, functor("g", {functor("a", {})}), root).has_value());
    EXPECT_FALSE(descender_.open_query(at_root, functor("f", {functor("a", {})}), root).has_value());
}

TEST_F(PudDescenderIntegrationTest, OpenQueryHonorsCallerFrameAndRootHead) {
    const pud_node_id caller_root = register_axiom(
        functor("caller", {var(0), var(1), var(2), var(3)}),
        {},
        4);
    const pud_node_id f_axiom = register_axiom(functor("f", {var(0), var(1), var(2)}), {}, 3);
    pud_descent caller = descender_.descent_root(caller_root);
    std::optional<pud_descent> opened = descender_.open_query(
        caller,
        functor("f", {functor("a", {}), var(0), var(1)}),
        f_axiom);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 3u);
    EXPECT_EQ(opened->node, f_axiom);

    const pud_node_id g_axiom = register_axiom(functor("g", {}), {}, 0);
    pud_descent at_g = descender_.descent_root(g_axiom);
    EXPECT_FALSE(descender_.open_query(at_g, functor("f", {}), g_axiom).has_value());
}

TEST_F(PudDescenderIntegrationTest, RepeatedHeadVariableForcesAgreement) {
    const pud_node_id root = register_axiom(functor("eq", {var(0), var(0)}), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("eq", {functor("a", {}), var(0)}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, functor("a", {}));

    EXPECT_FALSE(
        descender_.open_query(at_root, functor("eq", {functor("a", {}), functor("b", {})}), root).has_value());
}

TEST_F(PudDescenderIntegrationTest, NestedHeadUnifiesDeepStructure) {
    const pud_node_id root = register_axiom(functor("pair", {functor("left", {var(0)}), var(1)}), {}, 2);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("pair", {functor("left", {functor("x", {})}), var(0)}),
        root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 2u);
    EXPECT_EQ(opened->lvc, 2u);
    EXPECT_EQ(normalize_local_var(*opened, 0), functor("x", {}));
}

TEST_F(PudDescenderIntegrationTest, NestedQueryFromRuleBodySurfacesCallerBindingsOnClose) {
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0), var(1), var(2)}),
        {functor("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, fact);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 0u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 2u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, functor("tom", {}));
    EXPECT_EQ(node_specs_.get(closed)[1].value, functor("bob", {}));
    EXPECT_EQ(node_specs_.get(closed)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(closed)[1].var_idx, 2u);
}

TEST_F(PudDescenderIntegrationTest, OpenedDescentKeepsAxiomBodyGoals) {
    const expr* goal_a = functor("goal-a", {});
    const expr* goal_b = functor("goal-b", {});
    const pud_node_id root = register_axiom(functor("main", {var(0)}), {goal_a, goal_b}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("main", {functor("x", {})}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    EXPECT_EQ(opened->pending_body_goals.size(), 2u);
    EXPECT_EQ(*opened->pending_body_goals.find(0), goal_a);
    EXPECT_EQ(*opened->pending_body_goals.find(1), goal_b);
}

TEST_F(PudDescenderIntegrationTest, GrandparentChainResolvesTwoParentGoals) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)}), functor("parent", {var(2), var(1)})},
        3);
    const pud_node_id parent_tom_bob = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    const pud_node_id parent_bob_ann = register_axiom(
        functor("parent", {functor("bob", {}), functor("ann", {})}),
        {},
        0);

    pud_descent at_main = descender_.descent_root(main_id);
    const expr* first_goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_first =
        descender_.open_query(at_main, first_goal, parent_tom_bob);
    ASSERT_TRUE(opened_first.has_value());
    EXPECT_EQ(opened_first->frame_offset, 3u);
    EXPECT_EQ(opened_first->lvc, 0u);
    const pud_node_id first_child = descender_.close_query(*opened_first);
    children_.store(at_main.node, {first_child});
    call_sites_.store(at_main.node, 0);
    leaves_.set_leaf(first_child);

    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> at_first = descender_.descend(at_main, first_child);
    ASSERT_TRUE(at_first.has_value());
    EXPECT_EQ(at_first->pending_body_goals.count(resolved_at_main), 0u);
    const expr* second_goal = *at_first->pending_body_goals.find(1);
    std::optional<pud_descent> opened_second =
        descender_.open_query(*at_first, second_goal, parent_bob_ann);
    ASSERT_TRUE(opened_second.has_value());
    EXPECT_EQ(opened_second->frame_offset, 3u);
    EXPECT_EQ(opened_second->lvc, 0u);
    const pud_node_id second_child = descender_.close_query(*opened_second);
    children_.store(at_first->node, {second_child});
    call_sites_.store(at_first->node, 1);
    leaves_.unset_leaf(at_first->node);
    leaves_.set_leaf(second_child);

    const body_goal_id resolved_at_first = call_sites_.get(at_first->node);
    std::optional<pud_descent> at_second = descender_.descend(*at_first, second_child);
    ASSERT_TRUE(at_second.has_value());
    EXPECT_EQ(at_second->pending_body_goals.count(resolved_at_first), 0u);
    EXPECT_TRUE(at_second->pending_body_goals.empty());
    EXPECT_EQ(normalize_local_var(*at_second, 1), functor("ann", {}));
}

TEST_F(PudDescenderIntegrationTest, ParentFactWithWrongGroundNamesDoesNotOpen) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {functor("tom", {}), functor("bob", {})})},
        3);
    const pud_node_id wrong_fact = register_axiom(
        functor("parent", {functor("x", {}), functor("y", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    EXPECT_FALSE(descender_.open_query(at_main, goal, wrong_fact).has_value());
}

TEST_F(PudDescenderIntegrationTest, PeanoAddZeroBindsResult) {
    const pud_node_id add_zero = register_axiom(
        functor("add", {var(0), functor("z", {}), var(0)}),
        {},
        1);
    pud_descent at_root = descender_.descent_root(add_zero);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("add", {nat(2), functor("z", {}), var(0)}), add_zero);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, nat(2));
}

TEST_F(PudDescenderIntegrationTest, PeanoAddSuccDefersToRecursiveCall) {
    const expr* recursive_goal = functor("add", {var(0), var(1), var(2)});
    const pud_node_id add_succ = register_axiom(
        functor("add", {var(0), functor("s", {var(1)}), functor("s", {var(2)})}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(add_succ);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("add", {functor("a", {}), functor("s", {functor("b", {})}), var(0)}),
        add_succ);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 3u);
    const pud_node_id closed = descender_.close_query(*opened);
    // Query frame is 3; output is s(?3) and the deferred add uses the same ?3.
    const expr* expected_spec_value = functor("s", {var(3)});
    const expr* expected_body_goal =
        functor("add", {functor("a", {}), functor("b", {}), var(3)});
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, expected_spec_value);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
    EXPECT_EQ(node_body_goals_.get(closed)[0], expected_body_goal);
}

TEST_F(PudDescenderIntegrationTest, AppendNilCopiesRightList) {
    const pud_node_id append_nil = register_axiom(
        functor("append", {list({}), var(0), var(0)}),
        {},
        1);
    pud_descent at_root = descender_.descent_root(append_nil);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("append", {list({}), list({functor("a", {})}), var(0)}),
        append_nil);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, list({functor("a", {})}));
}

TEST_F(PudDescenderIntegrationTest, AppendConsPeelsHeadAndDefersTail) {
    const expr* recursive_goal = functor("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        functor("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    pud_descent at_root = descender_.descent_root(append_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("append", {list({functor("a", {})}), list({functor("b", {})}), var(0)}),
        append_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id closed = descender_.close_query(*opened);
    // Query frame is 4; normalized spec tail and recursive append goal use local ?4.
    const expr* expected_tail = var(4);
    const expr* expected_head_cell = cons(functor("a", {}), expected_tail);
    const expr* expected_body_goal =
        functor("append", {list({}), list({functor("b", {})}), var(4)});
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, expected_head_cell);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
    EXPECT_EQ(node_body_goals_.get(closed)[0], expected_body_goal);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
}

TEST_F(PudDescenderIntegrationTest, MemberOnNonEmptyListBindsTailPointer) {
    const pud_node_id member_head = register_axiom(
        functor("member", {var(0), cons(var(0), var(1))}),
        {},
        2);
    pud_descent at_root = descender_.descent_root(member_head);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("member", {functor("a", {}), list({functor("a", {}), functor("b", {})})}),
        member_head);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 2u);
    EXPECT_EQ(opened->lvc, 2u);
    EXPECT_EQ(normalize_local_var(*opened, 1), list({functor("b", {})}));
}

TEST_F(PudDescenderIntegrationTest, LengthOfEmptyListIsZero) {
    const pud_node_id length_nil = register_axiom(
        functor("length", {list({}), functor("z", {})}),
        {},
        0);
    pud_descent at_root = descender_.descent_root(length_nil);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("length", {list({}), functor("z", {})}), length_nil);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 0u);
    EXPECT_EQ(opened->lvc, 0u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderIntegrationTest, RefutedChildBlocksDescend) {
    const pud_node_id root = register_axiom(functor("main", {}), {}, 0);
    const pud_node_id child = sequencer_.next();
    node_specs_.store(child, {});
    node_body_goals_.store(child, {functor("g", {})});
    node_var_counts_.store(child, 0);
    children_.store(root, {child});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    refuted_.set_refuted(child);
    EXPECT_FALSE(descender_.descend(descender_.descent_root(root), child).has_value());
}

TEST_F(PudDescenderIntegrationTest, RefutedSiblingDoesNotBlockReachableBranch) {
    const pud_node_id root = register_axiom(functor("main", {}), {}, 0);
    const pud_node_id bad = sequencer_.next();
    const pud_node_id good = sequencer_.next();
    node_specs_.store(bad, {});
    node_body_goals_.store(bad, {});
    node_var_counts_.store(bad, 0);
    node_specs_.store(good, {});
    node_body_goals_.store(good, {functor("g", {})});
    node_var_counts_.store(good, 0);
    children_.store(root, std::vector<pud_node_id>{bad, good});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    refuted_.set_refuted(bad);
    pud_descent at_root = descender_.descent_root(root);
    const body_goal_id resolved_at_root = call_sites_.get(root);
    std::optional<pud_descent> at_good = descender_.descend(at_root, good);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_EQ(at_good->pending_body_goals.count(resolved_at_root), 0u);
    ASSERT_EQ(node_body_goals_.get(good).size(), 1u);
    EXPECT_EQ(node_body_goals_.get(good)[0], functor("g", {}));
}

TEST_F(PudDescenderIntegrationTest, GoalExpansionUpdatesChildrenLeavesAndCallSite) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, fact);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 0u);
    const pud_node_id child = descender_.close_query(*opened);
    children_.store(at_main.node, {child});
    call_sites_.store(at_main.node, 0);
    leaves_.set_leaf(child);
    EXPECT_THAT(children_.get(at_main.node), ElementsAre(child));
    EXPECT_EQ(call_sites_.get(at_main.node), 0u);
    EXPECT_FALSE(leaves_.check_leaf(at_main.node));
    EXPECT_TRUE(leaves_.check_leaf(child));
}

TEST_F(PudDescenderIntegrationTest, StoredSpecializationsReplayOnDescend) {
    const uint32_t spec_count = 50;
    std::vector<pud_specialization> specs;
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        specs.push_back(
            pud_specialization{.var_idx = spec_index, .value = functor(label.c_str(), {})});
    }
    const pud_node_id root = register_axiom(functor("main", {}), {}, 0);
    const pud_node_id heavy = sequencer_.next();
    node_specs_.store(heavy, std::move(specs));
    node_body_goals_.store(heavy, {});
    node_var_counts_.store(heavy, spec_count);
    children_.store(root, {heavy});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    pud_descent at_root = descender_.descent_root(root);
    const body_goal_id resolved_at_root = call_sites_.get(root);
    std::optional<pud_descent> at_heavy = descender_.descend(at_root, heavy);
    ASSERT_TRUE(at_heavy.has_value());
    EXPECT_EQ(at_heavy->pending_body_goals.count(resolved_at_root), 0u);
    EXPECT_EQ(at_heavy->lvc, spec_count);
    EXPECT_EQ(normalize_local_var(*at_heavy, 0), functor("v0", {}));
    EXPECT_EQ(
        normalize_local_var(*at_heavy, spec_count - 1),
        functor(("v" + std::to_string(spec_count - 1)).c_str(), {}));
}

TEST_F(PudDescenderIntegrationTest, MainGoalResolvesPeanoAddZeroOnLargeLiteral) {
    const pud_node_id add_zero = register_axiom(
        functor("add", {var(0), functor("z", {}), var(0)}),
        {},
        1);
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0)}),
        {functor("add", {nat(50), functor("z", {}), var(0)})},
        1);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, add_zero);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id child = descender_.close_query(*opened);
    children_.store(at_main.node, {child});
    call_sites_.store(at_main.node, 0);
    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> at_child = descender_.descend(at_main, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(at_child->pending_body_goals.count(resolved_at_main), 0u);
    EXPECT_EQ(normalize_local_var(*at_child, 0), nat(50));
}

TEST_F(PudDescenderIntegrationTest, ManyGroundFactRootsDisambiguateQueries) {
    std::vector<pud_node_id> fact_roots;
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        fact_roots.push_back(register_axiom(functor("pick", {functor(name.c_str(), {})}), {}, 0));
    }
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        const expr* query = functor("pick", {functor(name.c_str(), {})});
        uint32_t match_count = 0;
        for (const pud_node_id fact_root : fact_roots) {
            pud_descent at_root = descender_.descent_root(fact_root);
            std::optional<pud_descent> opened = descender_.open_query(at_root, query, fact_root);
            if (!opened.has_value())
                continue;
            EXPECT_EQ(opened->frame_offset, 0u);
            EXPECT_EQ(opened->lvc, 0u);
            ++match_count;
        }
        EXPECT_EQ(match_count, 1u);
    }
}

TEST_F(PudDescenderIntegrationTest, TwoChildrenUnderSameParentStayIndependent) {
    const pud_node_id root = register_axiom(functor("main", {}), {}, 0);
    const pud_node_id left = sequencer_.next();
    const pud_node_id right = sequencer_.next();
    node_specs_.store(left, {});
    node_body_goals_.store(left, {functor("left", {})});
    node_var_counts_.store(left, 0);
    node_specs_.store(right, {});
    node_body_goals_.store(right, {functor("right", {})});
    node_var_counts_.store(right, 0);
    children_.store(root, std::vector<pud_node_id>{left, right});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    pud_descent at_root = descender_.descent_root(root);
    const body_goal_id resolved_at_root = call_sites_.get(root);
    std::optional<pud_descent> at_left = descender_.descend(at_root, left);
    ASSERT_TRUE(at_left.has_value());
    EXPECT_EQ(at_left->pending_body_goals.count(resolved_at_root), 0u);
    std::optional<pud_descent> at_right = descender_.descend(at_root, right);
    ASSERT_TRUE(at_right.has_value());
    EXPECT_EQ(at_right->pending_body_goals.count(resolved_at_root), 0u);
    EXPECT_EQ(node_body_goals_.get(left)[0], functor("left", {}));
    EXPECT_EQ(node_body_goals_.get(right)[0], functor("right", {}));
}

TEST_F(PudDescenderIntegrationTest, FactQueryWithTwoCallerVarsNeedsNoCallerSpecs) {
    const pud_node_id parent_axiom = register_axiom(
        functor("parent", {var(0), var(1)}),
        {},
        2);
    pud_descent at_root = descender_.descent_root(parent_axiom);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("parent", {var(0), var(1)}),
        parent_axiom);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 2u);
    EXPECT_EQ(opened->lvc, 2u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderIntegrationTest, QueryFrameOffsetFollowsCallerLiveVarCount) {
    const pud_node_id narrow = register_axiom(functor("q", {}), {}, 0);
    pud_descent at_narrow = descender_.descent_root(narrow);
    std::optional<pud_descent> at_zero = descender_.open_query(at_narrow, functor("q", {}), narrow);
    ASSERT_TRUE(at_zero.has_value());
    EXPECT_EQ(at_zero->frame_offset, 0u);
    EXPECT_EQ(at_zero->lvc, 0u);

    const pud_node_id wide = register_axiom(functor("q", {}), {}, 5);
    pud_descent at_wide = descender_.descent_root(wide);
    std::optional<pud_descent> at_five = descender_.open_query(at_wide, functor("q", {}), wide);
    ASSERT_TRUE(at_five.has_value());
    EXPECT_EQ(at_five->frame_offset, 5u);
    EXPECT_EQ(at_five->lvc, 5u);
}

TEST_F(PudDescenderIntegrationTest, VarHeadUnifiesWithGroundQueryWithoutCallerSpecs) {
    const pud_node_id root = register_axiom(var(0), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(at_root, functor("q", {}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderIntegrationTest, ListShapedQueryUnifiesWithVarHead) {
    const expr* ground_list = list({functor("a", {})});
    const pud_node_id root = register_axiom(var(0), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(at_root, ground_list, root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderIntegrationTest, GroundListQueryUnifiesWithSameHead) {
    const expr* ground_list = list({functor("a", {}), functor("b", {})});
    const pud_node_id root = register_axiom(functor("same", {var(0)}), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("same", {ground_list}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
}

TEST_F(PudDescenderIntegrationTest, RootIdSelectsAxiomHeadForUnification) {
    const pud_node_id accepts = register_axiom(functor("f", {}), {}, 0);
    const pud_node_id rejects = register_axiom(functor("g", {}), {}, 0);
    pud_descent at_accepts = descender_.descent_root(accepts);
    pud_descent at_rejects = descender_.descent_root(rejects);
    std::optional<pud_descent> opened =
        descender_.open_query(at_accepts, functor("f", {}), accepts);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 0u);
    EXPECT_EQ(opened->lvc, 0u);
    EXPECT_FALSE(descender_.open_query(at_rejects, functor("f", {}), rejects).has_value());
}

TEST_F(PudDescenderIntegrationTest, FailedOpenDoesNotConsumeNextNodeId) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {functor("tom", {}), functor("bob", {})})},
        3);
    const pud_node_id wrong_fact = register_axiom(
        functor("parent", {functor("x", {}), functor("y", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    const pud_node_id next_before = sequencer_.peek();
    EXPECT_FALSE(descender_.open_query(at_main, goal, wrong_fact).has_value());
    EXPECT_EQ(sequencer_.peek(), next_before);
}

TEST_F(PudDescenderIntegrationTest, TwoFactChoicesFromSameGoalStayIndependent) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)}), functor("parent", {var(2), var(1)})},
        3);
    const pud_node_id parent_tom_bob = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    const pud_node_id parent_x_y = register_axiom(
        functor("parent", {functor("x", {}), functor("y", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_tom =
        descender_.open_query(at_main, goal, parent_tom_bob);
    ASSERT_TRUE(opened_tom.has_value());
    EXPECT_EQ(opened_tom->frame_offset, 3u);
    EXPECT_EQ(opened_tom->lvc, 0u);
    const pud_node_id tom_child = descender_.close_query(*opened_tom);
    ASSERT_EQ(node_specs_.get(tom_child).size(), 2u);
    EXPECT_EQ(node_specs_.get(tom_child)[0].value, functor("tom", {}));
    EXPECT_EQ(node_specs_.get(tom_child)[1].value, functor("bob", {}));
    std::optional<pud_descent> opened_x = descender_.open_query(at_main, goal, parent_x_y);
    ASSERT_TRUE(opened_x.has_value());
    EXPECT_EQ(opened_x->frame_offset, 3u);
    EXPECT_EQ(opened_x->lvc, 0u);
    const pud_node_id x_child = descender_.close_query(*opened_x);
    EXPECT_NE(tom_child, x_child);
    EXPECT_EQ(node_specs_.get(x_child)[0].value, functor("x", {}));
    EXPECT_EQ(node_specs_.get(x_child)[1].value, functor("y", {}));
}

TEST_F(PudDescenderIntegrationTest, SecondGrandparentGoalBindsAfterFirstChildReplay) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)}), functor("parent", {var(2), var(1)})},
        3);
    const pud_node_id parent_tom_bob = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    const pud_node_id parent_bob_ann = register_axiom(
        functor("parent", {functor("bob", {}), functor("ann", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* first_goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_first =
        descender_.open_query(at_main, first_goal, parent_tom_bob);
    ASSERT_TRUE(opened_first.has_value());
    EXPECT_EQ(opened_first->frame_offset, 3u);
    EXPECT_EQ(opened_first->lvc, 0u);
    const pud_node_id first_child = descender_.close_query(*opened_first);
    children_.store(at_main.node, {first_child});
    call_sites_.store(at_main.node, 0);
    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> at_first = descender_.descend(at_main, first_child);
    ASSERT_TRUE(at_first.has_value());
    EXPECT_EQ(at_first->pending_body_goals.count(resolved_at_main), 0u);
    const expr* second_goal = *at_first->pending_body_goals.find(1);
    std::optional<pud_descent> opened_second =
        descender_.open_query(*at_first, second_goal, parent_bob_ann);
    ASSERT_TRUE(opened_second.has_value());
    EXPECT_EQ(opened_second->frame_offset, 3u);
    EXPECT_EQ(opened_second->lvc, 0u);
    const pud_node_id second_child = descender_.close_query(*opened_second);
    ASSERT_EQ(node_specs_.get(second_child).size(), 1u);
    EXPECT_EQ(node_specs_.get(second_child)[0].var_idx, 1u);
    EXPECT_EQ(node_specs_.get(second_child)[0].value, functor("ann", {}));
}

TEST_F(PudDescenderIntegrationTest, CloseQueryTwiceYieldsDistinctIdsWithSameSpecs) {
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0), var(1), var(2)}),
        {functor("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, fact);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 0u);
    const pud_node_id first = descender_.close_query(*opened);
    const pud_node_id second = descender_.close_query(*opened);
    EXPECT_NE(first, second);
    ASSERT_EQ(node_specs_.get(first).size(), 2u);
    ASSERT_EQ(node_specs_.get(second).size(), 2u);
    EXPECT_EQ(node_specs_.get(first)[0].value, node_specs_.get(second)[0].value);
    EXPECT_EQ(node_specs_.get(first)[1].value, node_specs_.get(second)[1].value);
}

TEST_F(PudDescenderIntegrationTest, PeanoAddTwoOnesOpensOneRecursiveStep) {
    const expr* recursive_goal = functor("add", {var(0), var(1), var(2)});
    const pud_node_id add_succ = register_axiom(
        functor("add", {var(0), functor("s", {var(1)}), functor("s", {var(2)})}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(add_succ);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("add", {nat(1), nat(1), var(0)}), add_succ);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 3u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
}

TEST_F(PudDescenderIntegrationTest, PeanoAddOnePlusOneResolvesViaSuccThenZero) {
    const expr* add_body = functor("add", {var(0), var(1), var(2)});
    const pud_node_id add_zero = register_axiom(
        functor("add", {var(0), functor("z", {}), var(0)}),
        {},
        1);
    const pud_node_id add_succ = register_axiom(
        functor("add", {var(0), functor("s", {var(1)}), functor("s", {var(2)})}),
        {add_body},
        3);
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0)}),
        {functor("add", {nat(1), nat(1), var(0)})},
        1);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_succ = descender_.open_query(at_main, goal, add_succ);
    ASSERT_TRUE(opened_succ.has_value());
    EXPECT_EQ(opened_succ->frame_offset, 1u);
    EXPECT_EQ(opened_succ->lvc, 3u);
    const pud_node_id succ_child = descender_.close_query(*opened_succ);
    call_sites_.store(at_main.node, 0);
    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> after_succ = descender_.descend(at_main, succ_child);
    ASSERT_TRUE(after_succ.has_value());
    EXPECT_EQ(after_succ->pending_body_goals.count(resolved_at_main), 0u);
    const expr* zero_goal = *after_succ->pending_body_goals.find(1);
    std::optional<pud_descent> opened_zero = descender_.open_query(*after_succ, zero_goal, add_zero);
    ASSERT_TRUE(opened_zero.has_value());
    EXPECT_EQ(opened_zero->frame_offset, 2u);
    EXPECT_EQ(opened_zero->lvc, 1u);
    const pud_node_id zero_child = descender_.close_query(*opened_zero);
    ASSERT_EQ(node_specs_.get(zero_child).size(), 1u);
    EXPECT_EQ(node_specs_.get(zero_child)[0].var_idx, 1u);
    EXPECT_EQ(node_specs_.get(zero_child)[0].value, functor("s", {functor("z", {})}));
    call_sites_.store(after_succ->node, 1);
    const body_goal_id resolved_after_succ = call_sites_.get(after_succ->node);
    std::optional<pud_descent> at_end = descender_.descend(*after_succ, zero_child);
    ASSERT_TRUE(at_end.has_value());
    EXPECT_EQ(at_end->pending_body_goals.count(resolved_after_succ), 0u);
    EXPECT_EQ(normalize_local_var(*at_end, 1), nat(1));
}

TEST_F(PudDescenderIntegrationTest, PeanoAddTwoPlusTwoViaSuccChain) {
    const expr* add_body = functor("add", {var(0), var(1), var(2)});
    const pud_node_id add_zero = register_axiom(
        functor("add", {var(0), functor("z", {}), var(0)}),
        {},
        1);
    const pud_node_id add_succ = register_axiom(
        functor("add", {var(0), functor("s", {var(1)}), functor("s", {var(2)})}),
        {add_body},
        3);
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0)}),
        {functor("add", {nat(2), nat(2), var(0)})},
        1);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_first = descender_.open_query(at_main, goal, add_succ);
    ASSERT_TRUE(opened_first.has_value());
    EXPECT_EQ(opened_first->frame_offset, 1u);
    EXPECT_EQ(opened_first->lvc, 3u);
    const pud_node_id first_child = descender_.close_query(*opened_first);
    call_sites_.store(at_main.node, 0);
    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> at_first = descender_.descend(at_main, first_child);
    ASSERT_TRUE(at_first.has_value());
    EXPECT_EQ(at_first->pending_body_goals.count(resolved_at_main), 0u);
    const expr* second_goal = *at_first->pending_body_goals.find(1);
    std::optional<pud_descent> opened_second = descender_.open_query(*at_first, second_goal, add_succ);
    ASSERT_TRUE(opened_second.has_value());
    EXPECT_EQ(opened_second->frame_offset, 2u);
    EXPECT_EQ(opened_second->lvc, 3u);
    const pud_node_id second_child = descender_.close_query(*opened_second);
    call_sites_.store(at_first->node, 1);
    const body_goal_id resolved_at_first = call_sites_.get(at_first->node);
    std::optional<pud_descent> at_second = descender_.descend(*at_first, second_child);
    ASSERT_TRUE(at_second.has_value());
    EXPECT_EQ(at_second->pending_body_goals.count(resolved_at_first), 0u);
    const expr* third_goal = *at_second->pending_body_goals.find(2);
    std::optional<pud_descent> opened_zero = descender_.open_query(*at_second, third_goal, add_zero);
    ASSERT_TRUE(opened_zero.has_value());
    EXPECT_EQ(opened_zero->frame_offset, 3u);
    EXPECT_EQ(opened_zero->lvc, 1u);
    const pud_node_id zero_child = descender_.close_query(*opened_zero);
    ASSERT_EQ(node_specs_.get(zero_child).size(), 1u);
    EXPECT_EQ(node_specs_.get(zero_child)[0].var_idx, 2u);
    EXPECT_EQ(node_specs_.get(zero_child)[0].value, nat(2));
    call_sites_.store(at_second->node, 2);
    const body_goal_id resolved_at_second = call_sites_.get(at_second->node);
    std::optional<pud_descent> at_end = descender_.descend(*at_second, zero_child);
    ASSERT_TRUE(at_end.has_value());
    EXPECT_EQ(at_end->pending_body_goals.count(resolved_at_second), 0u);
    EXPECT_EQ(normalize_local_var(*at_end, 2), nat(2));
}

TEST_F(PudDescenderIntegrationTest, LeqSuccessorOnLeftDoesNotMatchEitherAxiom) {
    const pud_node_id leq_zero = register_axiom(
        functor("leq", {functor("z", {}), var(0)}),
        {},
        1);
    const expr* leq_body = functor("leq", {var(0), var(1)});
    const pud_node_id leq_succ = register_axiom(
        functor("leq", {functor("s", {var(0)}), functor("s", {var(1)})}),
        {leq_body},
        2);
    const pud_node_id main_id = register_axiom(
        functor("main", {}),
        {functor("leq", {functor("s", {functor("z", {})}), functor("z", {})})},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    EXPECT_FALSE(descender_.open_query(at_main, goal, leq_zero).has_value());
    EXPECT_FALSE(descender_.open_query(at_main, goal, leq_succ).has_value());
}

TEST_F(PudDescenderIntegrationTest, PeanoAddBackwardQueryBindsBothOperands) {
    const pud_node_id add_zero = register_axiom(
        functor("add", {var(0), functor("z", {}), var(0)}),
        {},
        1);
    pud_descent at_root = descender_.descent_root(add_zero);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("add", {var(0), var(1), functor("z", {})}),
        add_zero);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_FALSE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderIntegrationTest, PeanoAddFullyUnboundQueryKeepsRecursiveBodyGoal) {
    const expr* recursive_goal = functor("add", {var(0), var(1), var(2)});
    const pud_node_id add_succ = register_axiom(
        functor("add", {var(0), functor("s", {var(1)}), functor("s", {var(2)})}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(add_succ);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("add", {var(0), var(1), var(2)}), add_succ);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 3u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
}

TEST_F(PudDescenderIntegrationTest, AppendTwoCellLeftListOpensConsStep) {
    const expr* recursive_goal = functor("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        functor("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    pud_descent at_root = descender_.descent_root(append_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("append", {list({functor("a", {}), functor("b", {})}), list({functor("c", {})}), var(0)}),
        append_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].value, cons(functor("a", {}), var(4)));
}

TEST_F(PudDescenderIntegrationTest, AppendBackwardQueryDefersRecursiveGoal) {
    const expr* recursive_goal = functor("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        functor("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    const expr* target = list({functor("a", {}), functor("b", {}), functor("c", {})});
    pud_descent at_root = descender_.descent_root(append_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("append", {var(0), var(1), target}),
        append_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
}

TEST_F(PudDescenderIntegrationTest, AppendSharedListArgumentUnifies) {
    const expr* recursive_goal = functor("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        functor("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    const expr* shared = list({functor("a", {}), functor("a", {})});
    pud_descent at_root = descender_.descent_root(append_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("append", {var(0), var(0), shared}),
        append_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_FALSE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderIntegrationTest, DescendFailsWhenReplayedSpecContradictsBinding) {
    const expr* recursive_goal = functor("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        functor("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0)}),
        {functor("append", {list({functor("a", {})}), list({functor("b", {})}), var(0)})},
        1);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, append_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id good_child = descender_.close_query(*opened);
    call_sites_.store(at_main.node, 0);
    const pud_node_id bad = sequencer_.next();
    node_specs_.store(bad, {pud_specialization{.var_idx = 0, .value = functor("x", {})}});
    node_body_goals_.store(bad, {});
    node_var_counts_.store(bad, 0);
    children_.store(good_child, {bad});
    call_sites_.store(good_child, std::numeric_limits<size_t>::max());
    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> at_good = descender_.descend(at_main, good_child);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_EQ(at_good->pending_body_goals.count(resolved_at_main), 0u);
    EXPECT_FALSE(descender_.descend(*at_good, bad).has_value());
}

TEST_F(PudDescenderIntegrationTest, MemberHeadBindsMatchingElement) {
    const pud_node_id member_head = register_axiom(
        functor("member", {var(0), cons(var(0), var(1))}),
        {},
        2);
    pud_descent at_root = descender_.descent_root(member_head);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("member", {functor("a", {}), list({functor("a", {}), functor("b", {})})}),
        member_head);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 2u);
    EXPECT_EQ(opened->lvc, 2u);
    EXPECT_EQ(normalize_local_var(*opened, 0), functor("a", {}));
}

TEST_F(PudDescenderIntegrationTest, MemberTailDefersSearchToRecursiveGoal) {
    const expr* recursive_goal = functor("member", {var(0), var(1)});
    const pud_node_id member_tail = register_axiom(
        functor("member", {var(0), cons(var(2), var(1))}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(member_tail);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("member", {functor("b", {}), list({functor("a", {}), functor("b", {})})}),
        member_tail);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 3u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
}

TEST_F(PudDescenderIntegrationTest, LengthConsBackwardQueryDefersRecursiveLength) {
    const expr* recursive_goal = functor("length", {var(1), var(2)});
    const pud_node_id length_cons = register_axiom(
        functor("length", {cons(var(0), var(1)), functor("s", {var(2)})}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(length_cons);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("length", {var(0), functor("s", {functor("z", {})})}), length_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 3u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
    EXPECT_EQ(node_var_counts_.get(closed), 2u);
}

TEST_F(PudDescenderIntegrationTest, LengthOfThreeElementListOpensConsHead) {
    const expr* recursive_goal = functor("length", {var(1), var(2)});
    const pud_node_id length_cons = register_axiom(
        functor("length", {cons(var(0), var(1)), functor("s", {var(2)})}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(length_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("length", {list({functor("a", {}), functor("b", {}), functor("c", {})}), var(0)}),
        length_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 3u);
    EXPECT_EQ(opened->lvc, 3u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
}

TEST_F(PudDescenderIntegrationTest, ManyClosedSpecsReplayOnDescend) {
    const uint32_t spec_count = 200;
    std::vector<pud_specialization> specs;
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        specs.push_back(
            pud_specialization{.var_idx = spec_index, .value = functor(label.c_str(), {})});
    }
    const pud_node_id root = register_axiom(functor("main", {}), {}, 0);
    const pud_node_id heavy = sequencer_.next();
    node_specs_.store(heavy, std::move(specs));
    node_body_goals_.store(heavy, {});
    node_var_counts_.store(heavy, spec_count);
    children_.store(root, {heavy});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    pud_descent at_root = descender_.descent_root(root);
    const body_goal_id resolved_at_root = call_sites_.get(root);
    std::optional<pud_descent> at_heavy = descender_.descend(at_root, heavy);
    ASSERT_TRUE(at_heavy.has_value());
    EXPECT_EQ(at_heavy->pending_body_goals.count(resolved_at_root), 0u);
    EXPECT_EQ(at_heavy->lvc, spec_count);
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        EXPECT_EQ(normalize_local_var(*at_heavy, spec_index), functor(label.c_str(), {}));
    }
}

TEST_F(PudDescenderIntegrationTest, AppendLongListsOpenConsHead) {
    const expr* recursive_goal = functor("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        functor("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    std::vector<const expr*> left_elems;
    std::vector<const expr*> right_elems;
    for (uint32_t idx = 0; idx < 10; ++idx) {
        left_elems.push_back(functor(("l" + std::to_string(idx)).c_str(), {}));
        right_elems.push_back(functor(("r" + std::to_string(idx)).c_str(), {}));
    }
    pud_descent at_root = descender_.descent_root(append_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        functor("append", {list(left_elems), list(right_elems), var(0)}),
        append_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_FALSE(node_specs_.get(closed).empty());
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
}

TEST_F(PudDescenderIntegrationTest, ReverseTenElementsDefersRecursiveRevGoal) {
    const expr* recursive_goal = functor("rev", {var(1), cons(var(0), var(2)), var(3)});
    const pud_node_id rev_cons = register_axiom(
        functor("rev", {cons(var(0), var(1)), var(2), var(3)}),
        {recursive_goal},
        4);
    std::vector<const expr*> elems;
    for (uint32_t idx = 0; idx < 10; ++idx)
        elems.push_back(functor(("e" + std::to_string(idx)).c_str(), {}));
    pud_descent at_root = descender_.descent_root(rev_cons);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, functor("rev", {list(elems), list({}), var(0)}), rev_cons);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 4u);
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_body_goals_.get(closed).size(), 1u);
}

TEST_F(PudDescenderIntegrationTest, PeanoAddFiftyPlusZeroResolvesOnOneZeroStep) {
    const pud_node_id add_zero = register_axiom(
        functor("add", {var(0), functor("z", {}), var(0)}),
        {},
        1);
    const pud_node_id main_id = register_axiom(
        functor("main", {var(0)}),
        {functor("add", {nat(50), functor("z", {}), var(0)})},
        1);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, add_zero);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 1u);
    EXPECT_EQ(opened->lvc, 1u);
    const pud_node_id child = descender_.close_query(*opened);
    call_sites_.store(at_main.node, 0);
    const body_goal_id resolved_at_main = call_sites_.get(at_main.node);
    std::optional<pud_descent> at_child = descender_.descend(at_main, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(at_child->pending_body_goals.count(resolved_at_main), 0u);
    EXPECT_EQ(normalize_local_var(*at_child, 0), nat(50));
}

TEST_F(PudDescenderIntegrationTest, NatInductiveGoalOpensZeroAndSuccWithDistinctCloses) {
    const pud_node_id nat_zero = register_axiom(
        functor("nat", {functor("z", {})}),
        {},
        0);
    const pud_node_id nat_succ = register_axiom(
        functor("nat", {functor("s", {var(0)})}),
        {functor("nat", {var(0)})},
        1);
    const pud_node_id inductive = register_axiom(
        functor("inductive", {var(0)}),
        {functor("nat", {var(0)})},
        1);
    pud_descent at_inductive = descender_.descent_root(inductive);
    const expr* nat_goal = *at_inductive.pending_body_goals.find(0);

    std::optional<pud_descent> opened_zero =
        descender_.open_query(at_inductive, nat_goal, nat_zero);
    ASSERT_TRUE(opened_zero.has_value());
    EXPECT_EQ(opened_zero->frame_offset, 1u);
    EXPECT_EQ(opened_zero->lvc, 0u);
    const pud_node_id zero_child = descender_.close_query(*opened_zero);
    ASSERT_EQ(node_specs_.get(zero_child).size(), 1u);
    EXPECT_EQ(node_specs_.get(zero_child)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(zero_child)[0].value, functor("z", {}));
    EXPECT_TRUE(node_body_goals_.get(zero_child).empty());
    EXPECT_EQ(node_var_counts_.get(zero_child), 0u);

    std::optional<pud_descent> opened_succ =
        descender_.open_query(at_inductive, nat_goal, nat_succ);
    ASSERT_TRUE(opened_succ.has_value());
    EXPECT_EQ(opened_succ->frame_offset, 1u);
    EXPECT_EQ(opened_succ->lvc, 1u);
    const pud_node_id succ_child = descender_.close_query(*opened_succ);
    EXPECT_NE(zero_child, succ_child);
    const expr* expected_succ_spec = functor("s", {var(1)});
    const expr* expected_recursive_nat = functor("nat", {var(1)});
    ASSERT_EQ(node_specs_.get(succ_child).size(), 1u);
    EXPECT_EQ(node_specs_.get(succ_child)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(succ_child)[0].value, expected_succ_spec);
    EXPECT_EQ(node_var_counts_.get(succ_child), 1u);
    ASSERT_EQ(node_body_goals_.get(succ_child).size(), 1u);
    EXPECT_EQ(node_body_goals_.get(succ_child)[0], expected_recursive_nat);

    call_sites_.store(at_inductive.node, 0);
    const body_goal_id resolved_at_inductive = call_sites_.get(at_inductive.node);
    std::optional<pud_descent> at_zero_branch = descender_.descend(at_inductive, zero_child);
    ASSERT_TRUE(at_zero_branch.has_value());
    EXPECT_EQ(at_zero_branch->pending_body_goals.count(resolved_at_inductive), 0u);
    EXPECT_EQ(normalize_local_var(*at_zero_branch, 0), nat(0));

    std::optional<pud_descent> at_succ_branch = descender_.descend(at_inductive, succ_child);
    ASSERT_TRUE(at_succ_branch.has_value());
    EXPECT_EQ(at_succ_branch->pending_body_goals.count(resolved_at_inductive), 0u);
    EXPECT_EQ(at_succ_branch->lvc, 2u);
    const expr* deferred_nat = *at_succ_branch->pending_body_goals.find(1);
    EXPECT_EQ(deferred_nat, functor("nat", {var(1)}));
    EXPECT_EQ(normalize_local_var(*at_succ_branch, 0), functor("s", {var(1)}));
}

TEST_F(PudDescenderIntegrationTest, ForkedDescentsFromSharedPrefixStayIndependent) {
    const pud_node_id main_id = register_axiom(
        functor("grandparent", {var(0), var(1)}),
        {functor("parent", {var(0), var(2)}), functor("parent", {var(2), var(1)})},
        3);
    const pud_node_id parent_tom_bob = register_axiom(
        functor("parent", {functor("tom", {}), functor("bob", {})}),
        {},
        0);
    const pud_node_id parent_tom_x = register_axiom(
        functor("parent", {functor("tom", {}), functor("x", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_bob =
        descender_.open_query(at_main, goal, parent_tom_bob);
    ASSERT_TRUE(opened_bob.has_value());
    EXPECT_EQ(opened_bob->frame_offset, 3u);
    EXPECT_EQ(opened_bob->lvc, 0u);
    const pud_node_id child_bob = descender_.close_query(*opened_bob);
    std::optional<pud_descent> opened_x =
        descender_.open_query(at_main, goal, parent_tom_x);
    ASSERT_TRUE(opened_x.has_value());
    EXPECT_EQ(opened_x->frame_offset, 3u);
    EXPECT_EQ(opened_x->lvc, 0u);
    const pud_node_id child_x = descender_.close_query(*opened_x);
    EXPECT_NE(child_bob, child_x);
    EXPECT_EQ(node_specs_.get(child_bob)[1].value, functor("bob", {}));
    EXPECT_EQ(node_specs_.get(child_x)[1].value, functor("x", {}));
}
