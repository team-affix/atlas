// Integration: closed pud_manifest unfold reaction — fork, yields, self-unfold, no nested unfold.

#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudManifestIntegrationTest : public ::testing::Test {
    PudManifestIntegrationTest()
        : dummy_open_(0)
        , dummy_close_(1)
        , dummy_{om_label(&dummy_open_), om_label(&dummy_close_)} {}

    const expr* pred(const char* name) {
        return m_.exprs_.make_functor(functors_.id(name), {});
    }

    const pud_rule_id* add_axiom(size_t entry_idx,
                                 const expr* head,
                                 std::vector<const expr*> body) {
        return m_.forest_.add_axiom(
            entry_idx, pud_db_node{dummy_, {{0, head}}, std::move(body), 1});
    }

    void install_leaf_queries(const pud_rule_id* leaf) {
        const pud_db_node& node = m_.forest_.get_node(leaf);
        std::vector<pud_query> queries;
        for (const expr* goal : node.added_body_goals) {
            std::vector<pud_candidate_search_context> axiom_contexts;
            for (const pud_rule_id* root : m_.forest_.ordered_roots())
                axiom_contexts.push_back(pud_candidate_search_context{root, {}});
            queries.push_back(pud_query{
                m_.om_.allocate_child_of(node.interval),
                goal,
                std::move(axiom_contexts),
                node.lvc});
        }
        m_.leaf_queries_.replace_leaf_queries(leaf, std::move(queries));
        const uint32_t frame_offset = node.lvc;
        for (pud_query* query : m_.leaf_queries_.get(leaf)) {
            m_.unify_head_.bind_query(*query, frame_offset);
            m_.reinit_.reinit(*query, frame_offset);
            for (pud_candidate_search_context& axiom_ctx : query->axiom_contexts)
                m_.candidate_search_.resume(axiom_ctx);
            if (query->axiom_contexts.empty())
                continue;
            for (const pud_candidate_search_context& axiom_ctx : query->axiom_contexts) {
                if (axiom_ctx.live_edges.empty())
                    m_.watchers_.watch(axiom_ctx.cursor, query);
                for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
                    m_.watchers_.watch(edge.current, query);
            }
        }
    }

    std::vector<pud_forced_unfold> drain(coroutine<pud_forced_unfold, void> task) {
        std::vector<pud_forced_unfold> yields;
        while (!task.done()) {
            task.resume();
            if (task.has_yield())
                yields.push_back(task.consume_yield());
        }
        return yields;
    }

    uint64_t dummy_open_;
    uint64_t dummy_close_;
    om_interval dummy_;
    test_functors functors_;
    pud_manifest m_;
};

TEST_F(PudManifestIntegrationTest, UnfoldForksLeftoverQueryOntoTheChild) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(0, p, {q, r});
    const pud_rule_id* a1 = add_axiom(1, q, {});
    install_leaf_queries(a0);
    install_leaf_queries(a1);

    drain(m_.unfolder_.unfold(a0, 0, a1));

    EXPECT_FALSE(m_.forest_.is_leaf(a0));
    ASSERT_EQ(m_.forest_.ordered_children(a0).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(a0)[0];
    EXPECT_TRUE(m_.forest_.is_leaf(child));
    const std::vector<pud_query*>& child_queries = m_.leaf_queries_.get(child);
    ASSERT_EQ(child_queries.size(), 1u);
    EXPECT_EQ(child_queries[0]->body_goal, r);
}

TEST_F(PudManifestIntegrationTest, UnfoldOfWitnessAdvancesOtherQueryWithoutNestedUnfold) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* t = pred("t");
    const pud_rule_id* a0 = add_axiom(0, p, {q});
    const pud_rule_id* a1 = add_axiom(1, q, {t});
    const pud_rule_id* a2 = add_axiom(2, t, {});
    install_leaf_queries(a0);
    install_leaf_queries(a1);
    install_leaf_queries(a2);

    pud_query* a0_query = m_.leaf_queries_.get(a0)[0];
    bool watched_a1 = a0_query->axiom_contexts.size() > 1;
    EXPECT_TRUE(watched_a1 || !a0_query->axiom_contexts.empty());

    const size_t leaf_count_before = m_.forest_.leaves().size();
    drain(m_.unfolder_.unfold(a1, 0, a2));
    EXPECT_EQ(m_.forest_.leaves().size(), leaf_count_before);
    EXPECT_FALSE(m_.forest_.is_leaf(a1));
    ASSERT_EQ(m_.forest_.ordered_children(a1).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(a1)[0];
    EXPECT_TRUE(m_.forest_.is_leaf(child));
    EXPECT_TRUE(m_.forest_.is_leaf(a0));
}

TEST_F(PudManifestIntegrationTest, SelfUnfoldInternsInferenceWithSelfCallee) {
    const expr* p = pred("p");
    const pud_rule_id* leaf = add_axiom(0, p, {p});
    install_leaf_queries(leaf);

    drain(m_.unfolder_.unfold(leaf, 0, leaf));

    ASSERT_EQ(m_.forest_.ordered_children(leaf).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(leaf)[0];
    const pud_rule_id::inference& inf = std::get<pud_rule_id::inference>(child->content);
    EXPECT_EQ(inf.caller, leaf);
    EXPECT_EQ(inf.callee, leaf);
    EXPECT_EQ(inf.call_site, 0u);
}

TEST_F(PudManifestIntegrationTest, ZeroCandidatesYieldsRefutedForTheChild) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(0, p, {q, r});
    const pud_rule_id* a1 = add_axiom(1, q, {});
    install_leaf_queries(a0);
    install_leaf_queries(a1);

    const std::vector<pud_forced_unfold> yields = drain(m_.unfolder_.unfold(a0, 0, a1));
    ASSERT_FALSE(yields.empty());
    bool saw_refuted = false;
    for (const pud_forced_unfold& yield : yields) {
        if (!std::holds_alternative<pud_forced_unfold::refuted>(yield.content))
            continue;
        saw_refuted = true;
        EXPECT_EQ(std::get<pud_forced_unfold::refuted>(yield.content).leaf,
                  m_.forest_.ordered_children(a0)[0]);
    }
    EXPECT_TRUE(saw_refuted);
}

TEST_F(PudManifestIntegrationTest, UnitYieldDoesNotUnfoldTheUnitLeaf) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* s = pred("s");
    const pud_rule_id* a0 = add_axiom(0, p, {q});
    const pud_rule_id* a1 = add_axiom(1, q, {s});
    const pud_rule_id* a2 = add_axiom(2, s, {});
    install_leaf_queries(a0);
    install_leaf_queries(a1);
    install_leaf_queries(a2);

    const std::vector<pud_forced_unfold> yields = drain(m_.unfolder_.unfold(a0, 0, a1));
    const pud_rule_id* child = m_.forest_.ordered_children(a0)[0];
    EXPECT_TRUE(m_.forest_.is_leaf(child));

    bool saw_child_unit = false;
    for (const pud_forced_unfold& yield : yields) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        if (std::get<pud_forced_unfold::unit>(yield.content).leaf != child)
            continue;
        saw_child_unit = true;
    }
    EXPECT_TRUE(saw_child_unit);
    EXPECT_TRUE(m_.forest_.is_leaf(child));
}
