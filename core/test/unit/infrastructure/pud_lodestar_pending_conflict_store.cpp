#include <gtest/gtest.h>
#include "infrastructure/pud_lodestar_pending_conflict_store.hpp"

namespace {

struct PudLodestarPendingConflictStoreTest : public ::testing::Test {
    pud_lodestar_pending_conflict_store conflicts;
};

}

TEST_F(PudLodestarPendingConflictStoreTest, ANewStoreHasNoPendingConflicts) {
    EXPECT_EQ(conflicts.get(), 0u);
}

TEST_F(PudLodestarPendingConflictStoreTest, SetReplacesTheCount) {
    conflicts.set(3);
    EXPECT_EQ(conflicts.get(), 3u);
    conflicts.set(0);
    EXPECT_EQ(conflicts.get(), 0u);
}
