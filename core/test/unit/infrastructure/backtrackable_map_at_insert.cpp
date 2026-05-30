// backtrackable_map_at_insert inserts into an inner set at a map key. Unit tests assert
// insert/backtrack and error cases for missing outer keys and duplicate inner values.

#include <gtest/gtest.h>
#include <map>
#include <set>
#include <stdexcept>
#include "infrastructure/backtrackable_map_at_insert.hpp"

struct BacktrackableMapAtInsertTest : public ::testing::Test {
protected:
    std::map<int, std::set<int>> mp{{1, {}}};
    backtrackable_map_at_insert<std::map<int, std::set<int>>> m{1, 55};
    void SetUp() override { m.capture(mp); }
};

TEST_F(BacktrackableMapAtInsertTest, InvokeInsertsIntoInnerSet) {
    m.invoke();
    EXPECT_EQ(mp.at(1).count(55), 1u);
}

TEST_F(BacktrackableMapAtInsertTest, InvokeAndBacktrackRemovesFromInnerSet) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(mp.at(1).count(55), 0u);
}

TEST_F(BacktrackableMapAtInsertTest, InvokeWithMissingOuterKeyThrows) {
    backtrackable_map_at_insert<std::map<int, std::set<int>>> bad(42, 55);
    bad.capture(mp);
    EXPECT_THROW(bad.invoke(), std::out_of_range);
}

TEST_F(BacktrackableMapAtInsertTest, InvokeWithDuplicateInnerValueThrows) {
    mp.at(1).insert(55);
    EXPECT_THROW(m.invoke(), std::logic_error);
}
