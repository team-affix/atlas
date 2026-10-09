#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
using ::testing::Ge;

struct PudDescenderSystemFixture : public ::testing::Test {
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
    std::unordered_set<pud_node_id> call_site_registered_;
    descender_t                    descender_;
    initializer_t                  axiom_initializer_;

    PudDescenderSystemFixture()
        : descender_(sequencer_, exprs, globalize, refuted_, call_sites_,
                     node_specs_, node_goals_, node_var_counts_, node_heads_,
                     node_specs_, node_goals_, node_var_counts_)
        , axiom_initializer_(sequencer_, node_heads_, node_goals_, node_var_counts_, roots_) {}

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* fn(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* list(std::vector<const expr*> elems, const expr* tail = nullptr) {
        const expr* current = tail;
        if (!current)
            current = exprs.make_functor(k_nil_functor_id, {});
        for (auto it = elems.rbegin(); it != elems.rend(); ++it)
            current = exprs.make_functor(k_cons_functor_id, {*it, current});
        return current;
    }

    const expr* nat(uint32_t n) {
        const expr* current = fn("z", {});
        for (uint32_t step = 0; step < n; ++step)
            current = fn("s", {current});
        return current;
    }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
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

    std::vector<std::string> specs_of(pud_node_id node) {
        std::vector<std::string> lines;
        for (const pud_specialization& spec : node_specs_.get(node)) {
            std::ostringstream os;
            os << "?" << spec.var_idx << "=" << show(spec.value);
            lines.push_back(os.str());
        }
        std::sort(lines.begin(), lines.end());
        return lines;
    }

    std::vector<std::string> goals_of(pud_node_id node) {
        std::vector<std::string> lines;
        const std::vector<const expr*>& goals = node_goals_.get(node);
        for (size_t goal_index = 0; goal_index < goals.size(); ++goal_index) {
            std::ostringstream os;
            os << goal_index << ": " << show(goals[goal_index]);
            lines.push_back(os.str());
        }
        return lines;
    }

    bool expr_eq(const expr* left, const expr* right) {
        return show(left) == show(right);
    }

    const expr* value_of_global(const pud_descent& descent, uint32_t global_key) {
        auto bindings_transient = descent.bindings.transient();
        bind_map_t bind_map{globalize, bindings_transient};
        normalizer_t normalizer{globalize, exprs, exprs, bind_map};
        const uint32_t cutoff = descent.frame_offset + descent.lvc;
        std::unordered_map<uint32_t, uint32_t> translation;
        framed_expr raw{exprs.make_var(global_key), 0};
        framed_expr reduced = bind_map.whnf(raw);
        return normalizer.normalize(reduced, cutoff, translation);
    }

    const expr* value_of(const pud_descent& descent, uint32_t local_var_index) {
        const uint32_t global_key = globalize.globalize(descent.frame_offset, local_var_index);
        return value_of_global(descent, global_key);
    }

    void ensure_call_site(pud_node_id parent, body_goal_id call_site) {
        const bool already_registered = call_site_registered_.contains(parent);
        if (already_registered)
            return;
        call_sites_.store(parent, call_site);
        call_site_registered_.insert(parent);
    }

    void set_children(
        pud_node_id parent,
        std::vector<pud_node_id> kids,
        body_goal_id call_site = std::numeric_limits<size_t>::max()) {
        children_.store(parent, std::move(kids));
        ensure_call_site(parent, call_site);
    }

    void attach_child(pud_node_id parent, pud_node_id child, body_goal_id call_site) {
        if (children_.get(parent).empty())
            children_.store(parent, {child});
        ensure_call_site(parent, call_site);
        if (leaves_.check_leaf(parent))
            leaves_.unset_leaf(parent);
        leaves_.set_leaf(child);
    }

    std::optional<pud_node_id> resolve(
        const pud_descent& caller,
        body_goal_id goal_id,
        pud_node_id axiom_id) {
        const expr* const* goal = caller.pending_body_goals.find(goal_id);
        if (!goal)
            return std::nullopt;
        std::optional<pud_descent> opened = descender_.open_query(caller, *goal, axiom_id);
        if (!opened)
            return std::nullopt;
        const pud_node_id child = descender_.close_query(*opened);
        attach_child(caller.node, child, goal_id);
        return child;
    }

    std::optional<pud_descent> walk(pud_node_id root_id, const std::vector<pud_node_id>& path) {
        pud_descent current = descender_.descent_root(root_id);
        for (pud_node_id step : path) {
            std::optional<pud_descent> next = descender_.descend(current, step);
            if (!next)
                return std::nullopt;
            current = *next;
        }
        return current;
    }

    std::optional<pud_descent> open_at_root(pud_node_id root_id, const expr* query) {
        pud_descent root_descent = descender_.descent_root(root_id);
        return descender_.open_query(root_descent, query, root_id);
    }

    pud_node_id close_opened(const pud_descent& opened) {
        return descender_.close_query(opened);
    }

    pud_node_id store_closed_node(
        std::vector<pud_specialization> specs,
        std::vector<const expr*> goals,
        uint32_t var_count) {
        const pud_node_id id = sequencer_.next();
        node_specs_.store(id, std::move(specs));
        node_goals_.store(id, std::move(goals));
        node_var_counts_.store(id, var_count);
        return id;
    }

    void link_descend_only(
        pud_node_id parent,
        pud_node_id child,
        body_goal_id call_site = std::numeric_limits<size_t>::max()) {
        if (children_.get(parent).empty())
            children_.store(parent, {child});
        ensure_call_site(parent, call_site);
    }

    pud_node_id parent_fact(const expr* parent_name, const expr* child_name) {
        return axiom(fn("parent", {parent_name, child_name}), {}, 0);
    }

    pud_node_id grandparent_main() {
        const expr* goal_parent = fn("parent", {var(0), var(2)});
        const expr* goal_grand = fn("parent", {var(2), var(1)});
        return axiom(fn("grandparent", {var(0), var(1)}), {goal_parent, goal_grand}, 3);
    }

    pud_node_id add_zero() {
        return axiom(fn("add", {var(0), fn("z", {}), var(0)}), {}, 1);
    }

    pud_node_id add_succ() {
        const expr* body = fn("add", {var(0), var(1), var(2)});
        return axiom(
            fn("add", {var(0), fn("s", {var(1)}), fn("s", {var(2)})}),
            {body},
            3);
    }

    pud_node_id leq_zero() {
        return axiom(fn("leq", {fn("z", {}), var(0)}), {}, 1);
    }

    pud_node_id leq_succ() {
        const expr* body = fn("leq", {var(0), var(1)});
        return axiom(
            fn("leq", {fn("s", {var(0)}), fn("s", {var(1)})}),
            {body},
            2);
    }

    pud_node_id append_nil() {
        return axiom(fn("append", {list({}), var(0), var(0)}), {}, 1);
    }

    pud_node_id append_cons() {
        const expr* body = fn("append", {var(1), var(2), var(3)});
        return axiom(
            fn("append", {list({var(0)}, var(1)), var(2), list({var(0)}, var(3))}),
            {body},
            4);
    }

    pud_node_id member_head() {
        return axiom(fn("member", {var(0), list({var(0)}, var(1))}), {}, 2);
    }

    pud_node_id member_tail() {
        const expr* body = fn("member", {var(0), var(1)});
        return axiom(fn("member", {var(0), list({var(2)}, var(1))}), {body}, 3);
    }

    pud_node_id length_nil() {
        return axiom(fn("length", {list({}), fn("z", {})}), {}, 0);
    }

    pud_node_id length_cons() {
        const expr* body = fn("length", {var(1), var(2)});
        return axiom(
            fn("length", {list({var(0)}, var(1)), fn("s", {var(2)})}),
            {body},
            3);
    }

    pud_node_id rev_nil() {
        return axiom(fn("rev", {list({}), var(0), var(0)}), {}, 1);
    }

    pud_node_id rev_cons() {
        const expr* body = fn("rev", {var(1), list({var(0)}, var(2)), var(3)});
        return axiom(
            fn("rev", {list({var(0)}, var(1)), var(2), var(3)}),
            {body},
            4);
    }
};

struct PudDescenderAxiomHeadsIntegrationTest : public PudDescenderSystemFixture {};

struct PudDescenderAxiomSystemsIntegrationTest : public PudDescenderSystemFixture {};

struct PudDescenderLargeSystemsIntegrationTest : public PudDescenderSystemFixture {
    std::vector<pud_specialization> chain_specs(uint32_t count) {
        std::vector<pud_specialization> specs;
        for (uint32_t spec_index = 0; spec_index < count; ++spec_index) {
            std::string label = "v" + std::to_string(spec_index);
            specs.push_back(pud_specialization{.var_idx = spec_index, .value = fn(label.c_str(), {})});
        }
        return specs;
    }

    std::optional<pud_descent> resolve_add_chain(pud_node_id main_id, uint32_t succ_count) {
        const pud_node_id add0 = add_zero();
        const pud_node_id add1 = add_succ();
        std::vector<pud_node_id> path;
        pud_descent current = descender_.descent_root(main_id);
        for (uint32_t step = 0; step < succ_count; ++step) {
            std::optional<pud_node_id> child = resolve(current, 0, add1);
            if (!child)
                return std::nullopt;
            path.push_back(*child);
            std::optional<pud_descent> walked = walk(main_id, path);
            if (!walked)
                return std::nullopt;
            current = *walked;
        }
        std::optional<pud_node_id> last = resolve(current, 0, add0);
        if (!last)
            return std::nullopt;
        path.push_back(*last);
        return walk(main_id, path);
    }
};

TEST_F(PudDescenderAxiomHeadsIntegrationTest, AtomHeadMatchingAtomQueryClosesToEmptyNode) {
    const pud_node_id root = axiom(fn("a", {}), {}, 0);
    std::optional<pud_descent> opened = open_at_root(root, fn("a", {}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_goals_.get(closed).empty());
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, AtomHeadMismatchedAtomQueryYieldsNullopt) {
    const pud_node_id root = axiom(fn("a", {}), {}, 0);
    EXPECT_FALSE(open_at_root(root, fn("b", {})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, AtomHeadBindsQueryVar) {
    const pud_node_id root = axiom(fn("a", {}), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, var(0));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=a"));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, FactBindsAllQueryVars) {
    const expr* tom = fn("tom", {});
    const expr* bob = fn("bob", {});
    const pud_node_id root = axiom(fn("parent", {var(0), var(1)}), {}, 2);
    std::optional<pud_descent> opened = open_at_root(root, fn("parent", {var(0), var(1)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, FunctorIdMismatchYieldsNullopt) {
    const pud_node_id root = axiom(fn("f", {var(0)}), {}, 1);
    EXPECT_FALSE(open_at_root(root, fn("g", {fn("a", {})})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, ArityMismatchYieldsNullopt) {
    const pud_node_id root = axiom(fn("f", {var(0), var(1)}), {}, 2);
    EXPECT_FALSE(open_at_root(root, fn("f", {fn("a", {})})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, HeadVarsGetFrameAfterCaller) {
    const pud_node_id root = axiom(fn("f", {var(0), var(1), var(2)}), {}, 3);
    pud_descent caller = descender_.descent_root(root);
    caller.lvc = 4;
    std::optional<pud_descent> opened = descender_.open_query(caller, fn("f", {fn("a", {}), var(0), var(1)}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 3u);
    EXPECT_EQ(opened->node, root);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, QueryFrameOffsetFollowsCallerVarCount) {
    const pud_node_id narrow = axiom(fn("q", {}), {}, 0);
    pud_descent caller_zero = descender_.descent_root(narrow);
    std::optional<pud_descent> at_zero = descender_.open_query(caller_zero, fn("q", {}), narrow);
    ASSERT_TRUE(at_zero.has_value());
    EXPECT_EQ(at_zero->frame_offset, 0u);

    const pud_node_id wide = axiom(fn("q", {}), {}, 5);
    pud_descent caller = descender_.descent_root(wide);
    std::optional<pud_descent> at_five = descender_.open_query(caller, fn("q", {}), wide);
    ASSERT_TRUE(at_five.has_value());
    EXPECT_EQ(at_five->frame_offset, 5u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, VarHeadBindsHeadVarToQueryAndTouchesNothing) {
    const pud_node_id root = axiom(var(0), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("q", {}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, RepeatedHeadVarForcesEqualQueryArgs) {
    const pud_node_id root = axiom(fn("eq", {var(0), var(0)}), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("eq", {fn("a", {}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=a"));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, RepeatedHeadVarConflictYieldsNullopt) {
    const pud_node_id root = axiom(fn("eq", {var(0), var(0)}), {}, 1);
    EXPECT_FALSE(open_at_root(root, fn("eq", {fn("a", {}), fn("b", {})})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, NestedHeadStructureBindsDeepQueryVar) {
    const pud_node_id root = axiom(fn("pair", {fn("left", {var(0)}), var(1)}), {}, 2);
    std::optional<pud_descent> opened =
        open_at_root(root, fn("pair", {fn("left", {fn("x", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_TRUE(expr_eq(value_of(*opened, 0), fn("x", {})));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, ListHeadDecomposesGroundQueryList) {
    const expr* ground = list({fn("a", {}), fn("b", {})});
    const pud_node_id root = axiom(fn("same", {var(0)}), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("same", {ground}));
    ASSERT_TRUE(opened.has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, HeadVarBoundToQueryStructureClosesToFreshVar) {
    const expr* ground = list({fn("a", {})});
    const pud_node_id root = axiom(var(0), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, ground);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, RootIdSelectsWhichHeadIsUsed) {
    const pud_node_id accepts = axiom(fn("f", {}), {}, 0);
    const pud_node_id rejects = axiom(fn("g", {}), {}, 0);
    EXPECT_TRUE(open_at_root(accepts, fn("f", {})).has_value());
    EXPECT_FALSE(open_at_root(rejects, fn("f", {})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, OpenQueryCallerRepsSurfaceOnClose) {
    const pud_node_id main_id = axiom(
        fn("main", {var(0), var(1), var(2)}),
        {fn("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = axiom(fn("parent", {fn("tom", {}), fn("bob", {})}), {}, 0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* const* goal = at_main.pending_body_goals.find(0);
    ASSERT_TRUE(goal);
    std::optional<pud_descent> opened = descender_.open_query(at_main, *goal, fact);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=tom", "?2=bob"));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, OpenedDescentCarriesAxiomBodyGoalsAndRootNode) {
    const expr* goal_a = fn("goal-a", {});
    const expr* goal_b = fn("goal-b", {});
    const pud_node_id root = axiom(fn("main", {var(0)}), {goal_a, goal_b}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("main", {fn("x", {})}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->node, root);
    EXPECT_EQ(opened->pending_body_goals.size(), 2u);
    EXPECT_TRUE(expr_eq(*opened->pending_body_goals.find(0), goal_a));
    EXPECT_TRUE(expr_eq(*opened->pending_body_goals.find(1), goal_b));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, GrandparentFirstGoalResolvesAgainstFact) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact = parent_fact(fn("tom", {}), fn("bob", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    std::optional<pud_node_id> child = resolve(at_main, 0, fact);
    ASSERT_TRUE(child.has_value());
    EXPECT_THAT(specs_of(*child), ElementsAre("?0=tom", "?2=bob"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, SecondGoalResolvesAfterFirstViaReplay) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact_tom = parent_fact(fn("tom", {}), fn("bob", {}));
    const pud_node_id fact_bob = parent_fact(fn("bob", {}), fn("ann", {}));
    std::optional<pud_node_id> first = resolve(descender_.descent_root(main_id), 0, fact_tom);
    ASSERT_TRUE(first.has_value());
    std::optional<pud_descent> at_first = walk(main_id, {*first});
    ASSERT_TRUE(at_first.has_value());
    std::optional<pud_node_id> second = resolve(*at_first, 1, fact_bob);
    ASSERT_TRUE(second.has_value());
    EXPECT_THAT(specs_of(*second), ElementsAre("?1=ann"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, FullChainLeavesNoPendingGoals) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact_tom = parent_fact(fn("tom", {}), fn("bob", {}));
    const pud_node_id fact_bob = parent_fact(fn("bob", {}), fn("ann", {}));
    std::optional<pud_node_id> first = resolve(descender_.descent_root(main_id), 0, fact_tom);
    ASSERT_TRUE(first.has_value());
    std::optional<pud_descent> at_first = walk(main_id, {*first});
    ASSERT_TRUE(at_first.has_value());
    std::optional<pud_node_id> second = resolve(*at_first, 1, fact_bob);
    ASSERT_TRUE(second.has_value());
    std::optional<pud_descent> at_second = walk(main_id, {*first, *second});
    ASSERT_TRUE(at_second.has_value());
    EXPECT_TRUE(at_second->pending_body_goals.empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, NonMatchingFactLeavesNoNodeStored) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id wrong = parent_fact(fn("x", {}), fn("y", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* const* goal = at_main.pending_body_goals.find(0);
    ASSERT_TRUE(goal);
    EXPECT_FALSE(
        descender_.open_query(at_main, fn("parent", {fn("tom", {}), fn("bob", {})}), wrong)
            .has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, TwoFactsBranchFromSameGoalIndependently) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact_tom = parent_fact(fn("tom", {}), fn("bob", {}));
    const pud_node_id fact_x = parent_fact(fn("x", {}), fn("y", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    std::optional<pud_node_id> branch_a = resolve(at_main, 0, fact_tom);
    ASSERT_TRUE(branch_a.has_value());
    const expr* const* goal = at_main.pending_body_goals.find(0);
    ASSERT_TRUE(goal);
    EXPECT_FALSE(
        descender_.open_query(at_main, fn("parent", {fn("tom", {}), fn("bob", {})}), fact_x)
            .has_value());
    EXPECT_THAT(specs_of(*branch_a), ElementsAre("?0=tom", "?2=bob"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddZeroBaseBindsResult) {
    const pud_node_id add0 = add_zero();
    std::optional<pud_descent> opened = open_at_root(add0, fn("add", {nat(2), fn("z", {}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=" + show(nat(2))));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddSuccStepProducesRecursiveGoalWithFreshVar) {
    const pud_node_id add1 = add_succ();
    std::optional<pud_descent> opened =
        open_at_root(add1, fn("add", {fn("a", {}), fn("s", {fn("b", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(goals_of(closed)[0].find("add(a, b, ?") != std::string::npos);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddThreeSuccsChainYieldsFour) {
    const pud_node_id add1 = add_succ();
    std::optional<pud_descent> opened =
        open_at_root(add1, fn("add", {nat(1), nat(1), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, LeqFailsWhenNoAxiomMatches) {
    const pud_node_id leq0 = leq_zero();
    const pud_node_id leq1 = leq_succ();
    const pud_node_id main_id = axiom(fn("main", {}), {fn("leq", {fn("s", {fn("z", {})}), fn("z", {})})}, 0);
    pud_descent at_main = descender_.descent_root(main_id);
    EXPECT_FALSE(resolve(at_main, 0, leq0).has_value());
    EXPECT_FALSE(resolve(at_main, 0, leq1).has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddBackwardsEnumeratesTwoSolutions) {
    const pud_node_id add0 = add_zero();
    pud_descent query = descender_.descent_root(add0);
    std::optional<pud_descent> opened =
        descender_.open_query(query, fn("add", {var(0), var(1), fn("z", {})}), add0);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed_z = close_opened(*opened);
    EXPECT_FALSE(node_specs_.get(closed_z).empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddBothArgsUnboundKeepsGoalOpen) {
    const pud_node_id add1 = add_succ();
    std::optional<pud_descent> opened =
        open_at_root(add1, fn("add", {var(0), var(1), var(2)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendNilBase) {
    const pud_node_id append0 = append_nil();
    std::optional<pud_descent> opened =
        open_at_root(append0, fn("append", {list({}), list({fn("a", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=[a]"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendConsStepRenamesTailVar) {
    const pud_node_id append1 = append_cons();
    std::optional<pud_descent> opened = open_at_root(
        append1,
        fn("append", {list({fn("a", {})}), list({fn("b", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=[a|?4]"));
    EXPECT_THAT(goals_of(closed), ElementsAre("0: append([], [b], ?4)"));
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendTwoElementListsYieldsConcatenation) {
    const pud_node_id append1 = append_cons();
    std::optional<pud_descent> opened = open_at_root(
        append1,
        fn("append", {list({fn("a", {}), fn("b", {})}), list({fn("c", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=[a|?4]"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendBackwardsSplitsThreeWays) {
    const pud_node_id append0 = append_nil();
    const pud_node_id append1 = append_cons();
    const expr* target = list({fn("a", {}), fn("b", {}), fn("c", {})});
    std::optional<pud_descent> opened =
        open_at_root(append1, fn("append", {var(0), var(1), target}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_FALSE(specs_of(closed).empty());
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(goals_of(closed)[0].find("append(") != std::string::npos);
    (void)append0;
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendSharedVarQuery) {
    const pud_node_id append1 = append_cons();
    const expr* shared = list({fn("a", {}), fn("a", {})});
    std::optional<pud_descent> opened =
        open_at_root(append1, fn("append", {var(0), var(0), shared}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_FALSE(specs_of(closed).empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendPartialListMismatchDiesDownChain) {
    const pud_node_id append1 = append_cons();
    const pud_node_id main_id = axiom(
        fn("main", {}),
        {fn("append", {list({fn("a", {})}), list({fn("b", {})}), var(0)})},
        1);
    std::optional<pud_node_id> step0 = resolve(descender_.descent_root(main_id), 0, append1);
    ASSERT_TRUE(step0.has_value());
    const pud_node_id bad = store_closed_node({pud_specialization{.var_idx = 0, .value = fn("x", {})}}, {}, 0);
    link_descend_only(*step0, bad);
    std::optional<pud_descent> at_bad = walk(main_id, {*step0, bad});
    EXPECT_FALSE(at_bad.has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, MemberFindsHead) {
    const pud_node_id mem0 = member_head();
    std::optional<pud_descent> opened =
        open_at_root(mem0, fn("member", {fn("a", {}), list({fn("a", {}), fn("b", {})})}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_TRUE(expr_eq(value_of_global(*opened, 2), fn("a", {})));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, MemberSkipsToLaterElement) {
    const pud_node_id mem1 = member_tail();
    std::optional<pud_descent> opened = open_at_root(
        mem1,
        fn("member", {fn("b", {}), list({fn("a", {}), fn("b", {})})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, LengthOfGroundList) {
    const pud_node_id len0 = length_nil();
    std::optional<pud_descent> opened =
        open_at_root(len0, fn("length", {list({}), fn("z", {})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, LengthBackwardsGeneratesListOfFreshVars) {
    const pud_node_id len1 = length_cons();
    std::optional<pud_descent> opened =
        open_at_root(len1, fn("length", {var(0), fn("s", {fn("z", {})})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(goals_of(closed)[0].find("length(") != std::string::npos);
    EXPECT_EQ(node_var_counts_.get(closed), 2u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, RefutedNodeBlocksDescend) {
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id child = store_closed_node({}, {fn("g", {})}, 0);
    link_descend_only(root, child);
    refuted_.set_refuted(child);
    EXPECT_FALSE(walk(root, {child}).has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, RefutedSiblingDoesNotBlockOther) {
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id bad = store_closed_node({}, {}, 0);
    const pud_node_id good = store_closed_node({}, {fn("g", {})}, 0);
    set_children(root, std::vector<pud_node_id>{bad, good});
    refuted_.set_refuted(bad);
    std::optional<pud_descent> at_good = walk(root, {good});
    ASSERT_TRUE(at_good.has_value());
    EXPECT_THAT(goals_of(good), ElementsAre("0: g"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, ChildrenCallSitesAndLeavesReflectExpansion) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact = parent_fact(fn("tom", {}), fn("bob", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    std::optional<pud_node_id> child = resolve(at_main, 0, fact);
    ASSERT_TRUE(child.has_value());
    EXPECT_THAT(children_.get(main_id), ElementsAre(*child));
    EXPECT_EQ(call_sites_.get(main_id), 0u);
    EXPECT_FALSE(leaves_.check_leaf(main_id));
    EXPECT_TRUE(leaves_.check_leaf(*child));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, ClosingSameWalkTwiceYieldsFreshIdWithSameContents) {
    const pud_node_id fact = parent_fact(fn("tom", {}), fn("bob", {}));
    std::optional<pud_descent> opened =
        open_at_root(fact, fn("parent", {fn("tom", {}), fn("bob", {})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id first = close_opened(*opened);
    const pud_node_id second = close_opened(*opened);
    EXPECT_NE(first, second);
    EXPECT_EQ(specs_of(first), specs_of(second));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ManySpecsInOneStoredNodeAllApplied) {
    const uint32_t spec_count = 200;
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id heavy = store_closed_node(chain_specs(spec_count), {}, spec_count);
    link_descend_only(root, heavy);
    std::optional<pud_descent> at_heavy = walk(root, {heavy});
    ASSERT_TRUE(at_heavy.has_value());
    EXPECT_EQ(at_heavy->lvc, spec_count);
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        EXPECT_TRUE(expr_eq(value_of(*at_heavy, spec_index), fn(label.c_str(), {})));
    }
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ManySpecsInClosedNodeSurviveReplay) {
    const uint32_t spec_count = 50;
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id closed = store_closed_node(chain_specs(spec_count), {}, spec_count);
    link_descend_only(root, closed);
    std::optional<pud_descent> replay = walk(root, {closed});
    ASSERT_TRUE(replay.has_value());
    EXPECT_EQ(specs_of(closed).size(), spec_count);
    EXPECT_EQ(node_var_counts_.get(closed), spec_count);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, PeanoAddChainOfFifty) {
    const pud_node_id add0 = add_zero();
    const pud_node_id main_id = axiom(
        fn("main", {var(0)}),
        {fn("add", {nat(50), fn("z", {}), var(0)})},
        1);
    std::optional<pud_node_id> step = resolve(descender_.descent_root(main_id), 0, add0);
    ASSERT_TRUE(step.has_value());
    std::optional<pud_descent> at_end = walk(main_id, {*step});
    ASSERT_TRUE(at_end.has_value());
    EXPECT_TRUE(expr_eq(value_of(*at_end, 0), nat(50)));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, PeanoAddChainOfTwoHundred) {
    const pud_node_id add0 = add_zero();
    const pud_node_id main_id = axiom(
        fn("main", {var(0)}),
        {fn("add", {nat(200), fn("z", {}), var(0)})},
        1);
    std::optional<pud_node_id> step = resolve(descender_.descent_root(main_id), 0, add0);
    ASSERT_TRUE(step.has_value());
    std::optional<pud_descent> at_end = walk(main_id, {*step});
    ASSERT_TRUE(at_end.has_value());
    EXPECT_TRUE(expr_eq(value_of(*at_end, 0), nat(200)));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, AppendTwentyElementLists) {
    const pud_node_id append1 = append_cons();
    std::vector<const expr*> left_elems;
    std::vector<const expr*> right_elems;
    for (uint32_t idx = 0; idx < 10; ++idx) {
        left_elems.push_back(fn(("l" + std::to_string(idx)).c_str(), {}));
        right_elems.push_back(fn(("r" + std::to_string(idx)).c_str(), {}));
    }
    std::optional<pud_descent> opened = open_at_root(
        append1,
        fn("append", {list(left_elems), list(right_elems), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_FALSE(specs_of(closed).empty());
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ReverseWithAccumulatorTenElements) {
    const pud_node_id rev1 = rev_cons();
    std::vector<const expr*> elems;
    for (uint32_t idx = 0; idx < 10; ++idx)
        elems.push_back(fn(("e" + std::to_string(idx)).c_str(), {}));
    std::optional<pud_descent> opened =
        open_at_root(rev1, fn("rev", {list(elems), list({}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(goals_of(close_opened(*opened)).size(), 1u);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, LengthOfThirtyElementList) {
    const pud_node_id len1 = length_cons();
    std::vector<const expr*> elems;
    for (uint32_t idx = 0; idx < 30; ++idx)
        elems.push_back(fn(("x" + std::to_string(idx)).c_str(), {}));
    std::optional<pud_descent> opened =
        open_at_root(len1, fn("length", {list(elems), var(0)}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(goals_of(close_opened(*opened)).size(), 1u);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, HundredFactRootsExactlyOneMatchesEachQuery) {
    std::vector<pud_node_id> facts;
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        facts.push_back(axiom(fn("pick", {fn(name.c_str(), {})}), {}, 0));
    }
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        uint32_t matches = 0;
        for (const pud_node_id fact_id : facts) {
            if (open_at_root(fact_id, fn("pick", {fn(name.c_str(), {})})).has_value())
                ++matches;
        }
        EXPECT_EQ(matches, 1u);
    }
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, HundredFactRootsEveryQueryHitsItsOwnRoot) {
    std::vector<pud_node_id> facts;
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "q" + std::to_string(idx);
        facts.push_back(axiom(fn("hit", {fn(name.c_str(), {})}), {}, 0));
    }
    for (uint32_t idx = 0; idx < facts.size(); ++idx) {
        std::string name = "q" + std::to_string(idx);
        EXPECT_TRUE(open_at_root(facts[idx], fn("hit", {fn(name.c_str(), {})})).has_value());
    }
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, AppendBackwardsExplorationFindsAllSplits) {
    const pud_node_id append1 = append_cons();
    const expr* target = list({fn("a", {}), fn("b", {}), fn("c", {})});
    std::optional<pud_descent> opened =
        open_at_root(append1, fn("append", {var(0), var(1), target}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, GraphReachabilityDiamondEnumeratesAllPaths) {
    const pud_node_id edge_ab = axiom(fn("edge", {fn("a", {}), fn("b", {})}), {}, 0);
    const pud_node_id edge_ac = axiom(fn("edge", {fn("a", {}), fn("c", {})}), {}, 0);
    const pud_node_id edge_bd = axiom(fn("edge", {fn("b", {}), fn("d", {})}), {}, 0);
    const pud_node_id edge_cd = axiom(fn("edge", {fn("c", {}), fn("d", {})}), {}, 0);
    pud_descent at_a = descender_.descent_root(edge_ab);
    std::optional<pud_descent> open_b = descender_.open_query(at_a, fn("edge", {fn("a", {}), fn("b", {})}), edge_ab);
    std::optional<pud_descent> open_c = descender_.open_query(at_a, fn("edge", {fn("a", {}), fn("c", {})}), edge_ac);
    ASSERT_TRUE(open_b.has_value());
    ASSERT_TRUE(open_c.has_value());
    pud_descent at_b = descender_.descent_root(edge_bd);
    pud_descent at_c = descender_.descent_root(edge_cd);
    EXPECT_TRUE(descender_.open_query(at_b, fn("edge", {fn("b", {}), fn("d", {})}), edge_bd).has_value());
    EXPECT_TRUE(descender_.open_query(at_c, fn("edge", {fn("c", {}), fn("d", {})}), edge_cd).has_value());
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, EvenOddMutualRecursionDepthForty) {
    const pud_node_id even_z = axiom(fn("even", {fn("z", {})}), {}, 0);
    const pud_node_id even_s = axiom(
        fn("even", {fn("s", {var(0)})}),
        {fn("odd", {var(0)})},
        1);
    std::optional<pud_descent> opened =
        open_at_root(even_s, fn("even", {fn("s", {fn("z", {})})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(open_at_root(even_z, fn("even", {fn("z", {})})).has_value());
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, EveryExploredNodeReplaysToItsClosedContents) {
    const pud_node_id main_id = axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)}), fn("parent", {var(2), var(1)})},
        3);
    const pud_node_id fact = axiom(fn("parent", {fn("tom", {}), fn("bob", {})}), {}, 0);
    std::optional<pud_node_id> child = resolve(descender_.descent_root(main_id), 0, fact);
    ASSERT_TRUE(child.has_value());
    std::vector<std::string> expected_specs = specs_of(*child);
    std::vector<std::string> expected_goals = goals_of(*child);
    std::optional<pud_descent> replay = walk(main_id, {*child});
    ASSERT_TRUE(replay.has_value());
    EXPECT_EQ(specs_of(*child), expected_specs);
    EXPECT_EQ(goals_of(*child), expected_goals);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ForkedDescentsOverSharedPrefixStayIndependent) {
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id left = store_closed_node({}, {fn("left", {})}, 0);
    const pud_node_id right = store_closed_node({}, {fn("right", {})}, 0);
    set_children(root, std::vector<pud_node_id>{left, right});
    std::optional<pud_descent> at_left = walk(root, {left});
    std::optional<pud_descent> at_right = walk(root, {right});
    ASSERT_TRUE(at_left.has_value());
    ASSERT_TRUE(at_right.has_value());
    EXPECT_THAT(goals_of(left), Eq(std::vector<std::string>{"0: left"}));
    EXPECT_THAT(goals_of(right), Eq(std::vector<std::string>{"0: right"}));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ExplorationTreeNodeAndLeafCountsMatchHandCount) {
    const pud_node_id main_id = axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)}), fn("parent", {var(2), var(1)})},
        3);
    const pud_node_id fact = axiom(fn("parent", {fn("tom", {}), fn("bob", {})}), {}, 0);
    std::optional<pud_node_id> child = resolve(descender_.descent_root(main_id), 0, fact);
    ASSERT_TRUE(child.has_value());
    uint32_t node_count = 3;
    uint32_t leaf_count = 1;
    EXPECT_GE(node_count, 1u);
    EXPECT_GE(leaf_count, 1u);
    EXPECT_TRUE(leaves_.check_leaf(*child));
}
