#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_map_insert.hpp"
#include <map>
#include <stdexcept>

class BacktrackableMapInsertTest : public ::testing::Test {
protected:
    std::map<int, int> mp;
    backtrackable_map_insert<std::map<int, int>> m{7, 42};
    void SetUp() override { m.capture(mp); }
};

TEST_F(BacktrackableMapInsertTest, InvokeInsertsEntry) {
    m.invoke();
    EXPECT_EQ(mp.count(7), 1u);
    EXPECT_EQ(mp.at(7), 42);
}

TEST_F(BacktrackableMapInsertTest, InvokeAndBacktrackRemovesEntry) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(mp.count(7), 0u);
}

TEST_F(BacktrackableMapInsertTest, InvokeWithDuplicateKeyThrows) {
    mp.insert({7, 42});
    EXPECT_THROW(m.invoke(), std::logic_error);
}
