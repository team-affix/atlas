// seen_solutions: post-sim store of resolution lemmas. Lookup canonicalizes the
// lemma's interned-pointer set by sorting (unordered_set iteration is not a key).

#include <unordered_set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/seen_solutions.hpp"
#include "value_objects/lemma.hpp"

struct SeenSolutionsTest : public ::testing::Test {
    seen_solutions store;
    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};
    resolution_lineage rl2{nullptr, 2};
};

TEST_F(SeenSolutionsTest, UnseenLemmaIsNotARepeat) {
    lemma l{{&rl0}};
    EXPECT_FALSE(store.is_repeat_solution(l));
}

TEST_F(SeenSolutionsTest, RememberThenSameLemmaIsRepeat) {
    lemma l{{&rl0, &rl1}};
    store.remember_solution(l);
    EXPECT_TRUE(store.is_repeat_solution(l));
}

TEST_F(SeenSolutionsTest, DifferentLemmaIsNotARepeat) {
    lemma first{{&rl0}};
    lemma second{{&rl1}};
    store.remember_solution(first);
    EXPECT_FALSE(store.is_repeat_solution(second));
}

TEST_F(SeenSolutionsTest, SameSetDifferentInsertOrderIsRepeat) {
    lemma first{{&rl0, &rl1}};
    lemma second{{&rl1, &rl0}};
    store.remember_solution(first);
    EXPECT_TRUE(store.is_repeat_solution(second));
}

TEST_F(SeenSolutionsTest, EmptyLemmaCanBeRemembered) {
    lemma empty{std::unordered_set<const resolution_lineage*>{}};
    EXPECT_FALSE(store.is_repeat_solution(empty));
    store.remember_solution(empty);
    EXPECT_TRUE(store.is_repeat_solution(empty));
    lemma nonempty{{&rl2}};
    EXPECT_FALSE(store.is_repeat_solution(nonempty));
}
