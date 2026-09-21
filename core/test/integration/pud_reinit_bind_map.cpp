// Integration: DFS descend stores a node interval; unfold overwrite replaces it.

#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_node_added_unifications.hpp"
#include "infrastructure/pud_node_children.hpp"
#include "infrastructure/pud_node_interval.hpp"
#include "infrastructure/pud_node_parent.hpp"
#include "infrastructure/pud_node_touched_reps.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using witness_search_t = pud_witness_search<
    pud_node_children, pud_node_parent, pud_rule_id_pool,
    pud_node_interval, pud_node_interval, pud_node_interval,
    order_maintenance, pud_node_added_unifications,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, pud_node_touched_reps>;

struct PudReinitBindMapIntegrationTest : public ::testing::Test {
    PudReinitBindMapIntegrationTest()
        : witness_(children_, parent_, pool_,
                   node_interval_, node_interval_, node_interval_,
                   om_, added_unifications_,
                   fpa_, fpa_, glob_, exprs_, touched_reps_) {}

    const pud_rule_id* add_axiom(size_t entry_idx,
                                 std::vector<pud_added_unification> unifs) {
        const pud_rule_id* id = pool_.make_axiom(entry_idx);
        added_unifications_.store(id, std::move(unifs));
        parent_.store(id, nullptr);
        node_interval_.store(id, om_.allocate_root());
        return id;
    }

    const pud_rule_id* add_inference(const pud_rule_id* caller,
                                     size_t call_site,
                                     const pud_rule_id* callee,
                                     std::vector<pud_added_unification> unifs) {
        const pud_rule_id* id = pool_.make_inference(caller, call_site, callee);
        added_unifications_.store(id, std::move(unifs));
        return id;
    }

    void store_children_of(const pud_rule_id* parent,
                           std::initializer_list<const pud_rule_id*> kids) {
        children_.store(parent, std::set<const pud_rule_id*>{kids});
        for (const pud_rule_id* child : kids)
            parent_.store(child, parent);
    }

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    pud_node_added_unifications added_unifications_;
    pud_node_children children_;
    pud_node_parent parent_;
    pud_node_interval node_interval_;
    pud_node_touched_reps touched_reps_;
    witness_search_t witness_;
};

TEST_F(PudReinitBindMapIntegrationTest, DescendStoresIntervalAndRecordsHead) {
    const expr* head = exprs_.make_functor(3, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, head}});
    const pud_rule_id* child = add_inference(axiom, 0, axiom, {});
    store_children_of(axiom, {child});

    pud_witness_search_context ctx{axiom, 0, head, 1, axiom, axiom};
    witness_.resume(ctx);
    EXPECT_EQ(ctx.current, child);
    const pud_rule_id* key = pool_.make_inference(axiom, 0, child);
    const om_interval stored = node_interval_.get(key);
    const std::optional<framed_expr> found = fpa_.query(stored.open, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->skeleton, head);
}

TEST_F(PudReinitBindMapIntegrationTest, OverwriteAfterUnfoldDropsSearchBinds) {
    const expr* head = exprs_.make_functor(4, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, head}});
    pud_witness_search_context ctx{axiom, 0, head, 1, axiom, axiom};
    witness_.resume(ctx);
    EXPECT_EQ(ctx.current, axiom);
    const pud_rule_id* key = pool_.make_inference(axiom, 0, axiom);
    const om_interval search_interval = node_interval_.get(key);
    ASSERT_TRUE(fpa_.query(search_interval.open, 0).has_value());

    const om_interval clean = om_.allocate_child_of(node_interval_.get(axiom));
    node_interval_.store(key, clean);
    EXPECT_EQ(node_interval_.get(key).open.rank_ptr(), clean.open.rank_ptr());
    EXPECT_FALSE(fpa_.query(clean.open, 0).has_value());
}

TEST_F(PudReinitBindMapIntegrationTest, FailedMidPathDoesNotEnterLaterNode) {
    const expr* p = exprs_.make_functor(5, {});
    const expr* q = exprs_.make_functor(6, {});
    const expr* r = exprs_.make_functor(7, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, p}});
    const pud_rule_id* mid = add_inference(axiom, 0, axiom, {{0, q}});
    store_children_of(axiom, {mid});
    const pud_rule_id* leaf = add_inference(mid, 0, axiom, {{1, r}});
    store_children_of(mid, {leaf});
    pud_witness_search_context ctx{axiom, 0, p, 1, axiom, axiom};
    witness_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    const pud_rule_id* leaf_key = pool_.make_inference(axiom, 0, leaf);
    EXPECT_FALSE(node_interval_.contains(leaf_key));
}
