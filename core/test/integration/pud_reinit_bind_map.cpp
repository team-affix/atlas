// Integration: unify_head path-replays ancestor unifications into FPA under a nested interval.

#include <gtest/gtest.h>
#include <optional>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_node_added_unifications.hpp"
#include "infrastructure/pud_node_base_interval.hpp"
#include "infrastructure/pud_node_children.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_query.hpp"

using unify_head_t = pud_unify_head<
    order_maintenance, pud_node_children, pud_node_added_unifications,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, expr_pool>;

struct PudReinitBindMapIntegrationTest : public ::testing::Test {
    PudReinitBindMapIntegrationTest()
        : unify_head_(om_, children_, added_unifications_, fpa_, fpa_, glob_, exprs_, exprs_) {}

    const pud_rule_id* add_axiom(size_t entry_idx,
                                 std::vector<pud_added_unification> unifs) {
        const pud_rule_id* id = pool_.make_axiom(entry_idx);
        added_unifications_.store(id, std::move(unifs));
        children_.add_root(id);
        base_interval_.store(id, om_.allocate_root());
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

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    pud_node_added_unifications added_unifications_;
    pud_node_children children_;
    pud_node_base_interval base_interval_;
    unify_head_t unify_head_;
};

TEST_F(PudReinitBindMapIntegrationTest, PathReplayIsVisibleInQueryNodeInterval) {
    const expr* head = exprs_.make_functor(3, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, head}});
    const pud_rule_id* child = add_inference(axiom, 0, axiom, {});
    children_.link_children(axiom, {child});
    base_interval_.store(child, om_.allocate_child_of(base_interval_.get(axiom)));

    pud_query query{
        om_.allocate_child_of(base_interval_.get(child)),
        head,
        {pud_candidate_search_context{child, {}}},
        1};
    std::vector<uint32_t> touched_reps;
    om_interval env = query.interval;
    EXPECT_TRUE(unify_head_.unify_callee(query, child, touched_reps, env));
    const std::optional<framed_expr> found = fpa_.query(env.open, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->skeleton, head);
}

TEST_F(PudReinitBindMapIntegrationTest, UnifyCalleeBindingsVisibleInEnvInterval) {
    const expr* head = exprs_.make_functor(4, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, head}});
    pud_query query{
        om_.allocate_child_of(base_interval_.get(axiom)),
        head,
        {},
        1};
    std::vector<uint32_t> touched_reps;
    om_interval env = query.interval;
    EXPECT_TRUE(unify_head_.unify_callee(query, axiom, touched_reps, env));
    const std::optional<framed_expr> found = fpa_.query(env.open, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->skeleton, head);
}

TEST_F(PudReinitBindMapIntegrationTest, EnsureFailedMidPathDoesNotBindLaterNode) {
    const expr* p = exprs_.make_functor(5, {});
    const expr* q = exprs_.make_functor(6, {});
    const expr* r = exprs_.make_functor(7, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, p}});
    const pud_rule_id* mid = add_inference(axiom, 0, axiom, {{0, q}});
    children_.link_children(axiom, {mid});
    base_interval_.store(mid, om_.allocate_child_of(base_interval_.get(axiom)));
    const pud_rule_id* leaf = add_inference(mid, 0, axiom, {{1, r}});
    children_.link_children(mid, {leaf});
    base_interval_.store(leaf, om_.allocate_child_of(base_interval_.get(mid)));
    pud_query query{
        om_.allocate_child_of(base_interval_.get(leaf)),
        p,
        {pud_candidate_search_context{leaf, {}}},
        1};
    unify_head_.unify_head(query, leaf);
    EXPECT_FALSE(fpa_.query(query.interval.open, 1).has_value());
}
