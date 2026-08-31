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

    // Composite expressions
    expr f_var0{expr::functor{functors.id("f"), {&var0}}};
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr h_var0{expr::functor{functors.id("h"), {&var0}}};
    expr h_var0_var1{expr::functor{functors.id("h"), {&var0, &var1}}};
    expr f_a{expr::functor{functors.id("f"), {&a_atom}}};
    expr f_b{expr::functor{functors.id("f"), {&b_atom}}};
    expr h_a{expr::functor{functors.id("h"), {&a_atom}}};
    expr h_b{expr::functor{functors.id("h"), {&b_atom}}};

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
