// decision_memory records chosen resolutions and derives avoidance lemmas, pruning
// ancestor resolutions on the same goal chain. Tests assert size/clear behavior and that
// derive_lemma keeps only deepest resolutions per lineage branch.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include "../../../core/hpp/infrastructure/decision_memory.hpp"

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct DecisionMemoryTest : public ::testing::Test {
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
    EXPECT_THAT(mem.derive_lemma().get_resolutions(), IsEmpty());
}

TEST_F(DecisionMemoryTest, InsertIncreasesSize) {
    mem.insert(&rl0);
    EXPECT_THAT(mem.size(), Eq(1u));
}

TEST_F(DecisionMemoryTest, ClearRemovesAllDecisions) {
    mem.insert(&rl0);
    mem.insert(&rl1);
    mem.clear();
    EXPECT_THAT(mem.size(), Eq(0u));
}

TEST_F(DecisionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.insert(&rl0);
    mem.insert(&rl1);

    EXPECT_THAT(mem.derive_lemma().get_resolutions(),
        UnorderedElementsAre(&rl0, &rl1));
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

    EXPECT_THAT(mem.derive_lemma().get_resolutions(), UnorderedElementsAre(&res2));
}
