#include <gtest/gtest.h>
#include "infrastructure/pud_pending_conflict_store.hpp"

TEST(PudPendingConflictStoreTest, ANewStoreHasNothingToTake) {
    pud_pending_conflict_store store;
    EXPECT_FALSE(store.try_take());
}

TEST(PudPendingConflictStoreTest, EachAddedConflictIsTakenOnce) {
    pud_pending_conflict_store store;
    store.add(2);
    store.add(1);
    EXPECT_TRUE(store.try_take());
    EXPECT_TRUE(store.try_take());
    EXPECT_TRUE(store.try_take());
    EXPECT_FALSE(store.try_take());
}
