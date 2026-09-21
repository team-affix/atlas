// Integration: closed pud_manifest — only add_axiom(const rule&) and unfold.

#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

struct PudManifestIntegrationTest : public ::testing::Test {
    const expr* pred(const char* name) {
        return m_.exprs_.make_functor(functors_.id(name), {});
    }

    const pud_rule_id* add_axiom(const expr* head, std::vector<const expr*> body) {
        return m_.axiom_adder_.add_axiom(rule{head, std::move(body), 1});
    }

    struct unfold_out {
        std::vector<pud_forced_unfold> yields;
        std::vector<const pud_rule_id*> children;
    };

    unfold_out drain(
            coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>> task) {
        unfold_out out;
        while (!task.done()) {
            task.resume();
            if (task.has_yield())
                out.yields.push_back(task.consume_yield());
        }
        out.children = task.result();
        return out;
    }

    void expect_query_leaf_invariant(const std::vector<const pud_rule_id*>& known) {
        for (const pud_rule_id* id : known) {
            if (m_.forest_.is_leaf(id)) {
                EXPECT_NO_THROW(m_.queries_.get(id));
                continue;
            }
            EXPECT_THROW(m_.queries_.get(id), std::out_of_range);
        }
    }

    test_functors functors_;
    pud_manifest m_;
};

TEST_F(PudManifestIntegrationTest, UnfoldForksLeftoverQueryOntoTheChild) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(p, {q, r});
    add_axiom(q, {});

    drain(m_.unfolder_.unfold(a0, 0));

    EXPECT_FALSE(m_.forest_.is_leaf(a0));
    ASSERT_EQ(m_.forest_.ordered_children(a0).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(a0)[0];
    EXPECT_TRUE(m_.forest_.is_leaf(child));
    const std::vector<pud_query*>& child_queries = m_.queries_.get(child);
    ASSERT_EQ(child_queries.size(), 1u);
    EXPECT_EQ(child_queries[0]->body_goal, r);
}

TEST_F(PudManifestIntegrationTest, UnfoldOfWitnessAdvancesOtherQueryWithoutNestedUnfold) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* t = pred("t");
    const pud_rule_id* a0 = add_axiom(p, {q});
    const pud_rule_id* a1 = add_axiom(q, {t});
    add_axiom(t, {});

    pud_query* a0_query = m_.queries_.get(a0)[0];
    EXPECT_FALSE(a0_query->axiom_contexts.empty());

    drain(m_.unfolder_.unfold(a1, 0));
    EXPECT_FALSE(m_.forest_.is_leaf(a1));
    ASSERT_EQ(m_.forest_.ordered_children(a1).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(a1)[0];
    EXPECT_TRUE(m_.forest_.is_leaf(child));
    EXPECT_TRUE(m_.forest_.is_leaf(a0));
}

TEST_F(PudManifestIntegrationTest, SelfUnfoldInternsInferenceWithSelfCallee) {
    const expr* p = pred("p");
    const pud_rule_id* leaf = add_axiom(p, {p});

    drain(m_.unfolder_.unfold(leaf, 0));

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
    const pud_rule_id* a0 = add_axiom(p, {q, r});
    add_axiom(q, {});

    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_FALSE(out.yields.empty());
    bool saw_refuted = false;
    for (const pud_forced_unfold& yield : out.yields) {
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
    const pud_rule_id* a0 = add_axiom(p, {q});
    add_axiom(q, {s});
    add_axiom(s, {});

    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));
    const pud_rule_id* child = m_.forest_.ordered_children(a0)[0];
    EXPECT_TRUE(m_.forest_.is_leaf(child));

    bool saw_child_unit = false;
    for (const pud_forced_unfold& yield : out.yields) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        if (std::get<pud_forced_unfold::unit>(yield.content).leaf != child)
            continue;
        saw_child_unit = true;
    }
    EXPECT_TRUE(saw_child_unit);
    EXPECT_TRUE(m_.forest_.is_leaf(child));
}

TEST_F(PudManifestIntegrationTest, UnfoldFansOutOneChildPerMatchingAxiom) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(p, {q, r});
    const pud_rule_id* a1 = add_axiom(q, {});
    const pud_rule_id* a2 = add_axiom(q, {});

    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));

    EXPECT_FALSE(m_.forest_.is_leaf(a0));
    ASSERT_EQ(out.children.size(), 2u);
    const pud_rule_id::inference& inf0 =
        std::get<pud_rule_id::inference>(out.children[0]->content);
    const pud_rule_id::inference& inf1 =
        std::get<pud_rule_id::inference>(out.children[1]->content);
    EXPECT_TRUE(inf0.callee == a1 || inf0.callee == a2);
    EXPECT_TRUE(inf1.callee == a1 || inf1.callee == a2);
    EXPECT_NE(inf0.callee, inf1.callee);
    for (const pud_rule_id* child : out.children) {
        EXPECT_TRUE(m_.forest_.is_leaf(child));
        const std::vector<pud_query*>& child_queries = m_.queries_.get(child);
        ASSERT_EQ(child_queries.size(), 1u);
        EXPECT_EQ(child_queries[0]->body_goal, r);
    }
}

TEST_F(PudManifestIntegrationTest, UnfoldUsesChoicePointCursorNotLeafWitness) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* t = pred("t");
    const pud_rule_id* a0 = add_axiom(p, {q});
    const pud_rule_id* a1 = add_axiom(q, {t});
    add_axiom(t, {});
    add_axiom(t, {});

    drain(m_.unfolder_.unfold(a1, 0));
    ASSERT_EQ(m_.forest_.ordered_children(a1).size(), 2u);
    EXPECT_TRUE(m_.forest_.is_leaf(a0));

    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));

    ASSERT_EQ(out.children.size(), 1u);
    const pud_rule_id::inference& inf =
        std::get<pud_rule_id::inference>(out.children[0]->content);
    EXPECT_EQ(inf.callee, a1);
    EXPECT_EQ(inf.caller, a0);
    EXPECT_EQ(inf.call_site, 0u);
}

TEST_F(PudManifestIntegrationTest, AddAxiomAfterExistingLeafPatchesThatLeafQuery) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const pud_rule_id* a0 = add_axiom(p, {q});
    add_axiom(q, {});

    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_EQ(out.children.size(), 1u);
    EXPECT_TRUE(m_.forest_.is_leaf(out.children[0]));
}

TEST_F(PudManifestIntegrationTest, LateAxiomAppearsOnExistingLeafContexts) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const pud_rule_id* a0 = add_axiom(p, {q});
    const pud_rule_id* a1 = add_axiom(q, {});
    bool found = false;
    for (const pud_candidate_search_context& ctx : m_.queries_.get(a0)[0]->axiom_contexts) {
        if (ctx.cursor != a1)
            continue;
        found = true;
    }
    EXPECT_TRUE(found);
    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_EQ(out.children.size(), 1u);
}

TEST_F(PudManifestIntegrationTest, UnfoldSecondGoalKeepsFirstLeftover) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(p, {q, r});
    add_axiom(r, {});
    drain(m_.unfolder_.unfold(a0, 1));
    ASSERT_EQ(m_.forest_.ordered_children(a0).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(a0)[0];
    ASSERT_EQ(m_.queries_.get(child).size(), 1u);
    EXPECT_EQ(m_.queries_.get(child)[0]->body_goal, q);
}

TEST_F(PudManifestIntegrationTest, NestedUnfoldCascadeOnUnitYields) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(p, {q});
    add_axiom(q, {r});
    add_axiom(r, {});
    drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_EQ(m_.forest_.ordered_children(a0).size(), 1u);
    const pud_rule_id* child = m_.forest_.ordered_children(a0)[0];
    ASSERT_TRUE(m_.forest_.is_leaf(child));
    ASSERT_FALSE(m_.queries_.get(child).empty());
    ASSERT_FALSE(m_.queries_.live_callees(child, 0).empty());
    drain(m_.unfolder_.unfold(child, 0));
    EXPECT_FALSE(m_.forest_.ordered_children(child).empty());
}

TEST_F(PudManifestIntegrationTest, VarHeadUnifyAndNormalizeOnUnfold) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* p = m_.exprs_.make_functor(functors_.id("p"), {x});
    const expr* q_x = m_.exprs_.make_functor(functors_.id("q"), {x});
    const expr* q_a = m_.exprs_.make_functor(functors_.id("q"), {a});
    const pud_rule_id* a0 = m_.axiom_adder_.add_axiom(rule{p, {q_x}, 1});
    m_.axiom_adder_.add_axiom(rule{q_a, {}, 1});
    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_EQ(out.children.size(), 1u);
    const pud_db_node& child = m_.forest_.get_node(out.children[0]);
    bool saw_a = false;
    for (const pud_added_unification& added : child.added_unifications) {
        if (added.value == a)
            saw_a = true;
    }
    for (const expr* goal : child.added_body_goals) {
        if (goal == a || goal == q_a)
            saw_a = true;
    }
    EXPECT_TRUE(saw_a);
}

TEST_F(PudManifestIntegrationTest, LoadThenUnfoldNeverOrphansQueries) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const expr* s = pred("s");
    std::vector<const pud_rule_id*> known;
    known.push_back(add_axiom(p, {q}));
    known.push_back(add_axiom(q, {r}));
    known.push_back(add_axiom(r, {}));
    known.push_back(add_axiom(s, {}));
    known.push_back(add_axiom(q, {}));
    expect_query_leaf_invariant(known);
    const unfold_out first = drain(m_.unfolder_.unfold(known[1], 0));
    known.insert(known.end(), first.children.begin(), first.children.end());
    expect_query_leaf_invariant(known);
    if (m_.forest_.is_leaf(known[0]) && !m_.queries_.live_callees(known[0], 0).empty()) {
        const unfold_out second = drain(m_.unfolder_.unfold(known[0], 0));
        known.insert(known.end(), second.children.begin(), second.children.end());
    }
    expect_query_leaf_invariant(known);
}

TEST_F(PudManifestIntegrationTest, StressManyFactsFanout) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const pud_rule_id* a0 = add_axiom(p, {q});
    for (int idx = 0; idx < 40; ++idx)
        add_axiom(q, {});
    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_EQ(out.children.size(), 40u);
    for (const pud_rule_id* child : out.children) {
        EXPECT_TRUE(m_.forest_.is_leaf(child));
        EXPECT_TRUE(m_.queries_.get(child).empty());
    }
}

TEST_F(PudManifestIntegrationTest, FuzzAddAxiomThenUnfold) {
    const expr* preds[4] = {pred("p"), pred("q"), pred("r"), pred("s")};
    constexpr uint32_t k_seed = 20260920;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> pred_dist(0, 3);
    std::uniform_int_distribution<int> body_len(0, 2);
    std::ostringstream log;
    std::vector<const pud_rule_id*> known;
    std::vector<const pud_rule_id*> owned;
    for (int step = 0; step < 12; ++step) {
        const expr* head = preds[pred_dist(rng)];
        std::vector<const expr*> body;
        const int len = body_len(rng);
        for (int idx = 0; idx < len; ++idx)
            body.push_back(preds[pred_dist(rng)]);
        const pud_rule_id* id = add_axiom(head, std::move(body));
        known.push_back(id);
        owned.push_back(id);
        log << "add ";
    }
    expect_query_leaf_invariant(known);
    for (int step = 0; step < 12; ++step) {
        log << step << ' ';
        std::vector<const pud_rule_id*> unfoldable;
        for (const pud_rule_id* leaf : owned) {
            if (!m_.forest_.is_leaf(leaf))
                continue;
            const std::vector<pud_query*>& qs = m_.queries_.get(leaf);
            if (qs.empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_rule_id* leaf = unfoldable[rng() % unfoldable.size()];
        const std::vector<pud_query*>& qs = m_.queries_.get(leaf);
        const size_t idx = rng() % qs.size();
        const std::vector<const pud_rule_id*> callees = m_.queries_.live_callees(leaf, idx);
        if (callees.empty() || callees.size() > 4)
            continue;
        const unfold_out out = drain(m_.unfolder_.unfold(leaf, idx));
        owned.erase(std::remove(owned.begin(), owned.end(), leaf), owned.end());
        for (const pud_rule_id* child : out.children) {
            known.push_back(child);
            owned.push_back(child);
        }
        expect_query_leaf_invariant(known);
    }
    EXPECT_FALSE(log.str().empty()) << "seed " << k_seed;
}
