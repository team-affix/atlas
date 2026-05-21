#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/decision_memory.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"

class DecisionMemoryTest : public ::testing::Test {
protected:
    lineage_pool pool;
    expr goal_e{expr::var{0}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
};

TEST_F(DecisionMemoryTest, InsertIncreasesSize) {
    decision_memory mem;
    const resolution_lineage* rl = pool.resolution(nullptr, &rule0);
    mem.insert(rl);
    EXPECT_EQ(mem.size(), 1u);
}

TEST_F(DecisionMemoryTest, ClearRemovesAllDecisions) {
    decision_memory mem;
    mem.insert(pool.resolution(nullptr, &rule0));
    mem.insert(pool.resolution(nullptr, &rule1));
    mem.clear();
    EXPECT_EQ(mem.size(), 0u);
}

TEST_F(DecisionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    decision_memory mem;
    const resolution_lineage* rl0 = pool.resolution(nullptr, &rule0);
    const resolution_lineage* rl1 = pool.resolution(nullptr, &rule1);
    mem.insert(rl0);
    mem.insert(rl1);

    lemma l = mem.derive_lemma();
    EXPECT_EQ(l.get_resolutions().size(), 2u);
    EXPECT_TRUE(l.get_resolutions().contains(rl0));
    EXPECT_TRUE(l.get_resolutions().contains(rl1));
}
