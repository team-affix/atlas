// dbuct_resolution_memory: record/count/derive lemma and frame undo.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_resolution_memory.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct DbuctResolutionMemoryTest : public ::testing::Test {
    dbuct_resolution_memory mem;
    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};
};

TEST_F(DbuctResolutionMemoryTest, DeriveLemmaEmptyWithNoInsertions) {
    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(), IsEmpty());
    EXPECT_EQ(mem.get_resolution_count(), 0u);
}

TEST_F(DbuctResolutionMemoryTest, RecordIncreasesCount) {
    mem.push_frame();
    mem.record_resolution(&rl0);
    EXPECT_EQ(mem.get_resolution_count(), 1u);
}

TEST_F(DbuctResolutionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.push_frame();
    mem.record_resolution(&rl0);
    mem.record_resolution(&rl1);
    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(&rl0, &rl1));
}

TEST_F(DbuctResolutionMemoryTest, PopFrameRestoresPriorState) {
    mem.push_frame();
    mem.record_resolution(&rl0);
    mem.push_frame();
    mem.record_resolution(&rl1);
    EXPECT_EQ(mem.get_resolution_count(), 2u);
    mem.pop_frame();
    EXPECT_EQ(mem.get_resolution_count(), 1u);
    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(), UnorderedElementsAre(&rl0));
}
