#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/decision_memory.hpp"

struct DecisionMemoryTest : public ::testing::Test {
protected:
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    resolution_lineage rl0{nullptr, &rule0};
    resolution_lineage rl1{nullptr, &rule1};
};

TEST_F(DecisionMemoryTest, InsertIncreasesSize) {
    decision_memory mem;
    mem.insert(&rl0);
    EXPECT_EQ(mem.size(), 1u);
}

TEST_F(DecisionMemoryTest, ClearRemovesAllDecisions) {
    decision_memory mem;
    mem.insert(&rl0);
    mem.insert(&rl1);
    mem.clear();
    EXPECT_EQ(mem.size(), 0u);
}

TEST_F(DecisionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    decision_memory mem;
    mem.insert(&rl0);
    mem.insert(&rl1);

    lemma l = mem.derive_lemma();
    EXPECT_EQ(l.get_resolutions().size(), 2u);
    EXPECT_TRUE(l.get_resolutions().contains(&rl0));
    EXPECT_TRUE(l.get_resolutions().contains(&rl1));
}
