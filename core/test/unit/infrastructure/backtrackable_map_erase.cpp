// backtrackable_map_erase removes a map entry with undo re-insertion. Unit tests assert
// erase, backtrack restore, and missing-key errors.

#include <gtest/gtest.h>
#include "infrastructure/backtrackable_map_erase.hpp"
#include <map>
#include <stdexcept>

struct BacktrackableMapEraseTest : public ::testing::Test {
protected:
    std::map<int, int> mp{{3, 77}};
    backtrackable_map_erase<std::map<int, int>> m{3};
    void SetUp() override { m.capture(mp); }
};

TEST_F(BacktrackableMapEraseTest, InvokeCapturesValueAndErasesEntry) {
    m.invoke();
    EXPECT_EQ(mp.count(3), 0u);
}

TEST_F(BacktrackableMapEraseTest, InvokeAndBacktrackReInsertsWithCorrectValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(mp.count(3), 1u);
    EXPECT_EQ(mp.at(3), 77);
}

TEST_F(BacktrackableMapEraseTest, InvokeWithMissingKeyThrows) {
    backtrackable_map_erase<std::map<int, int>> bad(42);
    bad.capture(mp);
    EXPECT_THROW(bad.invoke(), std::out_of_range);
}
