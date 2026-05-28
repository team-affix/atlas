// resolution_memory stores resolutions used in a conflict and derives lemmas with the
// same ancestor-pruning policy as decision_memory. Tests cover empty/clear states,
// multi-insert derivation, and deepest-resolution-only pruning on nested lineages.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include "infrastructure/resolution_memory.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct ResolutionMemoryTest : public ::testing::Test {
    resolution_memory mem;

    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};
};

TEST_F(ResolutionMemoryTest, DeriveLemmaEmptyAfterClear) {
    mem.record_resolution(&rl0);
    mem.record_resolution(&rl1);
    mem.clear_recorded_resolutions();

    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(), IsEmpty());
}

TEST_F(ResolutionMemoryTest, DeriveLemmaEmptyWithNoInsertions) {
    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(), IsEmpty());
}

TEST_F(ResolutionMemoryTest, GetResolutionCountTracksInsertions) {
    EXPECT_EQ(mem.get_resolution_count(), 0u);
    mem.record_resolution(&rl0);
    EXPECT_EQ(mem.get_resolution_count(), 1u);
    mem.record_resolution(&rl1);
    EXPECT_EQ(mem.get_resolution_count(), 2u);
    mem.clear_recorded_resolutions();
    EXPECT_EQ(mem.get_resolution_count(), 0u);
}

TEST_F(ResolutionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.record_resolution(&rl0);
    mem.record_resolution(&rl1);

    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(&rl0, &rl1));
}

TEST_F(ResolutionMemoryTest, DeriveLemmaPrunesAncestorResolutions) {
    resolution_lineage res0{nullptr, 0};
    goal_lineage goal0{&res0, 0};
    resolution_lineage res1{&goal0, 0};
    goal_lineage goal1{&res1, 0};
    resolution_lineage res2{&goal1, 0};

    mem.record_resolution(&res0);
    mem.record_resolution(&res1);
    mem.record_resolution(&res2);

    EXPECT_THAT(mem.derive_resolution_lemma().get_resolutions(), UnorderedElementsAre(&res2));
}
