// Integration: closed pud_manifest — only add_axiom(const rule&) and unfold.

#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_manifest.hpp"
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
    const std::vector<pud_query*>& child_queries = m_.leaf_queries_.get(child);
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

    pud_query* a0_query = m_.leaf_queries_.get(a0)[0];
    EXPECT_FALSE(a0_query->axiom_contexts.empty());

    const size_t leaf_count_before = m_.forest_.leaves().size();
    drain(m_.unfolder_.unfold(a1, 0));
    EXPECT_EQ(m_.forest_.leaves().size(), leaf_count_before);
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
        const std::vector<pud_query*>& child_queries = m_.leaf_queries_.get(child);
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
