// rule_unfolder: one-step Horn clause unfolding. Unifying a chosen body goal
// against each matching original-db head produces specialized rules; the
// subject rule is erased and the new rule_ids are returned.

#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/db.hpp"
#include "infrastructure/rule_unfolder_manifest.hpp"
#include "functor_fixture.hpp"

using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<rule_id> collect(rule_id_set& rs) {
    std::vector<rule_id> ids;
    auto it = rs.iterate();
    while (!it.done()) {
        it.resume();
        if (it.has_yield())
            ids.push_back(it.consume_yield());
    }
    return ids;
}

static bool expr_eq(const expr* a, const expr* b) {
    if (const expr::var* va = std::get_if<expr::var>(&a->content)) {
        const expr::var* vb = std::get_if<expr::var>(&b->content);
        return vb && va->index == vb->index;
    }
    if (const expr::functor* fa = std::get_if<expr::functor>(&a->content)) {
        const expr::functor* fb = std::get_if<expr::functor>(&b->content);
        if (!fb || fa->id != fb->id || fa->args.size() != fb->args.size())
            return false;
        for (size_t i = 0; i < fa->args.size(); ++i)
            if (!expr_eq(fa->args[i], fb->args[i]))
                return false;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

struct RuleUnfolderTest : public ::testing::Test {
    test_functors functors;

    // Ground atoms
    expr f_atom{expr::functor{functors.id("f"), {}}};
    expr g_atom{expr::functor{functors.id("g"), {}}};
    expr h_atom{expr::functor{functors.id("h"), {}}};
    expr a_atom{expr::functor{functors.id("a"), {}}};
    expr b_atom{expr::functor{functors.id("b"), {}}};

    // Variables
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};

    // Composite expressions
    expr f_var0{expr::functor{functors.id("f"), {&var0}}};
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr h_var0{expr::functor{functors.id("h"), {&var0}}};
    expr h_var0_var1{expr::functor{functors.id("h"), {&var0, &var1}}};
    expr f_a{expr::functor{functors.id("f"), {&a_atom}}};
    expr f_b{expr::functor{functors.id("f"), {&b_atom}}};
    expr h_a{expr::functor{functors.id("h"), {&a_atom}}};
    expr h_b{expr::functor{functors.id("h"), {&b_atom}}};

    // Additional expressions for expanded tests
    expr f_var0_var1{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr f_var1_var0{expr::functor{functors.id("f"), {&var1, &var0}}};
    expr g_var0_var1{expr::functor{functors.id("g"), {&var0, &var1}}};
    expr g_var1_var0{expr::functor{functors.id("g"), {&var1, &var0}}};
    expr h_var0_var1_var2{expr::functor{functors.id("h"), {&var0, &var1, &var2}}};
    expr p_var0_var1{expr::functor{functors.id("p"), {&var0, &var1}}};
    expr h_var1{expr::functor{functors.id("h"), {&var1}}};
    expr k_atom{expr::functor{functors.id("k"), {}}};
    expr k_var0{expr::functor{functors.id("k"), {&var0}}};
    expr p_atom{expr::functor{functors.id("p"), {}}};
    expr p_var0{expr::functor{functors.id("p"), {&var0}}};
    expr g_a{expr::functor{functors.id("g"), {&a_atom}}};
    expr g_k{expr::functor{functors.id("g"), {}}};  // alias for clarity in ground tests
    expr f_var0_var0{expr::functor{functors.id("f"), {&var0, &var0}}};
    expr h_var0_var0{expr::functor{functors.id("h"), {&var0, &var0}}};
    expr c_atom{expr::functor{functors.id("c"), {}}};
    expr f_c{expr::functor{functors.id("f"), {&c_atom}}};
    expr k_var2{expr::functor{functors.id("k"), {&var2}}};
    expr f_a_var0{expr::functor{functors.id("f"), {&a_atom, &var0}}};

    db original_db;
    db expanded_db;
    rule_unfolder_manifest manifest{original_db, expanded_db};
};

// ---------------------------------------------------------------------------
// No match — body goal functor not in original db
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, NoMatchErasesSubjectAndReturnsEmpty) {
    // original_db has only g, not f
    original_db.push(rule{&g_atom, {}, 0});

    // expanded_db: h :- f
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_atom}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    EXPECT_THAT(new_ids, IsEmpty());
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
}

// ---------------------------------------------------------------------------
// Single match — fact body goal unfolded away
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SingleMatchFactBodyGoalDisappearsFromNewRule) {
    // original: f :-  (fact)
    original_db.push(rule{&f_atom, {}, 0});

    // expanded: h :- f  (copy original + subject)
    expanded_db.push(rule{&f_atom, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_atom}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_THAT(new_ids, UnorderedElementsAre(new_ids[0]));
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    EXPECT_TRUE(produced->body.empty());
    EXPECT_EQ(produced->var_count, 0u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
}

// ---------------------------------------------------------------------------
// Multiple matches — one new rule per unifying original head
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, TwoMatchesProduceTwoNewRules) {
    // original: f(a) :-  and  f(b) :-
    original_db.push(rule{&f_a, {}, 0});
    original_db.push(rule{&f_b, {}, 0});

    // expanded: same two facts + subject  h(X) :- f(X)
    expanded_db.push(rule{&f_a, {}, 0});
    expanded_db.push(rule{&f_b, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 2u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));

    // Produced heads should be h(a) and h(b) — X was bound to a or b
    const expr* head0 = expanded_db.get_rule(new_ids[0])->head;
    const expr* head1 = expanded_db.get_rule(new_ids[1])->head;
    const bool one_is_ha = expr_eq(head0, &h_a) || expr_eq(head1, &h_a);
    const bool one_is_hb = expr_eq(head0, &h_b) || expr_eq(head1, &h_b);
    EXPECT_TRUE(one_is_ha);
    EXPECT_TRUE(one_is_hb);

    EXPECT_TRUE(expanded_db.get_rule(new_ids[0])->body.empty());
    EXPECT_TRUE(expanded_db.get_rule(new_ids[1])->body.empty());
}

// ---------------------------------------------------------------------------
// Var body goal — triggers lookup of all original rules
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, VarBodyGoalUnifiesWithAllOriginalRules) {
    // original: f :-  and  g :-
    original_db.push(rule{&f_atom, {}, 0});
    original_db.push(rule{&g_atom, {}, 0});

    // expanded: subject  h(X) :- X  (body is a variable)
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 2u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));

    // Both produced rules should be facts (f and g have empty bodies)
    EXPECT_TRUE(expanded_db.get_rule(new_ids[0])->body.empty());
    EXPECT_TRUE(expanded_db.get_rule(new_ids[1])->body.empty());
}

// ---------------------------------------------------------------------------
// Substitution applied to head — bound vars replaced in result
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SubstitutionAppliedToHeadCorrectly) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X) :- f(X)  (var_count=1)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    // Head should be h(a) — var0 was bound to a
    ASSERT_EQ(std::get_if<expr::functor>(&produced->head->content) != nullptr, true);
    const auto& head_f = std::get<expr::functor>(produced->head->content);
    EXPECT_EQ(head_f.id, functors.id("h"));
    ASSERT_EQ(head_f.args.size(), 1u);
    EXPECT_TRUE(expr_eq(head_f.args[0], &a_atom));
    EXPECT_EQ(produced->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Original rule body inlined — body goals inserted at unfolded position
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, OriginalRuleBodyInlinedAtUnfoldedPosition) {
    // original: f(Z) :- g(Z)  (var_count=1)
    original_db.push(rule{&f_var0, {&g_var0}, 1});

    // subject: h(X) :- f(X)  (var_count=1)
    expanded_db.push(rule{&f_var0, {&g_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    // New rule: h(A) :- g(A)  — f(X) replaced by g(Z) with Z→X
    ASSERT_EQ(produced->body.size(), 1u);
    ASSERT_EQ(std::get_if<expr::functor>(&produced->body[0]->content) != nullptr, true);
    const auto& body_f = std::get<expr::functor>(produced->body[0]->content);
    EXPECT_EQ(body_f.id, functors.id("g"));
    // Head and body share the same variable (same index)
    const auto& head_f = std::get<expr::functor>(produced->head->content);
    ASSERT_EQ(head_f.args.size(), 1u);
    ASSERT_EQ(body_f.args.size(), 1u);
    const auto* head_var = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* body_var = std::get_if<expr::var>(&body_f.args[0]->content);
    ASSERT_NE(head_var, nullptr);
    ASSERT_NE(body_var, nullptr);
    EXPECT_EQ(head_var->index, body_var->index);
}

// ---------------------------------------------------------------------------
// Remaining subject body goals preserved around inlined original body
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, RemainingSubjectBodyGoalsPreservedAroundUnfoldedGoal) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h :- g, f(a), h   (unfold at idx 1)
    expr body0 = g_atom;
    expr body2 = h_atom;
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_atom, {&body0, &f_a, &body2}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 1);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    // body: g, h  (f(a) fact has no body, subject's remaining goals preserved)
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_atom));
    EXPECT_TRUE(expr_eq(produced->body[1], &h_atom));
}

// ---------------------------------------------------------------------------
// Unbound variables in head and non-unfolded body get fresh contiguous indices
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, FreeVariablesRenumberedContiguously) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X, Y) :- f(X)  (var_count=2, Y is free)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_var0_var1, {&f_var0}, 2});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    // Head: h(a, Y') — one free var remains
    EXPECT_EQ(produced->var_count, 1u);
    EXPECT_TRUE(produced->body.empty());
    const auto& head_f = std::get<expr::functor>(produced->head->content);
    EXPECT_TRUE(expr_eq(head_f.args[0], &a_atom));
    // Second arg should be a variable (the surviving Y)
    EXPECT_TRUE(std::holds_alternative<expr::var>(head_f.args[1]->content));
}

// ===========================================================================
// A — Variable connections (normalization correctness)
// ===========================================================================

// ---------------------------------------------------------------------------
// Base body var linked to subject var through unification
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, BaseBodyVarLinkedToSubjectVar) {
    // original: f(Z) :- g(Z)  (var_count=1)
    original_db.push(rule{&f_var0, {&g_var0}, 1});

    // subject: h(X) :- f(X)  (var_count=1)
    expanded_db.push(rule{&f_var0, {&g_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Head: h(V0); body: g(V0) — both must share the same var index
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(produced->var_count, 1u);

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& body_f = std::get<expr::functor>(produced->body[0]->content);
    EXPECT_EQ(body_f.id, functors.id("g"));

    ASSERT_EQ(head_f.args.size(), 1u);
    ASSERT_EQ(body_f.args.size(), 1u);
    const auto* hv = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* bv = std::get_if<expr::var>(&body_f.args[0]->content);
    ASSERT_NE(hv, nullptr);
    ASSERT_NE(bv, nullptr);
    EXPECT_EQ(hv->index, bv->index);
}

// ---------------------------------------------------------------------------
// Base head with swapped args — catches wrong normalization frame for base body
//
// If the base body were normalized at frame 0 instead of head_fe.frame_offset,
// the produced rule would be h(V0,V1):-g(V0,V1) instead of h(V0,V1):-g(V1,V0).
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, BaseBodySwappedArgsPreservesConnections) {
    // original: f(var1, var0) :- g(var0, var1)  (head args reversed, var_count=2)
    // A=var0, B=var1 in original's namespace; head is f(B,A), body is g(A,B)
    original_db.push(rule{&f_var1_var0, {&g_var0_var1}, 2});

    // subject: h(X, Y) :- f(X, Y)  (var_count=2)
    expanded_db.push(rule{&f_var1_var0, {&g_var0_var1}, 2});
    const rule_id subject_id = expanded_db.push(rule{&h_var0_var1, {&f_var0_var1}, 2});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Unification: f(X,Y) with f(B,A) binds B→X and A→Y.
    // Base body g(A,B) becomes g(Y,X) which compacts to g(V1,V0)
    // (head h(X,Y) is traversed first so X→V0, Y→V1).
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(produced->var_count, 2u);

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& body_f = std::get<expr::functor>(produced->body[0]->content);
    EXPECT_EQ(body_f.id, functors.id("g"));

    ASSERT_EQ(head_f.args.size(), 2u);
    ASSERT_EQ(body_f.args.size(), 2u);

    const auto* hv0 = std::get_if<expr::var>(&head_f.args[0]->content); // X → V0
    const auto* hv1 = std::get_if<expr::var>(&head_f.args[1]->content); // Y → V1
    const auto* bv0 = std::get_if<expr::var>(&body_f.args[0]->content); // A → Y → V1
    const auto* bv1 = std::get_if<expr::var>(&body_f.args[1]->content); // B → X → V0
    ASSERT_NE(hv0, nullptr);
    ASSERT_NE(hv1, nullptr);
    ASSERT_NE(bv0, nullptr);
    ASSERT_NE(bv1, nullptr);

    // g's first arg (A, bound to Y) must equal head's second arg (Y)
    EXPECT_EQ(bv0->index, hv1->index);
    // g's second arg (B, bound to X) must equal head's first arg (X)
    EXPECT_EQ(bv1->index, hv0->index);
    // and they must be distinct
    EXPECT_NE(hv0->index, hv1->index);
}

// ---------------------------------------------------------------------------
// Multiple base body goals all share the same variable
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, TwoBaseBodyGoalsShareVar) {
    // original: f(Z) :- g(Z), k(Z)  (var_count=1)
    original_db.push(rule{&f_var0, {&g_var0, &k_var0}, 1});

    // subject: h(X) :- f(X)  (var_count=1)
    expanded_db.push(rule{&f_var0, {&g_var0, &k_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Expected: h(V0) :- g(V0), k(V0)  — all three share one var index
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_EQ(produced->var_count, 1u);

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& g_f    = std::get<expr::functor>(produced->body[0]->content);
    const auto& k_f    = std::get<expr::functor>(produced->body[1]->content);
    EXPECT_EQ(g_f.id, functors.id("g"));
    EXPECT_EQ(k_f.id, functors.id("k"));

    const auto* hv = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* gv = std::get_if<expr::var>(&g_f.args[0]->content);
    const auto* kv = std::get_if<expr::var>(&k_f.args[0]->content);
    ASSERT_NE(hv, nullptr);
    ASSERT_NE(gv, nullptr);
    ASSERT_NE(kv, nullptr);
    EXPECT_EQ(hv->index, gv->index);
    EXPECT_EQ(hv->index, kv->index);
}

// ---------------------------------------------------------------------------
// Two base body goals, two distinct variables
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, TwoBaseBodyGoalsTwoDistinctVars) {
    // original: f(A, B) :- g(A), h(B)  (var_count=2) — g(var0), h(var1) in head vars
    expr g_v0{expr::functor{functors.id("g"), {&var0}}};
    expr h_v1{expr::functor{functors.id("h"), {&var1}}};
    original_db.push(rule{&f_var0_var1, {&g_v0, &h_v1}, 2});

    // subject: p(X, Y) :- f(X, Y)  (var_count=2)
    expanded_db.push(rule{&f_var0_var1, {&g_v0, &h_v1}, 2});
    const rule_id subject_id = expanded_db.push(rule{&p_var0_var1, {&f_var0_var1}, 2});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Expected: p(V0, V1) :- g(V0), h(V1)
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_EQ(produced->var_count, 2u);

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& g_body = std::get<expr::functor>(produced->body[0]->content);
    const auto& h_body = std::get<expr::functor>(produced->body[1]->content);
    EXPECT_EQ(g_body.id, functors.id("g"));
    EXPECT_EQ(h_body.id, functors.id("h"));

    // p's first arg (X) and g's arg (A→X) must share an index
    const auto* px = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* py = std::get_if<expr::var>(&head_f.args[1]->content);
    const auto* ga = std::get_if<expr::var>(&g_body.args[0]->content);
    const auto* hb = std::get_if<expr::var>(&h_body.args[0]->content);
    ASSERT_NE(px, nullptr); ASSERT_NE(py, nullptr);
    ASSERT_NE(ga, nullptr); ASSERT_NE(hb, nullptr);
    EXPECT_EQ(px->index, ga->index);
    EXPECT_EQ(py->index, hb->index);
    EXPECT_NE(px->index, py->index);
}

// ===========================================================================
// B — Partial-body-index unfolding
// ===========================================================================

// ---------------------------------------------------------------------------
// Unfold at the first of three body goals
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, UnfoldAtFirstOfThreeGoals) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h :- f(a), g, k  (unfold at idx 0)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_atom, {&f_a, &g_atom, &k_atom}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // f(a) fact has empty body; remaining goals are g and k in that order
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_atom));
    EXPECT_TRUE(expr_eq(produced->body[1], &k_atom));
    EXPECT_EQ(produced->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Unfold at the last of three body goals
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, UnfoldAtLastOfThreeGoals) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h :- g, k, f(a)  (unfold at idx 2)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_atom, {&g_atom, &k_atom, &f_a}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 2);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_atom));
    EXPECT_TRUE(expr_eq(produced->body[1], &k_atom));
    EXPECT_EQ(produced->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Unfold middle goal — base body inserted in place, before/after preserved
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, UnfoldMiddleGoalWithBaseBodyInserted) {
    // original: f(Z) :- g(Z)  (var_count=1)
    original_db.push(rule{&f_var0, {&g_var0}, 1});

    // subject: h(X) :- p, f(X), k  (var_count=1, unfold at idx 1)
    expanded_db.push(rule{&f_var0, {&g_var0}, 1});
    const rule_id subject_id =
        expanded_db.push(rule{&h_var0, {&p_atom, &f_var0, &k_atom}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 1);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Expected body order: p, g(V0), k
    ASSERT_EQ(produced->body.size(), 3u);
    EXPECT_TRUE(expr_eq(produced->body[0], &p_atom));
    EXPECT_EQ(std::get<expr::functor>(produced->body[1]->content).id, functors.id("g"));
    EXPECT_TRUE(expr_eq(produced->body[2], &k_atom));

    // g(V0) and h(V0) share the same var
    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& g_body = std::get<expr::functor>(produced->body[1]->content);
    const auto* hv = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* gv = std::get_if<expr::var>(&g_body.args[0]->content);
    ASSERT_NE(hv, nullptr);
    ASSERT_NE(gv, nullptr);
    EXPECT_EQ(hv->index, gv->index);
}

// ===========================================================================
// C — var_count accuracy
// ===========================================================================

// ---------------------------------------------------------------------------
// Three vars in subject, one bound → two survive, var_count = 2
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, VarCountEqualsNumberOfDistinctFreeVars) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X, Y, Z) :- f(X)  (var_count=3)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_var0_var1_var2, {&f_var0}, 3});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // X was bound to a; Y and Z survive → var_count = 2
    EXPECT_EQ(produced->var_count, 2u);
    EXPECT_TRUE(produced->body.empty());

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    ASSERT_EQ(head_f.args.size(), 3u);
    EXPECT_TRUE(expr_eq(head_f.args[0], &a_atom));
    EXPECT_TRUE(std::holds_alternative<expr::var>(head_f.args[1]->content));
    EXPECT_TRUE(std::holds_alternative<expr::var>(head_f.args[2]->content));

    // Y and Z must have distinct var indices
    const uint32_t iy = std::get<expr::var>(head_f.args[1]->content).index;
    const uint32_t iz = std::get<expr::var>(head_f.args[2]->content).index;
    EXPECT_NE(iy, iz);
}

// ---------------------------------------------------------------------------
// All vars bound → var_count = 0
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, VarCountZeroWhenAllVarsBound) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X) :- f(X)  (var_count=1, X will be bound to a)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    EXPECT_EQ(produced->var_count, 0u);
    EXPECT_TRUE(produced->body.empty());
    EXPECT_TRUE(expr_eq(produced->head, &h_a));
}

// ---------------------------------------------------------------------------
// Two matches → two produced rules each with var_count = 0
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, VarCountZeroInEachOfTwoProducedRules) {
    original_db.push(rule{&f_a, {}, 0});
    original_db.push(rule{&f_b, {}, 0});

    expanded_db.push(rule{&f_a, {}, 0});
    expanded_db.push(rule{&f_b, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 2u);
    EXPECT_EQ(expanded_db.get_rule(new_ids[0])->var_count, 0u);
    EXPECT_EQ(expanded_db.get_rule(new_ids[1])->var_count, 0u);
}

// ===========================================================================
// D — Non-unifying candidates skipped
// ===========================================================================

// ---------------------------------------------------------------------------
// Ground body goal — only the matching original rule produces a new rule
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, PartialMatchOnlyUnifyingCandidatesProduceRules) {
    // original: f(a) :- and f(b) :-
    original_db.push(rule{&f_a, {}, 0});
    original_db.push(rule{&f_b, {}, 0});

    // subject: h :- f(a)  (ground — only f(a) can unify)
    expanded_db.push(rule{&f_a, {}, 0});
    expanded_db.push(rule{&f_b, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_a}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    EXPECT_TRUE(produced->body.empty());
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
}

// ---------------------------------------------------------------------------
// Variable body goal unifies with every original rule
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, VarBodyGoalUnifiesWithEveryOriginalAndBindsHead) {
    // original: f :- and g :-
    original_db.push(rule{&f_atom, {}, 0});
    original_db.push(rule{&g_atom, {}, 0});

    // subject: h(X) :- X  (body is a bare variable, var_count=1)
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 2u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));

    // Both produced rules are facts; heads are h(f) and h(g) (X bound to each fact)
    const rule* r0 = expanded_db.get_rule(new_ids[0]);
    const rule* r1 = expanded_db.get_rule(new_ids[1]);
    EXPECT_TRUE(r0->body.empty());
    EXPECT_TRUE(r1->body.empty());
    EXPECT_EQ(r0->var_count, 0u);
    EXPECT_EQ(r1->var_count, 0u);

    expr h_f{expr::functor{functors.id("h"), {&f_atom}}};
    expr h_g{expr::functor{functors.id("h"), {&g_atom}}};
    const bool one_is_hf = expr_eq(r0->head, &h_f) || expr_eq(r1->head, &h_f);
    const bool one_is_hg = expr_eq(r0->head, &h_g) || expr_eq(r1->head, &h_g);
    EXPECT_TRUE(one_is_hf);
    EXPECT_TRUE(one_is_hg);
}

// ===========================================================================
// E — Ground subject rules
// ===========================================================================

// ---------------------------------------------------------------------------
// Ground subject, fact original → ground fact produced
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, GroundSubjectWithFactOriginalProducesGroundFact) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h :- f(a)  (var_count=0)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_a}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    EXPECT_TRUE(produced->body.empty());
    EXPECT_EQ(produced->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Ground subject, original with body → body inlined verbatim
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, GroundSubjectWithBodyOriginalInlinesBodyVerbatim) {
    // original: f :- g, k
    original_db.push(rule{&f_atom, {&g_atom, &k_atom}, 0});

    // subject: h :- f  (var_count=0)
    expanded_db.push(rule{&f_atom, {&g_atom, &k_atom}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_atom}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);
    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_atom));
    EXPECT_TRUE(expr_eq(produced->body[1], &k_atom));
    EXPECT_EQ(produced->var_count, 0u);
}

// ===========================================================================
// F — Binding propagation across the full rule
// ===========================================================================

// ---------------------------------------------------------------------------
// Binding from the chosen goal propagates backward into before-goals
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SubjectVarBoundByChosenGoalPropagatestoBeforeGoal) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X) :- g(X), f(X)  (var_count=1, unfold at idx 1)
    // X appears in both the before-goal and the chosen goal
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&g_var0, &f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 1);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // X was bound to a; before-goal g(X) and head h(X) must both reflect that
    EXPECT_TRUE(expr_eq(produced->head, &h_a));
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_a));
    EXPECT_EQ(produced->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Binding from the chosen goal propagates forward into after-goals
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SubjectVarBoundByChosenGoalPropagatestoAfterGoal) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X) :- f(X), g(X)  (var_count=1, unfold at idx 0)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0, &g_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    EXPECT_TRUE(expr_eq(produced->head, &h_a));
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_a));
    EXPECT_EQ(produced->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Binding propagates into head and both before- and after-goals simultaneously
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SubjectVarBoundPropagatesHeadAndBothSurroundingGoals) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X) :- g(X), f(X), k(X)  (var_count=1, unfold at idx 1)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_var0, {&g_var0, &f_var0, &k_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 1);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    expr k_a{expr::functor{functors.id("k"), {&a_atom}}};

    EXPECT_TRUE(expr_eq(produced->head, &h_a));
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_TRUE(expr_eq(produced->body[0], &g_a));
    EXPECT_TRUE(expr_eq(produced->body[1], &k_a));
    EXPECT_EQ(produced->var_count, 0u);
}

// ===========================================================================
// G — Base rule introduces new existential (body-only) variables
// ===========================================================================

// ---------------------------------------------------------------------------
// Existential var in base body survives as a brand-new free var in produced rule
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, BaseBodyExistentialVarSurvivesAsNewFreeVar) {
    // original: f(a) :- g(W)  (var_count=1, W only in body — existential)
    original_db.push(rule{&f_a, {&g_var0}, 1});

    // subject: h :- f(a)  (var_count=0, fully ground)
    expanded_db.push(rule{&f_a, {&g_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_a}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // W was existential — not bound by unification, survives as a free var
    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(std::get<expr::functor>(produced->body[0]->content).id, functors.id("g"));
    const auto& g_args = std::get<expr::functor>(produced->body[0]->content).args;
    ASSERT_EQ(g_args.size(), 1u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(g_args[0]->content));
    EXPECT_EQ(produced->var_count, 1u);
}

// ---------------------------------------------------------------------------
// Existential base var shared across multiple base body goals
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, BaseBodyExistentialVarSharedAcrossBaseGoals) {
    // original: f(a) :- g(W), k(W)  (var_count=1, W existential, appears in both body goals)
    original_db.push(rule{&f_a, {&g_var0, &k_var0}, 1});

    // subject: h :- f(a)  (var_count=0)
    expanded_db.push(rule{&f_a, {&g_var0, &k_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_a}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_EQ(produced->var_count, 1u);

    // Both g(W) and k(W) must use the same var index — W is shared
    const auto& g_args = std::get<expr::functor>(produced->body[0]->content).args;
    const auto& k_args = std::get<expr::functor>(produced->body[1]->content).args;
    ASSERT_EQ(g_args.size(), 1u);
    ASSERT_EQ(k_args.size(), 1u);
    const auto* gw = std::get_if<expr::var>(&g_args[0]->content);
    const auto* kw = std::get_if<expr::var>(&k_args[0]->content);
    ASSERT_NE(gw, nullptr);
    ASSERT_NE(kw, nullptr);
    EXPECT_EQ(gw->index, kw->index);
}

// ===========================================================================
// H — Nested functor unification
// ===========================================================================

// ---------------------------------------------------------------------------
// Var connection preserved through nested functor in head and body
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, NestedFunctorDeepVarConnectionPreserved) {
    // original: f(g(Z)) :- p(Z)  (var_count=1; Z buried inside g in the head)
    expr f_g_var0{expr::functor{functors.id("f"), {&g_var0}}};
    original_db.push(rule{&f_g_var0, {&p_var0}, 1});

    // subject: h(X) :- f(g(X))  (var_count=1)
    expanded_db.push(rule{&f_g_var0, {&p_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_g_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Unification thread: g(X)=g(Z) → X↔Z. Both head h(X) and body p(Z) must use same var.
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(produced->var_count, 1u);
    EXPECT_EQ(std::get<expr::functor>(produced->body[0]->content).id, functors.id("p"));

    const auto& head_f  = std::get<expr::functor>(produced->head->content);
    const auto& body_f  = std::get<expr::functor>(produced->body[0]->content);
    const auto* hv = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* bv = std::get_if<expr::var>(&body_f.args[0]->content);
    ASSERT_NE(hv, nullptr);
    ASSERT_NE(bv, nullptr);
    EXPECT_EQ(hv->index, bv->index);
}

// ===========================================================================
// I — Multiple matches, independent var compaction per produced rule
// ===========================================================================

TEST_F(RuleUnfolderTest, MultipleMatchesHaveIndependentVarCounts) {
    // original: f(a) :-  AND  f(Z) :- g(Z)  (two very different rules)
    original_db.push(rule{&f_a, {}, 0});
    original_db.push(rule{&f_var0, {&g_var0}, 1});

    // subject: h(X) :- f(X)  (var_count=1)
    expanded_db.push(rule{&f_a, {}, 0});
    expanded_db.push(rule{&f_var0, {&g_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 2u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));

    // One produced rule is h(a):- (var_count=0), the other h(V0):-g(V0) (var_count=1)
    const rule* r0 = expanded_db.get_rule(new_ids[0]);
    const rule* r1 = expanded_db.get_rule(new_ids[1]);

    const uint32_t vc0 = r0->var_count;
    const uint32_t vc1 = r1->var_count;
    const bool one_is_ground    = (vc0 == 0 || vc1 == 0);
    const bool one_has_free_var = (vc0 == 1 || vc1 == 1);
    EXPECT_TRUE(one_is_ground);
    EXPECT_TRUE(one_has_free_var);

    // The ground rule has head h(a) and empty body
    const rule* ground = (vc0 == 0) ? r0 : r1;
    EXPECT_TRUE(expr_eq(ground->head, &h_a));
    EXPECT_TRUE(ground->body.empty());

    // The rule with a free var has head h(V0) and body g(V0), sharing the var
    const rule* with_var = (vc0 == 1) ? r0 : r1;
    ASSERT_EQ(with_var->body.size(), 1u);
    const auto& wh = std::get<expr::functor>(with_var->head->content);
    const auto& wb = std::get<expr::functor>(with_var->body[0]->content);
    EXPECT_EQ(wb.id, functors.id("g"));
    ASSERT_EQ(wh.args.size(), 1u);
    ASSERT_EQ(wb.args.size(), 1u);
    EXPECT_EQ(std::get<expr::var>(wh.args[0]->content).index,
              std::get<expr::var>(wb.args[0]->content).index);
}

// ===========================================================================
// J — Base head with repeated variable forces subject vars to unify
// ===========================================================================

// ---------------------------------------------------------------------------
// f(Z,Z) in base head forces both h(X,Y) args to collapse to the same var
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, BaseHeadSameVarTwiceForcesSubjectVarsEqual) {
    // original: f(Z, Z) :- g(Z)  (var_count=1; same var in both head args)
    original_db.push(rule{&f_var0_var0, {&g_var0}, 1});

    // subject: h(X, Y) :- f(X, Y)  (var_count=2; X and Y are distinct)
    expanded_db.push(rule{&f_var0_var0, {&g_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0_var1, {&f_var0_var1}, 2});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Unification: X↔Z, Y↔Z  →  X=Y=Z. Head h(X,Y) collapses to h(V0,V0).
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(produced->var_count, 1u);

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& body_f = std::get<expr::functor>(produced->body[0]->content);
    ASSERT_EQ(head_f.args.size(), 2u);
    ASSERT_EQ(body_f.args.size(), 1u);

    const auto* hv0 = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* hv1 = std::get_if<expr::var>(&head_f.args[1]->content);
    const auto* bv  = std::get_if<expr::var>(&body_f.args[0]->content);
    ASSERT_NE(hv0, nullptr);
    ASSERT_NE(hv1, nullptr);
    ASSERT_NE(bv, nullptr);

    // All three must share the same var index — they were all forced equal
    EXPECT_EQ(hv0->index, hv1->index);
    EXPECT_EQ(hv0->index, bv->index);
}

// ===========================================================================
// K — Var only in non-unfolded body goals (never in chosen goal)
// ===========================================================================

// ---------------------------------------------------------------------------
// Var only in a before-goal (not in head or chosen goal) survives in result
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SubjectVarOnlyInBeforeGoalSurvives) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h :- g(Y), f(a)  (var_count=1; Y only in the before-goal)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&g_var0, &f_a}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 1);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // h is ground; g(Y) survives with a fresh var; f(a) is gone
    EXPECT_TRUE(expr_eq(produced->head, &h_atom));
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(std::get<expr::functor>(produced->body[0]->content).id, functors.id("g"));
    const auto& g_args = std::get<expr::functor>(produced->body[0]->content).args;
    ASSERT_EQ(g_args.size(), 1u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(g_args[0]->content));
    EXPECT_EQ(produced->var_count, 1u);
}

// ---------------------------------------------------------------------------
// Var in head and both surrounding non-unfolded goals maps consistently
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SubjectVarInHeadAndNonUnfoldedGoalsGetsConsistentIndex) {
    // original: f(a) :-
    original_db.push(rule{&f_a, {}, 0});

    // subject: h(X) :- g(X), f(a), k(X)  (var_count=1, unfold at idx 1)
    // X is NOT in the chosen goal f(a) — it only lives in head and surrounding goals
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id =
        expanded_db.push(rule{&h_var0, {&g_var0, &f_a, &k_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 1);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // X stays free, gets index 0. Must appear consistently in h, g, and k.
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_EQ(produced->var_count, 1u);

    const auto& head_f = std::get<expr::functor>(produced->head->content);
    const auto& g_body = std::get<expr::functor>(produced->body[0]->content);
    const auto& k_body = std::get<expr::functor>(produced->body[1]->content);

    const auto* hx = std::get_if<expr::var>(&head_f.args[0]->content);
    const auto* gx = std::get_if<expr::var>(&g_body.args[0]->content);
    const auto* kx = std::get_if<expr::var>(&k_body.args[0]->content);
    ASSERT_NE(hx, nullptr); ASSERT_NE(gx, nullptr); ASSERT_NE(kx, nullptr);
    EXPECT_EQ(hx->index, gx->index);
    EXPECT_EQ(hx->index, kx->index);
}

// ===========================================================================
// L — Stress tests: many args, each doing something different
// ===========================================================================

// ---------------------------------------------------------------------------
// 4-arg functor: alternating ground/var args, all var connections preserved
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, StressFourArgFunctor_AlternatingGroundAndVarArgs) {
    // original: f(a, X, b, Y) :- g(X, Y)  (var_count=2)
    // head: positions 0=a(ground), 1=X(var), 2=b(ground), 3=Y(var)
    expr f_a_v0_b_v1{expr::functor{functors.id("f"), {&a_atom, &var0, &b_atom, &var1}}};
    original_db.push(rule{&f_a_v0_b_v1, {&g_var0_var1}, 2});

    // subject: h(P, Q) :- f(a, P, b, Q)  (var_count=2)
    expr h_v0_v1 = h_var0_var1;  // alias for clarity
    expanded_db.push(rule{&f_a_v0_b_v1, {&g_var0_var1}, 2});
    const rule_id subject_id = expanded_db.push(rule{&h_v0_v1, {&f_a_v0_b_v1}, 2});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Unify: a=a ✓, P↔X (→V0), b=b ✓, Q↔Y (→V1)
    // Result: h(V0, V1) :- g(V0, V1), var_count=2
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(produced->var_count, 2u);

    const auto& h_f = std::get<expr::functor>(produced->head->content);
    const auto& g_f = std::get<expr::functor>(produced->body[0]->content);
    ASSERT_EQ(h_f.args.size(), 2u);
    ASSERT_EQ(g_f.args.size(), 2u);

    const auto* hp = std::get_if<expr::var>(&h_f.args[0]->content);
    const auto* hq = std::get_if<expr::var>(&h_f.args[1]->content);
    const auto* gx = std::get_if<expr::var>(&g_f.args[0]->content);
    const auto* gy = std::get_if<expr::var>(&g_f.args[1]->content);
    ASSERT_NE(hp, nullptr); ASSERT_NE(hq, nullptr);
    ASSERT_NE(gx, nullptr); ASSERT_NE(gy, nullptr);

    EXPECT_EQ(hp->index, gx->index);   // P↔X
    EXPECT_EQ(hq->index, gy->index);   // Q↔Y
    EXPECT_NE(hp->index, hq->index);   // P and Q are distinct
}

// ---------------------------------------------------------------------------
// 5 variables doing 5 distinct things in one unfold
//
// Subject: h(W, X, Y) :- f(W, X), k(Y)  — unfold f(W,X) at idx 0
// Original: f(a, A) :- g(A, B)           — A connected to X, B existential
//
// After unfolding:
//   W → bound to 'a'  (erased from free vars)
//   X → connected to A (V0 in head and g body arg 0)
//   Y → free subject var only in after-goal k (V1)
//   A → linked to X, already accounted for
//   B → existential base var, new free var V2 in g body arg 1
//
// Result: h(a, V0, V1) :- g(V0, V2), k(V1)  var_count=3
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, StressFiveVariables_FiveDistinctRoles) {
    // original: f(a, A) :- g(A, B)  (var_count=2; A=var0 in head+body, B=var1 existential)
    original_db.push(rule{&f_a_var0, {&g_var0_var1}, 2});

    // subject: h(W, X, Y) :- f(W, X), k(Y)  (var_count=3, unfold at idx 0)
    // W=var0, X=var1, Y=var2
    expr f_v0_v1{expr::functor{functors.id("f"), {&var0, &var1}}};
    expanded_db.push(rule{&f_a_var0, {&g_var0_var1}, 2});
    const rule_id subject_id =
        expanded_db.push(rule{&h_var0_var1_var2, {&f_v0_v1, &k_var2}, 3});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Expected: h(a, V0, V1) :- g(V0, V2), k(V1)  var_count=3
    ASSERT_EQ(produced->body.size(), 2u);
    EXPECT_EQ(produced->var_count, 3u);

    const auto& h_f = std::get<expr::functor>(produced->head->content);
    const auto& g_f = std::get<expr::functor>(produced->body[0]->content);
    const auto& k_f = std::get<expr::functor>(produced->body[1]->content);
    ASSERT_EQ(h_f.args.size(), 3u);
    ASSERT_EQ(g_f.args.size(), 2u);
    ASSERT_EQ(k_f.args.size(), 1u);

    // h's first arg is 'a' (W was bound)
    EXPECT_TRUE(expr_eq(h_f.args[0], &a_atom));

    // h's second arg (X=V0) must match g's first arg (A=V0)
    const auto* hx = std::get_if<expr::var>(&h_f.args[1]->content);
    const auto* ga = std::get_if<expr::var>(&g_f.args[0]->content);
    ASSERT_NE(hx, nullptr);
    ASSERT_NE(ga, nullptr);
    EXPECT_EQ(hx->index, ga->index);

    // h's third arg (Y=V1) must match k's arg (Y=V1)
    const auto* hy = std::get_if<expr::var>(&h_f.args[2]->content);
    const auto* ky = std::get_if<expr::var>(&k_f.args[0]->content);
    ASSERT_NE(hy, nullptr);
    ASSERT_NE(ky, nullptr);
    EXPECT_EQ(hy->index, ky->index);

    // g's second arg (B=V2) is a new existential — distinct from X and Y
    const auto* gb = std::get_if<expr::var>(&g_f.args[1]->content);
    ASSERT_NE(gb, nullptr);
    EXPECT_NE(gb->index, hx->index);
    EXPECT_NE(gb->index, hy->index);
}

// ---------------------------------------------------------------------------
// Three-level deep nesting: f(g(h(X))) — var buried at the innermost level
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, StressThreeLevelDeepNesting_VarAtInnermost) {
    // original: f(g(h(Z))) :- p(Z)  (var_count=1; Z buried three functors deep)
    expr h_var0_inner{expr::functor{functors.id("h"), {&var0}}};
    expr g_h_var0{expr::functor{functors.id("g"), {&h_var0_inner}}};
    expr f_g_h_var0{expr::functor{functors.id("f"), {&g_h_var0}}};
    original_db.push(rule{&f_g_h_var0, {&p_var0}, 1});

    // subject: q(X) :- f(g(h(X)))  (var_count=1)
    expanded_db.push(rule{&f_g_h_var0, {&p_var0}, 1});
    expr q_var0{expr::functor{functors.id("q"), {&var0}}};
    const rule_id subject_id = expanded_db.push(rule{&q_var0, {&f_g_h_var0}, 1});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    ASSERT_EQ(new_ids.size(), 1u);
    const rule* produced = expanded_db.get_rule(new_ids[0]);

    // Unification propagates through f→g→h to find X↔Z; result q(V0):-p(V0)
    ASSERT_EQ(produced->body.size(), 1u);
    EXPECT_EQ(produced->var_count, 1u);
    EXPECT_EQ(std::get<expr::functor>(produced->body[0]->content).id, functors.id("p"));

    const auto& q_f = std::get<expr::functor>(produced->head->content);
    const auto& p_f = std::get<expr::functor>(produced->body[0]->content);
    EXPECT_EQ(q_f.id, functors.id("q"));
    ASSERT_EQ(q_f.args.size(), 1u);
    ASSERT_EQ(p_f.args.size(), 1u);

    const auto* qv = std::get_if<expr::var>(&q_f.args[0]->content);
    const auto* pv = std::get_if<expr::var>(&p_f.args[0]->content);
    ASSERT_NE(qv, nullptr);
    ASSERT_NE(pv, nullptr);
    EXPECT_EQ(qv->index, pv->index);
}

// ===========================================================================
// M — Refutation: all candidates fail unification → subject erased, 0 new rules
// ===========================================================================

// ---------------------------------------------------------------------------
// Candidate exists for the right functor but argument value mismatches
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, RefutationCandidateExistsButUnificationFails) {
    // original: f(a) :-   (only one candidate, with concrete arg 'a')
    original_db.push(rule{&f_a, {}, 0});

    // subject: h :- f(b)  (b ≠ a → unification fails)
    expanded_db.push(rule{&f_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_b}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    EXPECT_TRUE(new_ids.empty());
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
}

// ---------------------------------------------------------------------------
// Multiple candidates exist but every one fails unification (distinct grounds)
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, RefutationAllThreeCandidatesFailUnification) {
    // original: f(a) :-  f(b) :-  — two candidates, neither matches f(c)
    original_db.push(rule{&f_a, {}, 0});
    original_db.push(rule{&f_b, {}, 0});

    // subject: h :- f(c)
    expanded_db.push(rule{&f_a, {}, 0});
    expanded_db.push(rule{&f_b, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_c}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    EXPECT_TRUE(new_ids.empty());
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
}

// ---------------------------------------------------------------------------
// Nested-functor argument mismatch: functor id at inner level differs
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, RefutationInnerFunctorMismatch) {
    // original: f(g(a)) :-   (inner arg is g(a))
    expr f_g_a{expr::functor{functors.id("f"), {&g_a}}};
    original_db.push(rule{&f_g_a, {}, 0});

    // subject: h :- f(h_atom)  — inner arg is a nullary h, not g(a)
    // (functor id 'h' ≠ 'g', so unification fails at the inner level)
    expr f_h{expr::functor{functors.id("f"), {&h_atom}}};
    expanded_db.push(rule{&f_g_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_atom, {&f_h}, 0});

    const std::vector<rule_id> new_ids = manifest.unfold(subject_id, 0);

    EXPECT_TRUE(new_ids.empty());
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
}

// ===========================================================================
// N — Sequential unfolds on the same manifest
// ===========================================================================

// ---------------------------------------------------------------------------
// Two back-to-back unfolds; second hits a dead end → propagates refutation
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SequentialUnfolds_SecondHitsDeadEnd) {
    // original: f(Z) :- g(Z)   (g is NOT in original — dead end)
    original_db.push(rule{&f_var0, {&g_var0}, 1});

    // expanded starts with a copy + subject h(X) :- f(X)
    expanded_db.push(rule{&f_var0, {&g_var0}, 1});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    // First unfold: h(X):-f(X) → h(V0):-g(V0)
    const std::vector<rule_id> ids1 = manifest.unfold(subject_id, 0);

    ASSERT_EQ(ids1.size(), 1u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));
    // New rule should have g in its body
    EXPECT_EQ(expanded_db.get_rule(ids1[0])->body.size(), 1u);
    EXPECT_EQ(std::get<expr::functor>(expanded_db.get_rule(ids1[0])->body[0]->content).id,
              functors.id("g"));

    // Second unfold: h(V0):-g(V0) → no match (g not in original) → erased
    const std::vector<rule_id> ids2 = manifest.unfold(ids1[0], 0);

    EXPECT_TRUE(ids2.empty());
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(ids1[0]));
}

// ---------------------------------------------------------------------------
// Two sequential unfolds that chain all the way to a ground fact
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SequentialUnfolds_ChainToGroundFact) {
    // original: f(Z) :- g(Z)  AND  g(a) :-
    original_db.push(rule{&f_var0, {&g_var0}, 1});
    original_db.push(rule{&g_a, {}, 0});

    // expanded: same + subject h(X) :- f(X)
    expanded_db.push(rule{&f_var0, {&g_var0}, 1});
    expanded_db.push(rule{&g_a, {}, 0});
    const rule_id subject_id = expanded_db.push(rule{&h_var0, {&f_var0}, 1});

    // First unfold: h(X):-f(X)  →  h(V0):-g(V0)
    const std::vector<rule_id> ids1 = manifest.unfold(subject_id, 0);
    ASSERT_EQ(ids1.size(), 1u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(subject_id));

    // Second unfold: h(V0):-g(V0)  →  h(a):-
    const std::vector<rule_id> ids2 = manifest.unfold(ids1[0], 0);
    ASSERT_EQ(ids2.size(), 1u);
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(ids1[0]));

    const rule* final_rule = expanded_db.get_rule(ids2[0]);
    EXPECT_TRUE(expr_eq(final_rule->head, &h_a));
    EXPECT_TRUE(final_rule->body.empty());
    EXPECT_EQ(final_rule->var_count, 0u);
}

// ---------------------------------------------------------------------------
// Two independent subjects unfolded on the same manifest back-to-back
// ---------------------------------------------------------------------------

TEST_F(RuleUnfolderTest, SequentialUnfolds_TwoIndependentSubjects) {
    // original: f(a) :-  AND  g(b) :-
    original_db.push(rule{&f_a, {}, 0});
    original_db.push(rule{&g_a, {}, 0});  // reuse g_a as a second fact

    // expanded: copies + two subjects
    expanded_db.push(rule{&f_a, {}, 0});
    expanded_db.push(rule{&g_a, {}, 0});
    const rule_id s1 = expanded_db.push(rule{&h_var0, {&f_var0}, 1});
    const rule_id s2 = expanded_db.push(rule{&p_var0, {&g_var0}, 1});

    // Unfold s1: h(X):-f(X)  →  h(a):-
    const std::vector<rule_id> ids1 = manifest.unfold(s1, 0);
    ASSERT_EQ(ids1.size(), 1u);
    EXPECT_TRUE(expr_eq(expanded_db.get_rule(ids1[0])->head, &h_a));
    EXPECT_TRUE(expanded_db.get_rule(ids1[0])->body.empty());

    // Unfold s2: p(Y):-g(Y)  →  p(a):-   (using same manifest, different subject)
    const std::vector<rule_id> ids2 = manifest.unfold(s2, 0);
    ASSERT_EQ(ids2.size(), 1u);
    expr p_a{expr::functor{functors.id("p"), {&a_atom}}};
    EXPECT_TRUE(expr_eq(expanded_db.get_rule(ids2[0])->head, &p_a));
    EXPECT_TRUE(expanded_db.get_rule(ids2[0])->body.empty());

    // Both originals are gone, two new ground facts remain
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(s1));
    EXPECT_FALSE(expanded_db.lookup_all_rules().contains(s2));
}
