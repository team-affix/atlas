#include <gtest/gtest.h>
#include "infrastructure/pud_pending_conflict_store.hpp"

namespace {

struct PudPendingConflictStoreTest : public ::testing::Test {
    pud_pending_conflict_store conflicts;
};

}

TEST_F(PudPendingConflictStoreTest, ANewStoreHasNoPendingConflicts) {
    EXPECT_EQ(conflicts.get(), 0u);
}

TEST_F(PudPendingConflictStoreTest, SetReplacesTheCount) {
    conflicts.set(3);
    EXPECT_EQ(conflicts.get(), 3u);
    conflicts.set(0);
    EXPECT_EQ(conflicts.get(), 0u);
}
