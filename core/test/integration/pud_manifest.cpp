// Integration: closed pud_manifest — only add_axiom(const rule&) and unfold.

#include <algorithm>
#include <gtest/gtest.h>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

struct PudManifestIntegrationTest : public ::testing::Test {
    const expr* pred(const char* name) {
        return m_.exprs_.make_functor(functors_.id(name), {});
    }

    const expr* fn(const char* name, std::vector<const expr*> args) {
        return m_.exprs_.make_functor(functors_.id(name), std::move(args));
    }

    const pud_rule_id* add_axiom(const expr* head, std::vector<const expr*> body) {
        return m_.axiom_adder_.add_axiom(rule{head, std::move(body), 1});
    }

    const pud_rule_id* add_rule(const expr* head,
                                std::vector<const expr*> body,
                                uint32_t var_count) {
        return m_.axiom_adder_.add_axiom(rule{head, std::move(body), var_count});
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
            if (!m_.children_.get(id).has_value())
                continue;
            EXPECT_THROW(m_.queries_.unfold_site(id, 0), std::out_of_range);
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

    EXPECT_TRUE(m_.children_.get(a0).has_value());
    ASSERT_EQ(m_.children_.get(a0)->size(), 1u);
    const pud_rule_id* child = *m_.children_.get(a0)->begin();
    EXPECT_FALSE(m_.children_.get(child).has_value());
    EXPECT_EQ(m_.queries_.unfold_site(child, 0).body_goal, r);
}

TEST_F(PudManifestIntegrationTest, HoleLivesOnQueryInternAndIsVisibleFromSearch) {
    const expr* p = pred("p");
    const pud_rule_id* caller = add_axiom(p, {p});
    const pud_rule_id* fact = add_axiom(p, {});
    m_.queries_.unfold_site(caller, 0);
    const pud_rule_id* query_key = m_.pool_.make_inference(caller, 0, nullptr);
    ASSERT_TRUE(m_.node_interval_.contains(query_key));
    const om_interval query_interval = m_.node_interval_.get(query_key);
    const uint32_t hole = m_.globalizer_.globalize(m_.lvc_.get(caller), 0);
    ASSERT_TRUE(m_.fpa_.query(query_interval.open, hole).has_value());
    EXPECT_EQ(m_.fpa_.query(query_interval.open, hole)->skeleton,
              m_.added_body_goals_.get(caller)[0]);
    EXPECT_EQ(m_.fpa_.query(query_interval.open, hole)->frame_offset, 0u);

    const pud_rule_id* search_key = m_.pool_.make_inference(caller, 0, fact);
    ASSERT_TRUE(m_.node_interval_.contains(search_key));
    const om_interval search_interval = m_.node_interval_.get(search_key);
    ASSERT_TRUE(m_.fpa_.query(search_interval.open, hole).has_value());
    EXPECT_EQ(m_.fpa_.query(search_interval.open, hole)->skeleton,
              m_.added_body_goals_.get(caller)[0]);
}

TEST_F(PudManifestIntegrationTest, UnfoldOfWitnessAdvancesOtherQueryWithoutNestedUnfold) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* t = pred("t");
    const pud_rule_id* a0 = add_axiom(p, {q});
    const pud_rule_id* a1 = add_axiom(q, {t});
    add_axiom(t, {});

    EXPECT_FALSE(m_.queries_.unfold_site(a0, 0).live.empty());

    drain(m_.unfolder_.unfold(a1, 0));
    EXPECT_TRUE(m_.children_.get(a1).has_value());
    ASSERT_EQ(m_.children_.get(a1)->size(), 1u);
    const pud_rule_id* child = *m_.children_.get(a1)->begin();
    EXPECT_FALSE(m_.children_.get(child).has_value());
    EXPECT_FALSE(m_.children_.get(a0).has_value());
}

TEST_F(PudManifestIntegrationTest, SelfUnfoldInternsInferenceWithSelfCallee) {
    const expr* p = pred("p");
    const pud_rule_id* leaf = add_axiom(p, {p});

    drain(m_.unfolder_.unfold(leaf, 0));

    ASSERT_EQ(m_.children_.get(leaf)->size(), 1u);
    const pud_rule_id* child = *m_.children_.get(leaf)->begin();
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
                  *m_.children_.get(a0)->begin());
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
    const pud_rule_id* child = *m_.children_.get(a0)->begin();
    EXPECT_FALSE(m_.children_.get(child).has_value());

    bool saw_child_unit = false;
    for (const pud_forced_unfold& yield : out.yields) {
        if (!std::holds_alternative<pud_forced_unfold::unit>(yield.content))
            continue;
        if (std::get<pud_forced_unfold::unit>(yield.content).leaf != child)
            continue;
        saw_child_unit = true;
    }
    EXPECT_TRUE(saw_child_unit);
    EXPECT_FALSE(m_.children_.get(child).has_value());
}

TEST_F(PudManifestIntegrationTest, UnfoldFansOutOneChildPerMatchingAxiom) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(p, {q, r});
    const pud_rule_id* a1 = add_axiom(q, {});
    const pud_rule_id* a2 = add_axiom(q, {});

    const unfold_out out = drain(m_.unfolder_.unfold(a0, 0));

    EXPECT_TRUE(m_.children_.get(a0).has_value());
    ASSERT_EQ(out.children.size(), 2u);
    const pud_rule_id::inference& inf0 =
        std::get<pud_rule_id::inference>(out.children[0]->content);
    const pud_rule_id::inference& inf1 =
        std::get<pud_rule_id::inference>(out.children[1]->content);
    EXPECT_TRUE(inf0.callee == a1 || inf0.callee == a2);
    EXPECT_TRUE(inf1.callee == a1 || inf1.callee == a2);
    EXPECT_NE(inf0.callee, inf1.callee);
    for (const pud_rule_id* child : out.children) {
        EXPECT_FALSE(m_.children_.get(child).has_value());
        EXPECT_EQ(m_.queries_.unfold_site(child, 0).body_goal, r);
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
    ASSERT_EQ(m_.children_.get(a1)->size(), 2u);
    EXPECT_FALSE(m_.children_.get(a0).has_value());

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
    EXPECT_FALSE(m_.children_.get(out.children[0]).has_value());
}

TEST_F(PudManifestIntegrationTest, LateAxiomAppearsOnExistingLeafContexts) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const pud_rule_id* a0 = add_axiom(p, {q});
    const pud_rule_id* a1 = add_axiom(q, {});
    bool found = false;
    for (pud_candidate_search_context* ctx : m_.queries_.unfold_site(a0, 0).live) {
        if (ctx->cursor != a1)
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
    ASSERT_EQ(m_.children_.get(a0)->size(), 1u);
    const pud_rule_id* child = *m_.children_.get(a0)->begin();
    EXPECT_EQ(m_.queries_.unfold_site(child, 0).body_goal, q);
}

TEST_F(PudManifestIntegrationTest, NestedUnfoldCascadeOnUnitYields) {
    const expr* p = pred("p");
    const expr* q = pred("q");
    const expr* r = pred("r");
    const pud_rule_id* a0 = add_axiom(p, {q});
    add_axiom(q, {r});
    add_axiom(r, {});
    drain(m_.unfolder_.unfold(a0, 0));
    ASSERT_EQ(m_.children_.get(a0)->size(), 1u);
    const pud_rule_id* child = *m_.children_.get(a0)->begin();
    ASSERT_FALSE(m_.children_.get(child).has_value());
    ASSERT_FALSE(m_.queries_.unfold_site(child, 0).live.empty());
    drain(m_.unfolder_.unfold(child, 0));
    EXPECT_TRUE(m_.children_.get(child).has_value());
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
    const std::vector<pud_added_unification>& added =
        m_.added_unifications_.get(out.children[0]);
    const std::vector<const expr*>& child_goals =
        m_.added_body_goals_.get(out.children[0]);
    bool saw_a = false;
    for (const pud_added_unification& unif : added) {
        if (unif.value == a)
            saw_a = true;
    }
    for (const expr* goal : child_goals) {
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
    if (!m_.children_.get(known[0]).has_value()
            && !m_.queries_.unfold_site(known[0], 0).live.empty()) {
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
        EXPECT_FALSE(m_.children_.get(child).has_value());
        EXPECT_TRUE(m_.added_body_goals_.get(child).empty());
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
    std::unordered_map<const pud_rule_id*, size_t> query_count;
    for (int step = 0; step < 12; ++step) {
        const expr* head = preds[pred_dist(rng)];
        std::vector<const expr*> body;
        const int len = body_len(rng);
        for (int idx = 0; idx < len; ++idx)
            body.push_back(preds[pred_dist(rng)]);
        const size_t n_queries = body.size();
        const pud_rule_id* id = add_axiom(head, std::move(body));
        known.push_back(id);
        owned.push_back(id);
        query_count[id] = n_queries;
        log << "add ";
    }
    expect_query_leaf_invariant(known);
    for (int step = 0; step < 12; ++step) {
        log << step << ' ';
        std::vector<const pud_rule_id*> unfoldable;
        for (const pud_rule_id* leaf : owned) {
            if (m_.children_.get(leaf).has_value())
                continue;
            if (query_count[leaf] == 0)
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            break;
        const pud_rule_id* leaf = unfoldable[rng() % unfoldable.size()];
        const size_t idx = rng() % query_count[leaf];
        const std::vector<pud_candidate_search_context*> live =
            m_.queries_.unfold_site(leaf, idx).live;
        if (live.empty() || live.size() > 4)
            continue;
        const size_t parent_queries = query_count[leaf];
        const unfold_out out = drain(m_.unfolder_.unfold(leaf, idx));
        owned.erase(std::remove(owned.begin(), owned.end(), leaf), owned.end());
        query_count.erase(leaf);
        for (const pud_rule_id* child : out.children) {
            known.push_back(child);
            owned.push_back(child);
            query_count[child] = parent_queries - 1
                + m_.added_body_goals_.get(child).size();
        }
        expect_query_leaf_invariant(known);
    }
    EXPECT_FALSE(log.str().empty()) << "seed " << k_seed;
}

TEST_F(PudManifestIntegrationTest, LeftoverSharedVarMatchesOnlyBoundFact) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* b = pred("b");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* r_x = fn("r", {x});
    const expr* q_a = fn("q", {a});
    const expr* r_a = fn("r", {a});
    const expr* r_b = fn("r", {b});
    const pud_rule_id* caller = add_rule(p_x, {q_x, r_x}, 1);
    add_rule(q_a, {}, 1);
    const pud_rule_id* r_a_ax = add_rule(r_a, {}, 1);
    add_rule(r_b, {}, 1);

    const unfold_out first = drain(m_.unfolder_.unfold(caller, 0));
    ASSERT_EQ(first.children.size(), 1u);
    const pud_rule_id* child = first.children[0];
    const std::vector<pud_added_unification>& first_added =
        m_.added_unifications_.get(child);
    bool saw_x_to_a = false;
    for (const pud_added_unification& unif : first_added) {
        if (unif.var_idx == 1 && unif.value == a)
            saw_x_to_a = true;
    }
    EXPECT_TRUE(saw_x_to_a);
    EXPECT_EQ(m_.queries_.unfold_site(child, 0).body_goal,
              m_.added_body_goals_.get(caller)[1]);
    ASSERT_EQ(m_.queries_.unfold_site(child, 0).live.size(), 1u);

    const unfold_out second = drain(m_.unfolder_.unfold(child, 0));
    ASSERT_EQ(second.children.size(), 1u);
    const pud_rule_id::inference& inf =
        std::get<pud_rule_id::inference>(second.children[0]->content);
    EXPECT_EQ(inf.callee, r_a_ax);
}

TEST_F(PudManifestIntegrationTest, CalleeRepeatedVarIdentifiesCallerVars) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* y = m_.exprs_.make_var(1);
    const expr* a = m_.exprs_.make_var(0);
    const expr* p_xy = fn("p", {x, y});
    const expr* q_xy = fn("q", {x, y});
    const expr* q_aa = fn("q", {a, a});
    const pud_rule_id* caller = add_rule(p_xy, {q_xy}, 2);
    add_rule(q_aa, {}, 1);

    const unfold_out out = drain(m_.unfolder_.unfold(caller, 0));
    ASSERT_EQ(out.children.size(), 1u);
    const std::vector<pud_added_unification>& added =
        m_.added_unifications_.get(out.children[0]);
    bool saw_caller_ident = false;
    for (const pud_added_unification& unif : added) {
        EXPECT_LT(unif.var_idx, m_.lvc_.get(caller));
        if (unif.var_idx == 2)
            saw_caller_ident = true;
    }
    EXPECT_TRUE(saw_caller_ident);
}

TEST_F(PudManifestIntegrationTest, NonRecursiveChainUnfoldsInSeriesToFact) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* b = pred("b");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* r_x = fn("r", {x});
    const expr* r_a = fn("r", {a});
    const expr* s_b = fn("s", {b});
    const pud_rule_id* p_ax = add_rule(p_x, {q_x}, 1);
    add_rule(q_x, {r_x}, 1);
    add_rule(r_a, {}, 1);
    add_rule(s_b, {}, 1);

    const unfold_out first = drain(m_.unfolder_.unfold(p_ax, 0));
    ASSERT_EQ(first.children.size(), 1u);
    const pud_rule_id* child = first.children[0];
    ASSERT_FALSE(m_.queries_.unfold_site(child, 0).live.empty());
    const unfold_out second = drain(m_.unfolder_.unfold(child, 0));
    ASSERT_EQ(second.children.size(), 1u);
    bool saw_a = false;
    for (const pud_added_unification& unif :
         m_.added_unifications_.get(second.children[0])) {
        EXPECT_LT(unif.var_idx, m_.lvc_.get(child));
        if (unif.value == a)
            saw_a = true;
    }
    EXPECT_TRUE(saw_a);
}

TEST_F(PudManifestIntegrationTest, ThreeStepChainDoesNotUnifyWrongPredicate) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* c = pred("c");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* r_x = fn("r", {x});
    const expr* r_a = fn("r", {a});
    const expr* t_c = fn("t", {c});
    const pud_rule_id* p_ax = add_rule(p_x, {q_x}, 1);
    add_rule(q_x, {r_x}, 1);
    add_rule(r_a, {}, 1);
    add_rule(t_c, {}, 1);

    const unfold_out first = drain(m_.unfolder_.unfold(p_ax, 0));
    const pud_rule_id* child = first.children[0];
    const unfold_out second = drain(m_.unfolder_.unfold(child, 0));
    ASSERT_EQ(second.children.size(), 1u);
    for (const pud_added_unification& unif :
         m_.added_unifications_.get(second.children[0])) {
        EXPECT_NE(unif.value, c);
    }
}

TEST_F(PudManifestIntegrationTest, UnfoldNonRootCursorConcatenatesPathDeltas) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* r_x = fn("r", {x});
    const expr* r_a = fn("r", {a});
    const pud_rule_id* p_ax = add_rule(p_x, {q_x}, 1);
    const pud_rule_id* q_ax = add_rule(q_x, {r_x}, 1);
    add_rule(r_a, {}, 1);

    const unfold_out q_out = drain(m_.unfolder_.unfold(q_ax, 0));
    ASSERT_EQ(q_out.children.size(), 1u);
    const unfold_out p_out = drain(m_.unfolder_.unfold(p_ax, 0));
    ASSERT_EQ(p_out.children.size(), 1u);
    const pud_rule_id::inference& inf =
        std::get<pud_rule_id::inference>(p_out.children[0]->content);
    EXPECT_EQ(inf.callee, q_out.children[0]);
    bool saw_a = false;
    for (const pud_added_unification& unif :
         m_.added_unifications_.get(p_out.children[0])) {
        EXPECT_LT(unif.var_idx, m_.lvc_.get(p_ax));
        if (unif.value == a)
            saw_a = true;
    }
    EXPECT_TRUE(saw_a);
}

TEST_F(PudManifestIntegrationTest, NatRecursionDoesNotSmashCalleeVarZeroIntoCallerHead) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* z = pred("z");
    const expr* s_x = fn("s", {x});
    const expr* nat_z = fn("nat", {z});
    const expr* nat_s_x = fn("nat", {s_x});
    const expr* nat_x = fn("nat", {x});
    add_rule(nat_z, {}, 1);
    const pud_rule_id* succ = add_rule(nat_s_x, {nat_x}, 1);

    const unfold_out first = drain(m_.unfolder_.unfold(succ, 0));
    ASSERT_EQ(first.children.size(), 2u);
    const pud_rule_id* rec = nullptr;
    for (const pud_rule_id* child : first.children) {
        const pud_rule_id::inference& inf =
            std::get<pud_rule_id::inference>(child->content);
        if (inf.callee != succ)
            continue;
        rec = child;
        break;
    }
    ASSERT_NE(rec, nullptr);
    for (const pud_added_unification& unif : m_.added_unifications_.get(rec))
        EXPECT_LT(unif.var_idx, m_.lvc_.get(succ));
    ASSERT_FALSE(m_.queries_.unfold_site(rec, 0).live.empty());
    const unfold_out second = drain(m_.unfolder_.unfold(rec, 0));
    ASSERT_FALSE(second.children.empty());
    for (const pud_rule_id* grand : second.children) {
        for (const pud_added_unification& unif : m_.added_unifications_.get(grand))
            EXPECT_LT(unif.var_idx, m_.lvc_.get(rec));
    }
}

TEST_F(PudManifestIntegrationTest, EvenOddMutualRecursionKeepsDistinctHeads) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* z = pred("z");
    const expr* s_x = fn("s", {x});
    const expr* even_z = fn("even", {z});
    const expr* even_s_x = fn("even", {s_x});
    const expr* odd_s_x = fn("odd", {s_x});
    const expr* even_x = fn("even", {x});
    const expr* odd_x = fn("odd", {x});
    add_rule(even_z, {}, 1);
    const pud_rule_id* even_succ = add_rule(even_s_x, {odd_x}, 1);
    add_rule(odd_s_x, {even_x}, 1);

    const unfold_out first = drain(m_.unfolder_.unfold(even_succ, 0));
    ASSERT_FALSE(first.children.empty());
    for (const pud_rule_id* child : first.children) {
        const pud_rule_id::inference& inf =
            std::get<pud_rule_id::inference>(child->content);
        const expr* callee_head = m_.added_unifications_.get(inf.callee)[0].value;
        EXPECT_NE(callee_head, even_s_x);
        for (const pud_added_unification& unif : m_.added_unifications_.get(child))
            EXPECT_LT(unif.var_idx, m_.lvc_.get(even_succ));
    }
}

TEST_F(PudManifestIntegrationTest, SelfUnfoldDeepensSuccessorPeel) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* z = pred("z");
    const expr* s_x = fn("s", {x});
    const expr* p_z = fn("p", {z});
    const expr* p_s_x = fn("p", {s_x});
    const expr* p_x = fn("p", {x});
    add_rule(p_z, {}, 1);
    const pud_rule_id* succ = add_rule(p_s_x, {p_x}, 1);

    const unfold_out first = drain(m_.unfolder_.unfold(succ, 0));
    ASSERT_EQ(first.children.size(), 2u);
    const pud_rule_id* rec = nullptr;
    for (const pud_rule_id* child : first.children) {
        const pud_rule_id::inference& inf =
            std::get<pud_rule_id::inference>(child->content);
        if (inf.callee != succ)
            continue;
        rec = child;
        break;
    }
    ASSERT_NE(rec, nullptr);
    bool saw_s_on_rec = false;
    for (const pud_added_unification& unif : m_.added_unifications_.get(rec)) {
        EXPECT_LT(unif.var_idx, m_.lvc_.get(succ));
        const expr::functor* f = std::get_if<expr::functor>(&unif.value->content);
        if (f == nullptr || f->args.empty())
            continue;
        saw_s_on_rec = true;
    }
    EXPECT_TRUE(saw_s_on_rec);
    ASSERT_FALSE(m_.queries_.unfold_site(rec, 0).live.empty());
    const unfold_out second = drain(m_.unfolder_.unfold(rec, 0));
    ASSERT_FALSE(second.children.empty());
    bool saw_s_on_grand = false;
    for (const pud_rule_id* grand : second.children) {
        for (const pud_added_unification& unif : m_.added_unifications_.get(grand)) {
            EXPECT_LT(unif.var_idx, m_.lvc_.get(rec));
            const expr::functor* f = std::get_if<expr::functor>(&unif.value->content);
            if (f == nullptr || f->args.empty())
                continue;
            saw_s_on_grand = true;
        }
    }
    EXPECT_TRUE(saw_s_on_grand);
}

TEST_F(PudManifestIntegrationTest, FanOutIgnoresNonUnifyingHead) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* b = pred("b");
    const expr* c = pred("c");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* q_a = fn("q", {a});
    const expr* q_b = fn("q", {b});
    const expr* r_c = fn("r", {c});
    const pud_rule_id* caller = add_rule(p_x, {q_x}, 1);
    add_rule(q_a, {}, 1);
    add_rule(q_b, {}, 1);
    add_rule(r_c, {}, 1);

    const unfold_out out = drain(m_.unfolder_.unfold(caller, 0));
    ASSERT_EQ(out.children.size(), 2u);
    for (const pud_rule_id* child : out.children) {
        for (const pud_added_unification& unif : m_.added_unifications_.get(child))
            EXPECT_NE(unif.value, c);
    }
}

TEST_F(PudManifestIntegrationTest, FanOutAfterPriorUnfoldKeepsCallerVarZero) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* b = pred("b");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* r_x = fn("r", {x});
    const expr* r_a = fn("r", {a});
    const expr* r_b = fn("r", {b});
    const pud_rule_id* q_ax = add_rule(q_x, {r_x}, 1);
    add_rule(r_a, {}, 1);
    add_rule(r_b, {}, 1);
    const pud_rule_id* p_ax = add_rule(p_x, {q_x}, 1);

    drain(m_.unfolder_.unfold(q_ax, 0));
    const unfold_out out = drain(m_.unfolder_.unfold(p_ax, 0));
    ASSERT_FALSE(out.children.empty());
    for (const pud_rule_id* child : out.children) {
        for (const pud_added_unification& unif : m_.added_unifications_.get(child))
            EXPECT_LT(unif.var_idx, m_.lvc_.get(p_ax));
    }
}

TEST_F(PudManifestIntegrationTest, ChoicePointCursorWithVarBody) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* b = pred("b");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* q_a = fn("q", {a});
    const expr* q_b = fn("q", {b});
    const pud_rule_id* caller = add_rule(p_x, {q_x}, 1);
    add_rule(q_a, {}, 1);
    add_rule(q_b, {}, 1);

    const unfold_out out = drain(m_.unfolder_.unfold(caller, 0));
    ASSERT_EQ(out.children.size(), 2u);
}

TEST_F(PudManifestIntegrationTest, DifferentHeadStaysRefuted) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* a = pred("a");
    const expr* p_x = fn("p", {x});
    const expr* q_x = fn("q", {x});
    const expr* r_a = fn("r", {a});
    const pud_rule_id* caller = add_rule(p_x, {q_x}, 1);
    add_rule(r_a, {}, 1);

    EXPECT_TRUE(m_.queries_.unfold_site(caller, 0).live.empty());
}

TEST_F(PudManifestIntegrationTest, RecursionWithoutBaseDoesNotSpuriousSucceed) {
    const expr* x = m_.exprs_.make_var(0);
    const expr* s_x = fn("s", {x});
    const expr* p_s_x = fn("p", {s_x});
    const expr* p_x = fn("p", {x});
    const pud_rule_id* succ = add_rule(p_s_x, {p_x}, 1);

    const unfold_out out = drain(m_.unfolder_.unfold(succ, 0));
    ASSERT_EQ(out.children.size(), 1u);
    const pud_rule_id* child = out.children[0];
    EXPECT_EQ(std::get<pud_rule_id::inference>(child->content).callee, succ);
}
