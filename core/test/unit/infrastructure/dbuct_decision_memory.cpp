// dbuct_decision_memory: record/count/derive lemma (no ancestor prune) and frame undo.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_decision_memory.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct DbuctDecisionMemoryTest : public ::testing::Test {
    dbuct_decision_memory mem;
    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};
};

TEST_F(DbuctDecisionMemoryTest, DeriveLemmaEmptyWithNoInsertions) {
    EXPECT_THAT(mem.derive_decision_lemma().get_resolutions(), IsEmpty());
    EXPECT_EQ(mem.count(), 0u);
}

TEST_F(DbuctDecisionMemoryTest, RecordIncreasesCount) {
    mem.push_frame();
    mem.record_decision(&rl0);
    EXPECT_EQ(mem.count(), 1u);
}

TEST_F(DbuctDecisionMemoryTest, DuplicateRecordIsIdempotent) {
    mem.push_frame();
    mem.record_decision(&rl0);
    mem.record_decision(&rl0);
    EXPECT_EQ(mem.count(), 1u);
}

TEST_F(DbuctDecisionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.push_frame();
    mem.record_decision(&rl0);
    mem.record_decision(&rl1);
    EXPECT_THAT(mem.derive_decision_lemma().get_resolutions(),
        UnorderedElementsAre(&rl0, &rl1));
}

TEST_F(DbuctDecisionMemoryTest, DeriveLemmaKeepsDeepestOnLineageChain) {
    resolution_lineage res0{nullptr, 0};
    goal_lineage goal0{&res0, 0};
    resolution_lineage res1{&goal0, 0};

    mem.push_frame();
    mem.record_decision(&res0);
    mem.record_decision(&res1);
    // lemma construction prunes ancestor resolutions from the returned set
    EXPECT_THAT(mem.derive_decision_lemma().get_resolutions(), UnorderedElementsAre(&res1));
}

TEST_F(DbuctDecisionMemoryTest, PopFrameRestoresPriorState) {
    mem.push_frame();
    mem.record_decision(&rl0);
    mem.push_frame();
    mem.record_decision(&rl1);
    EXPECT_EQ(mem.count(), 2u);
    mem.pop_frame();
    EXPECT_EQ(mem.count(), 1u);
    EXPECT_THAT(mem.derive_decision_lemma().get_resolutions(), UnorderedElementsAre(&rl0));
}
