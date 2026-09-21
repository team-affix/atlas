// Integration: reinit path-replays ancestor unifications into FPA under a nested interval.

#include <gtest/gtest.h>
#include <optional>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_forest.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_query.hpp"

using forest_t = pud_forest<pud_rule_id_pool, pud_rule_id_pool, order_maintenance, order_maintenance>;
using unify_head_t = pud_unify_head<
    order_maintenance, forest_t, forest_t,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, expr_pool>;

struct PudReinitBindMapIntegrationTest : public ::testing::Test {
    PudReinitBindMapIntegrationTest()
        : forest_(pool_, pool_, om_, om_)
        , unify_head_(om_, forest_, forest_, fpa_, fpa_, glob_, exprs_, exprs_) {}

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    forest_t forest_;
    unify_head_t unify_head_;
};

TEST_F(PudReinitBindMapIntegrationTest, PathReplayIsVisibleInChildQueryInterval) {
    const expr* head = exprs_.make_functor(3, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, head}}, {}, 1);
    const pud_rule_id* child = forest_.add_inference(axiom, 0, axiom, {}, {}, 1);
    forest_.link_children(axiom, {child});

    pud_query query{
        om_.allocate_child_of(forest_.get_node(child).interval),
        head,
        {pud_candidate_search_context{child, {}}},
        1};
    unify_head_.reinit(query);

    const std::optional<framed_expr> found = fpa_.query(query.interval.open, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->skeleton, head);
}

TEST_F(PudReinitBindMapIntegrationTest, UnifyCalleeBindingsVisibleInEnvInterval) {
    const expr* head = exprs_.make_functor(4, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, head}}, {}, 1);
    pud_query query{
        om_.allocate_child_of(forest_.get_node(axiom).interval),
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

TEST_F(PudReinitBindMapIntegrationTest, ReinitFailedMidPathDoesNotBindLaterNode) {
    const expr* p = exprs_.make_functor(5, {});
    const expr* q = exprs_.make_functor(6, {});
    const expr* r = exprs_.make_functor(7, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, p}}, {p}, 1);
    const pud_rule_id* mid = forest_.add_inference(axiom, 0, axiom, {{0, q}}, {p}, 1);
    forest_.link_children(axiom, {mid});
    const pud_rule_id* leaf = forest_.add_inference(mid, 0, axiom, {{1, r}}, {}, 1);
    forest_.link_children(mid, {leaf});
    pud_query query{
        om_.allocate_child_of(forest_.get_node(leaf).interval),
        p,
        {pud_candidate_search_context{leaf, {}}},
        1};
    unify_head_.reinit(query);
    EXPECT_FALSE(fpa_.query(query.interval.open, 1).has_value());
}
