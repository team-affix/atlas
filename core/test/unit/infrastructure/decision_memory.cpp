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
    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};
};

TEST_F(DecisionMemoryTest, DeriveLemmaEmptyWithNoInsertions) {
    EXPECT_THAT(mem.derive().get_resolutions(), IsEmpty());
}

TEST_F(DecisionMemoryTest, InsertIncreasesSize) {
    mem.record_decision(&rl0);
    EXPECT_THAT(mem.count(), Eq(1u));
}

TEST_F(DecisionMemoryTest, ClearRemovesAllDecisions) {
    mem.record_decision(&rl0);
    mem.record_decision(&rl1);
    mem.clear_decision_record();
    EXPECT_THAT(mem.count(), Eq(0u));
}

TEST_F(DecisionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.record_decision(&rl0);
    mem.record_decision(&rl1);

    EXPECT_THAT(mem.derive().get_resolutions(),
        UnorderedElementsAre(&rl0, &rl1));
}

TEST_F(DecisionMemoryTest, DeriveLemmaPrunesAncestorResolutions) {
    resolution_lineage res0{nullptr, 0};
    goal_lineage goal0{&res0, 0};
    resolution_lineage res1{&goal0, 0};
    goal_lineage goal1{&res1, 0};
    resolution_lineage res2{&goal1, 0};

    mem.record_decision(&res0);
    mem.record_decision(&res1);
    mem.record_decision(&res2);

    EXPECT_THAT(mem.derive().get_resolutions(), UnorderedElementsAre(&res2));
}
