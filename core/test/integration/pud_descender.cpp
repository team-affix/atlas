// pud_descender on real axiom heads, unification, and exploration graph wiring.
// Fixture holds only infrastructure; each test states its rules and expectations inline.

#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map_transient.hpp>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/named_var_printer.hpp"
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
#include "infrastructure/var_names.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_descent.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::Eq;

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
    var_names                      var_names_;
    pud_node_id_sequencer          sequencer_;
    pud_node_heads                 node_heads_;
    pud_node_added_specializations node_specs_;
    pud_node_added_body_goals      node_goals_;
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
                     node_specs_, node_goals_, node_var_counts_, node_heads_,
                     node_specs_, node_goals_, node_var_counts_)
        , axiom_initializer_(sequencer_, node_heads_, node_goals_, node_var_counts_, roots_) {}

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* fn(const char* name, std::vector<const expr*> args) {
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
        const expr* current = fn("z", {});
        for (uint32_t step = 0; step < value; ++step)
            current = fn("s", {current});
        return current;
    }

    pud_node_id register_axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        return axiom_initializer_.initialize_axiom(rule(head, std::move(body), var_count));
    }

    std::string show(const expr* expression) {
        std::ostringstream os;
        named_var_printer<var_names> print_var{var_names_};
        named_functor_printer<functor_names> print_functor{functors.names};
        expr_printer<named_var_printer<var_names>, named_functor_printer<functor_names>> printer{
            os, print_var, print_functor};
        printer.print(expression);
        return os.str();
    }

    const expr* value_of(const pud_descent& descent, uint32_t local_var_index) {
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
    const pud_node_id root = register_axiom(fn("a", {}), {}, 0);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(at_root, fn("a", {}), root);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_goals_.get(closed).empty());
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderIntegrationTest, GroundHeadRejectsMismatchedFunctor) {
    const pud_node_id root = register_axiom(fn("a", {}), {}, 0);
    pud_descent at_root = descender_.descent_root(root);
    EXPECT_FALSE(descender_.open_query(at_root, fn("b", {}), root).has_value());
}

TEST_F(PudDescenderIntegrationTest, GroundHeadBindsQueryVariable) {
    const pud_node_id root = register_axiom(fn("a", {}), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(at_root, var(0), root);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(node_specs_.get(closed)[0].var_idx, 0u);
    EXPECT_EQ(show(node_specs_.get(closed)[0].value), "a");
}

TEST_F(PudDescenderIntegrationTest, FunctorOrArityMismatchRejectsQuery) {
    const pud_node_id root = register_axiom(fn("f", {var(0), var(1)}), {}, 2);
    pud_descent at_root = descender_.descent_root(root);
    EXPECT_FALSE(descender_.open_query(at_root, fn("g", {fn("a", {})}), root).has_value());
    EXPECT_FALSE(descender_.open_query(at_root, fn("f", {fn("a", {})}), root).has_value());
}

TEST_F(PudDescenderIntegrationTest, OpenQueryHonorsCallerFrameAndRootHead) {
    const pud_node_id root = register_axiom(fn("f", {var(0), var(1), var(2)}), {}, 3);
    pud_descent caller = descender_.descent_root(root);
    caller.lvc = 4;
    std::optional<pud_descent> opened =
        descender_.open_query(caller, fn("f", {fn("a", {}), var(0), var(1)}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 3u);
    EXPECT_EQ(opened->node, root);

    const pud_node_id other_root = register_axiom(fn("g", {}), {}, 0);
    pud_descent at_other = descender_.descent_root(other_root);
    EXPECT_FALSE(descender_.open_query(at_other, fn("f", {}), other_root).has_value());
}

TEST_F(PudDescenderIntegrationTest, RepeatedHeadVariableForcesAgreement) {
    const pud_node_id root = register_axiom(fn("eq", {var(0), var(0)}), {}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, fn("eq", {fn("a", {}), var(0)}), root);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(show(node_specs_.get(closed)[0].value), "a");

    EXPECT_FALSE(
        descender_.open_query(at_root, fn("eq", {fn("a", {}), fn("b", {})}), root).has_value());
}

TEST_F(PudDescenderIntegrationTest, NestedHeadUnifiesDeepStructure) {
    const pud_node_id root = register_axiom(fn("pair", {fn("left", {var(0)}), var(1)}), {}, 2);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        fn("pair", {fn("left", {fn("x", {})}), var(0)}),
        root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(show(value_of(*opened, 0)), "x");
}

TEST_F(PudDescenderIntegrationTest, NestedQueryFromRuleBodySurfacesCallerBindingsOnClose) {
    const pud_node_id main_id = register_axiom(
        fn("main", {var(0), var(1), var(2)}),
        {fn("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = register_axiom(
        fn("parent", {fn("tom", {}), fn("bob", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, fact);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 2u);
    EXPECT_EQ(show(node_specs_.get(closed)[0].value), "tom");
    EXPECT_EQ(show(node_specs_.get(closed)[1].value), "bob");
    EXPECT_EQ(node_specs_.get(closed)[0].var_idx, 0u);
    EXPECT_EQ(node_specs_.get(closed)[1].var_idx, 2u);
}

TEST_F(PudDescenderIntegrationTest, OpenedDescentKeepsAxiomBodyGoals) {
    const expr* goal_a = fn("goal-a", {});
    const expr* goal_b = fn("goal-b", {});
    const pud_node_id root = register_axiom(fn("main", {var(0)}), {goal_a, goal_b}, 1);
    pud_descent at_root = descender_.descent_root(root);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, fn("main", {fn("x", {})}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->pending_body_goals.size(), 2u);
    EXPECT_EQ(show(*opened->pending_body_goals.find(0)), "goal-a");
    EXPECT_EQ(show(*opened->pending_body_goals.find(1)), "goal-b");
}

TEST_F(PudDescenderIntegrationTest, GrandparentChainResolvesTwoParentGoals) {
    const pud_node_id main_id = register_axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)}), fn("parent", {var(2), var(1)})},
        3);
    const pud_node_id parent_tom_bob = register_axiom(
        fn("parent", {fn("tom", {}), fn("bob", {})}),
        {},
        0);
    const pud_node_id parent_bob_ann = register_axiom(
        fn("parent", {fn("bob", {}), fn("ann", {})}),
        {},
        0);

    pud_descent at_main = descender_.descent_root(main_id);
    const expr* first_goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened_first =
        descender_.open_query(at_main, first_goal, parent_tom_bob);
    ASSERT_TRUE(opened_first.has_value());
    const pud_node_id first_child = descender_.close_query(*opened_first);
    children_.store(at_main.node, {first_child});
    call_sites_.store(at_main.node, 0);
    leaves_.set_leaf(first_child);

    std::optional<pud_descent> at_first = descender_.descend(at_main, first_child);
    ASSERT_TRUE(at_first.has_value());
    const expr* second_goal = *at_first->pending_body_goals.find(1);
    std::optional<pud_descent> opened_second =
        descender_.open_query(*at_first, second_goal, parent_bob_ann);
    ASSERT_TRUE(opened_second.has_value());
    const pud_node_id second_child = descender_.close_query(*opened_second);
    children_.store(at_first->node, {second_child});
    call_sites_.store(at_first->node, 1);
    leaves_.unset_leaf(at_first->node);
    leaves_.set_leaf(second_child);

    std::optional<pud_descent> at_second =
        descender_.descend(*descender_.descend(at_main, first_child), second_child);
    ASSERT_TRUE(at_second.has_value());
    EXPECT_TRUE(at_second->pending_body_goals.empty());
    EXPECT_EQ(show(value_of(*at_second, 1)), "ann");
}

TEST_F(PudDescenderIntegrationTest, ParentFactWithWrongGroundNamesDoesNotOpen) {
    const pud_node_id main_id = register_axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)})},
        3);
    const pud_node_id wrong_fact = register_axiom(
        fn("parent", {fn("x", {}), fn("y", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    EXPECT_FALSE(
        descender_.open_query(at_main, fn("parent", {fn("tom", {}), fn("bob", {})}), wrong_fact)
            .has_value());
}

TEST_F(PudDescenderIntegrationTest, PeanoAddZeroBindsResult) {
    const pud_node_id add_zero = register_axiom(
        fn("add", {var(0), fn("z", {}), var(0)}),
        {},
        1);
    pud_descent at_root = descender_.descent_root(add_zero);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, fn("add", {nat(2), fn("z", {}), var(0)}), add_zero);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(show(node_specs_.get(closed)[0].value), show(nat(2)));
}

TEST_F(PudDescenderIntegrationTest, PeanoAddSuccDefersToRecursiveCall) {
    const expr* recursive_goal = fn("add", {var(0), var(1), var(2)});
    const pud_node_id add_succ = register_axiom(
        fn("add", {var(0), fn("s", {var(1)}), fn("s", {var(2)})}),
        {recursive_goal},
        3);
    pud_descent at_root = descender_.descent_root(add_succ);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        fn("add", {fn("a", {}), fn("s", {fn("b", {})}), var(0)}),
        add_succ);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
    ASSERT_EQ(node_goals_.get(closed).size(), 1u);
    EXPECT_EQ(show(node_goals_.get(closed)[0]), "add(a, b, ?3)");
}

TEST_F(PudDescenderIntegrationTest, AppendNilCopiesRightList) {
    const pud_node_id append_nil = register_axiom(
        fn("append", {list({}), var(0), var(0)}),
        {},
        1);
    pud_descent at_root = descender_.descent_root(append_nil);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        fn("append", {list({}), list({fn("a", {})}), var(0)}),
        append_nil);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(show(node_specs_.get(closed)[0].value), "[a]");
}

TEST_F(PudDescenderIntegrationTest, AppendConsPeelsHeadAndDefersTail) {
    const expr* recursive_goal = fn("append", {var(1), var(2), var(3)});
    const pud_node_id append_cons = register_axiom(
        fn("append", {cons(var(0), var(1)), var(2), cons(var(0), var(3))}),
        {recursive_goal},
        4);
    pud_descent at_root = descender_.descent_root(append_cons);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        fn("append", {list({fn("a", {})}), list({fn("b", {})}), var(0)}),
        append_cons);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    ASSERT_EQ(node_specs_.get(closed).size(), 1u);
    EXPECT_EQ(show(node_specs_.get(closed)[0].value), "[a|?4]");
    ASSERT_EQ(node_goals_.get(closed).size(), 1u);
    EXPECT_EQ(show(node_goals_.get(closed)[0]), "append([], [b], ?4)");
}

TEST_F(PudDescenderIntegrationTest, MemberOnNonEmptyListBindsTailPointer) {
    const pud_node_id member_head = register_axiom(
        fn("member", {var(0), cons(var(0), var(1))}),
        {},
        2);
    pud_descent at_root = descender_.descent_root(member_head);
    std::optional<pud_descent> opened = descender_.open_query(
        at_root,
        fn("member", {fn("a", {}), list({fn("a", {}), fn("b", {})})}),
        member_head);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(show(value_of(*opened, 1)), "[b]");
}

TEST_F(PudDescenderIntegrationTest, LengthOfEmptyListIsZero) {
    const pud_node_id length_nil = register_axiom(
        fn("length", {list({}), fn("z", {})}),
        {},
        0);
    pud_descent at_root = descender_.descent_root(length_nil);
    std::optional<pud_descent> opened =
        descender_.open_query(at_root, fn("length", {list({}), fn("z", {})}), length_nil);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = descender_.close_query(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderIntegrationTest, RefutedChildBlocksDescend) {
    const pud_node_id root = register_axiom(fn("main", {}), {}, 0);
    const pud_node_id child = sequencer_.next();
    node_specs_.store(child, {});
    node_goals_.store(child, {fn("g", {})});
    node_var_counts_.store(child, 0);
    children_.store(root, {child});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    refuted_.set_refuted(child);
    EXPECT_FALSE(descender_.descend(descender_.descent_root(root), child).has_value());
}

TEST_F(PudDescenderIntegrationTest, RefutedSiblingDoesNotBlockReachableBranch) {
    const pud_node_id root = register_axiom(fn("main", {}), {}, 0);
    const pud_node_id bad = sequencer_.next();
    const pud_node_id good = sequencer_.next();
    node_specs_.store(bad, {});
    node_goals_.store(bad, {});
    node_var_counts_.store(bad, 0);
    node_specs_.store(good, {});
    node_goals_.store(good, {fn("g", {})});
    node_var_counts_.store(good, 0);
    children_.store(root, std::vector<pud_node_id>{bad, good});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    refuted_.set_refuted(bad);
    std::optional<pud_descent> at_good = descender_.descend(descender_.descent_root(root), good);
    ASSERT_TRUE(at_good.has_value());
    ASSERT_EQ(node_goals_.get(good).size(), 1u);
    EXPECT_EQ(show(node_goals_.get(good)[0]), "g");
}

TEST_F(PudDescenderIntegrationTest, GoalExpansionUpdatesChildrenLeavesAndCallSite) {
    const pud_node_id main_id = register_axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = register_axiom(
        fn("parent", {fn("tom", {}), fn("bob", {})}),
        {},
        0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, fact);
    ASSERT_TRUE(opened.has_value());
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
            pud_specialization{.var_idx = spec_index, .value = fn(label.c_str(), {})});
    }
    const pud_node_id root = register_axiom(fn("main", {}), {}, 0);
    const pud_node_id heavy = sequencer_.next();
    node_specs_.store(heavy, std::move(specs));
    node_goals_.store(heavy, {});
    node_var_counts_.store(heavy, spec_count);
    children_.store(root, {heavy});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    std::optional<pud_descent> at_heavy = descender_.descend(descender_.descent_root(root), heavy);
    ASSERT_TRUE(at_heavy.has_value());
    EXPECT_EQ(at_heavy->lvc, spec_count);
    EXPECT_EQ(show(value_of(*at_heavy, 0)), "v0");
    EXPECT_EQ(show(value_of(*at_heavy, spec_count - 1)), "v" + std::to_string(spec_count - 1));
}

TEST_F(PudDescenderIntegrationTest, MainGoalResolvesPeanoAddZeroOnLargeLiteral) {
    const pud_node_id add_zero = register_axiom(
        fn("add", {var(0), fn("z", {}), var(0)}),
        {},
        1);
    const pud_node_id main_id = register_axiom(
        fn("main", {var(0)}),
        {fn("add", {nat(50), fn("z", {}), var(0)})},
        1);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* goal = *at_main.pending_body_goals.find(0);
    std::optional<pud_descent> opened = descender_.open_query(at_main, goal, add_zero);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id child = descender_.close_query(*opened);
    children_.store(at_main.node, {child});
    call_sites_.store(at_main.node, 0);
    std::optional<pud_descent> at_child = descender_.descend(at_main, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(show(value_of(*at_child, 0)), show(nat(50)));
}

TEST_F(PudDescenderIntegrationTest, ManyGroundFactRootsDisambiguateQueries) {
    std::vector<pud_node_id> fact_roots;
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        fact_roots.push_back(register_axiom(fn("pick", {fn(name.c_str(), {})}), {}, 0));
    }
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        const expr* query = fn("pick", {fn(name.c_str(), {})});
        uint32_t match_count = 0;
        for (const pud_node_id fact_root : fact_roots) {
            pud_descent at_root = descender_.descent_root(fact_root);
            if (descender_.open_query(at_root, query, fact_root).has_value())
                ++match_count;
        }
        EXPECT_EQ(match_count, 1u);
    }
}

TEST_F(PudDescenderIntegrationTest, TwoChildrenUnderSameParentStayIndependent) {
    const pud_node_id root = register_axiom(fn("main", {}), {}, 0);
    const pud_node_id left = sequencer_.next();
    const pud_node_id right = sequencer_.next();
    node_specs_.store(left, {});
    node_goals_.store(left, {fn("left", {})});
    node_var_counts_.store(left, 0);
    node_specs_.store(right, {});
    node_goals_.store(right, {fn("right", {})});
    node_var_counts_.store(right, 0);
    children_.store(root, std::vector<pud_node_id>{left, right});
    call_sites_.store(root, std::numeric_limits<size_t>::max());
    ASSERT_TRUE(descender_.descend(descender_.descent_root(root), left).has_value());
    ASSERT_TRUE(descender_.descend(descender_.descent_root(root), right).has_value());
    EXPECT_THAT(
        show(node_goals_.get(left)[0]),
        Eq(std::string("left")));
    EXPECT_THAT(
        show(node_goals_.get(right)[0]),
        Eq(std::string("right")));
}
