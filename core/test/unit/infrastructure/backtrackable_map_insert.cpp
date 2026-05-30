// backtrackable_map_insert records a map insert for trail undo. Unit tests assert insert,
// backtrack removal, and duplicate-key failure.

#include <gtest/gtest.h>
#include <map>
#include <stdexcept>
#include "infrastructure/backtrackable_map_insert.hpp"

struct BacktrackableMapInsertTest : public ::testing::Test {
    static constexpr int kKey = 7;
    static constexpr int kValue = 42;

    std::map<int, int> mp;
    backtrackable_map_insert<std::map<int, int>> m{kKey, kValue};

    void SetUp() override { m.capture(mp); }
};

TEST_F(BacktrackableMapInsertTest, InvokeInsertsEntry) {
    m.invoke();
    EXPECT_EQ(mp.count(kKey), 1u);
    EXPECT_EQ(mp.at(kKey), kValue);
}

TEST_F(BacktrackableMapInsertTest, InvokeAndBacktrackRemovesEntry) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(mp.count(kKey), 0u);
}

TEST_F(BacktrackableMapInsertTest, InvokeWithDuplicateKeyThrows) {
    mp.insert({kKey, kValue});
    EXPECT_THROW(m.invoke(), std::logic_error);
}
