#include <gtest/gtest.h>
#include <unordered_set>
#include "../../../core/hpp/infrastructure/decision_memory.hpp"

struct DecisionMemoryTest : public ::testing::Test {
protected:
    decision_memory mem;

    expr goal_expr0{expr::var{0}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    resolution_lineage rl0{nullptr, &rule0};
    resolution_lineage rl1{nullptr, &rule1};
};

TEST_F(DecisionMemoryTest, DeriveLemmaEmptyWithNoInsertions) {
    lemma l = mem.derive_lemma();
    EXPECT_TRUE(l.get_resolutions().empty());
}

TEST_F(DecisionMemoryTest, InsertIncreasesSize) {
    mem.insert(&rl0);
    EXPECT_EQ(mem.size(), 1u);
}

TEST_F(DecisionMemoryTest, ClearRemovesAllDecisions) {
    mem.insert(&rl0);
    mem.insert(&rl1);
    mem.clear();
    EXPECT_EQ(mem.size(), 0u);
}

TEST_F(DecisionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.insert(&rl0);
    mem.insert(&rl1);

    lemma l = mem.derive_lemma();
    EXPECT_EQ(l.get_resolutions().size(), 2u);
    EXPECT_TRUE(l.get_resolutions().contains(&rl0));
    EXPECT_TRUE(l.get_resolutions().contains(&rl1));
}

TEST_F(DecisionMemoryTest, DeriveLemmaPrunesAncestorResolutions) {
    resolution_lineage res0{nullptr, &rule0};
    goal_lineage goal0{&res0, &goal_expr0};
    resolution_lineage res1{&goal0, &rule0};
    goal_lineage goal1{&res1, &goal_expr0};
    resolution_lineage res2{&goal1, &rule0};

    mem.insert(&res0);
    mem.insert(&res1);
    mem.insert(&res2);

    lemma l = mem.derive_lemma();
    EXPECT_EQ(l.get_resolutions(), (std::unordered_set<const resolution_lineage*>{&res2}));
}
