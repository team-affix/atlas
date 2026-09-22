// Integration: witness search + candidate search on the per-node tables.

#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_candidate_search.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_unifications.hpp"
#include "infrastructure/pud_node_children.hpp"
#include "infrastructure/pud_node_interval.hpp"
#include "infrastructure/pud_node_lvc.hpp"
#include "infrastructure/pud_node_parent.hpp"
#include "infrastructure/pud_node_added_touched_caller_reps.hpp"
#include "infrastructure/pud_query_starter.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using witness_search_t = pud_witness_search<
    pud_node_children, pud_node_parent, pud_rule_id_pool,
    pud_node_interval, pud_node_interval, pud_node_interval,
    order_maintenance, pud_node_added_unifications,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, pud_node_added_touched_caller_reps>;
using candidate_search_t = pud_candidate_search<
    witness_search_t, witness_search_t, pud_node_children, pud_node_parent,
    pud_node_added_body_goals>;
using query_starter_t = pud_query_starter<
    pud_rule_id_pool, pud_node_interval, order_maintenance, pud_node_interval,
    globalizer, fully_persistent_array, fully_persistent_array>;

struct PudForestSearchIntegrationTest : public ::testing::Test {
    PudForestSearchIntegrationTest()
        : witness_(children_, parent_, pool_,
                   node_interval_, node_interval_, node_interval_,
                   om_, added_unifications_,
                   fpa_, fpa_, glob_, exprs_, added_caller_reps_)
        , candidate_(witness_, witness_, children_, parent_, added_body_goals_)
        , starter_(pool_, node_interval_, om_, node_interval_,
                   glob_, fpa_, fpa_) {}

    const pud_rule_id* add_axiom(size_t entry_idx,
                                 std::vector<pud_added_unification> unifs,
                                 std::vector<const expr*> body,
                                 uint32_t lvc) {
        const pud_rule_id* id = pool_.make_axiom(entry_idx);
        added_unifications_.store(id, std::move(unifs));
        added_body_goals_.store(id, std::move(body));
        lvc_.store(id, lvc);
        parent_.store(id, nullptr);
        node_interval_.store(id, om_.allocate_root());
        return id;
    }

    const pud_rule_id* add_inference(const pud_rule_id* caller,
                                     size_t call_site,
                                     const pud_rule_id* callee,
                                     std::vector<pud_added_unification> unifs,
                                     std::vector<const expr*> body,
                                     uint32_t lvc) {
        const pud_rule_id* id = pool_.make_inference(caller, call_site, callee);
        added_unifications_.store(id, std::move(unifs));
        added_body_goals_.store(id, std::move(body));
        lvc_.store(id, lvc);
        return id;
    }

    void store_children_of(const pud_rule_id* parent,
                           std::initializer_list<const pud_rule_id*> kids) {
        children_.store(parent, std::set<const pud_rule_id*>{kids});
        for (const pud_rule_id* child : kids)
            parent_.store(child, parent);
    }

    void start_query(const pud_rule_id* leaf,
                     size_t body_goal_idx,
                     const expr* body_goal,
                     uint32_t frame_offset) {
        starter_.start(leaf, body_goal_idx, body_goal, frame_offset);
    }

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    pud_node_added_unifications added_unifications_;
    pud_node_added_body_goals added_body_goals_;
    pud_node_lvc lvc_;
    pud_node_children children_;
    pud_node_parent parent_;
    pud_node_interval node_interval_;
    pud_node_added_touched_caller_reps added_caller_reps_;
    witness_search_t witness_;
    candidate_search_t candidate_;
    query_starter_t starter_;
};

TEST_F(PudForestSearchIntegrationTest, WitnessSearchFindsUnifyingAxiomLeaf) {
    const expr* pred = exprs_.make_functor(4, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {}, 1);
    pud_witness_search_context ctx{axiom, 0, pred, 1, axiom, axiom};
    start_query(axiom, 0, pred, 1);
    witness_.resume(ctx);
    EXPECT_EQ(ctx.current, axiom);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchSelfWitnessesMatchingLeaf) {
    const expr* pred = exprs_.make_functor(5, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {}, 1);
    pud_candidate_search_context ctx{
        axiom, 0, pred, 1, axiom, std::nullopt, added_body_goals_.get(axiom)};
    start_query(axiom, 0, pred, 1);
    candidate_.resume(ctx);
    EXPECT_EQ(ctx.cursor, axiom);
    EXPECT_FALSE(ctx.witnesses.has_value());
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchRefutesAxiomWhenHeadDoesNotUnify) {
    const expr* head = exprs_.make_functor(6, {});
    const expr* body = exprs_.make_functor(7, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, head}}, {body}, 1);
    pud_candidate_search_context ctx{
        axiom, 0, body, 1, axiom, std::nullopt, added_body_goals_.get(axiom)};
    start_query(axiom, 0, body, 1);
    candidate_.resume(ctx);
    EXPECT_EQ(ctx.cursor, nullptr);
}

TEST_F(PudForestSearchIntegrationTest, WitnessSearchFindsGrandchildUnderLinkedForest) {
    const expr* pred = exprs_.make_functor(8, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* child = add_inference(axiom, 0, axiom, {{0, pred}}, {pred}, 1);
    store_children_of(axiom, {child});
    const pud_rule_id* grand = add_inference(child, 0, axiom, {{0, pred}}, {}, 1);
    store_children_of(child, {grand});
    pud_witness_search_context ctx{axiom, 0, pred, 1, axiom, axiom};
    start_query(axiom, 0, pred, 1);
    witness_.resume(ctx);
    EXPECT_EQ(ctx.current, grand);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchChoicePointOnTwoLinkedChildren) {
    const expr* pred = exprs_.make_functor(9, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* c0 = add_inference(axiom, 0, axiom, {{0, pred}}, {}, 1);
    const pud_rule_id* c1 = add_inference(axiom, 1, axiom, {{0, pred}}, {}, 1);
    store_children_of(axiom, {c0, c1});
    pud_candidate_search_context ctx{
        axiom, 0, pred, 1, axiom, std::nullopt, added_body_goals_.get(axiom)};
    start_query(axiom, 0, pred, 1);
    candidate_.resume(ctx);
    ASSERT_TRUE(ctx.witnesses.has_value());
    EXPECT_NE(ctx.witnesses->a.current, nullptr);
    EXPECT_NE(ctx.witnesses->b.current, nullptr);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchAdvancesWhenWitnessCurrentIsDeeper) {
    const expr* pred = exprs_.make_functor(10, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* child = add_inference(axiom, 0, axiom, {{0, pred}}, {pred}, 1);
    store_children_of(axiom, {child});
    const pud_rule_id* grand = add_inference(child, 0, axiom, {{0, pred}}, {}, 1);
    store_children_of(child, {grand});
    pud_candidate_search_context ctx{
        axiom, 0, pred, 1, axiom, std::nullopt, added_body_goals_.get(axiom)};
    start_query(axiom, 0, pred, 1);
    candidate_.resume(ctx);
    EXPECT_EQ(ctx.cursor, grand);
    EXPECT_FALSE(ctx.witnesses.has_value());
}
