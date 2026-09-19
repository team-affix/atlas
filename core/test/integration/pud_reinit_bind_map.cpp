// Integration: reinit path-replays ancestor unifications into FPA under a nested interval.

#include <gtest/gtest.h>
#include <optional>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud.hpp"
#include "infrastructure/pud_query_reinitializer.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"

using forest_t = pud<pud_rule_id_pool, pud_rule_id_pool, order_maintenance, order_maintenance>;
using unify_head_t = pud_unify_head<
    order_maintenance, forest_t, forest_t,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, expr_pool>;
using reinit_t = pud_query_reinitializer<
    forest_t, forest_t, fully_persistent_array, unify_head_t, unify_head_t>;

struct PudReinitBindMapIntegrationTest : public ::testing::Test {
    PudReinitBindMapIntegrationTest()
        : dummy_open_(0)
        , dummy_close_(1)
        , dummy_{om_label(&dummy_open_), om_label(&dummy_close_)}
        , forest_(pool_, pool_, om_, om_)
        , unify_head_(om_, forest_, forest_, fpa_, fpa_, glob_, exprs_, exprs_)
        , reinit_(forest_, forest_, fpa_, unify_head_, unify_head_) {}

    uint64_t dummy_open_;
    uint64_t dummy_close_;
    om_interval dummy_;
    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    forest_t forest_;
    unify_head_t unify_head_;
    reinit_t reinit_;
};

TEST_F(PudReinitBindMapIntegrationTest, PathReplayIsVisibleInChildQueryInterval) {
    const expr* head = exprs_.make_functor(3, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, pud_db_node{dummy_, {{0, head}}, {}, 1});
    const pud_rule_id* child = forest_.add_inference(
        axiom, 0, axiom, pud_db_node{dummy_, {}, {}, 1});
    forest_.link_child(axiom, child);

    pud_query query{
        om_.allocate_child_of(forest_.get_node(child).interval),
        head,
        {pud_candidate_search_context{child, {}}}};
    unify_head_.bind_query(query, 1);
    reinit_.reinit(query, 1);

    const std::optional<framed_expr> found = fpa_.query(query.interval.open, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->skeleton, head);
}
