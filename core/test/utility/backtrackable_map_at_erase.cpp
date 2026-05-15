#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_map_at_erase.hpp"
#include <map>
#include <set>
#include <stdexcept>

class BacktrackableMapAtEraseTest : public ::testing::Test {
protected:
    std::map<int, std::set<int>> mp{{1, {55}}};
    backtrackable_map_at_erase<std::map<int, std::set<int>>> m{1, 55};
    void SetUp() override { m.capture(mp); }
};

TEST_F(BacktrackableMapAtEraseTest, InvokeErasesFromInnerSet) {
    m.invoke();
    EXPECT_EQ(mp.at(1).count(55), 0u);
}

TEST_F(BacktrackableMapAtEraseTest, InvokeAndBacktrackReInsertsIntoInnerSet) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(mp.at(1).count(55), 1u);
}

TEST_F(BacktrackableMapAtEraseTest, InvokeWithMissingOuterKeyThrows) {
    backtrackable_map_at_erase<std::map<int, std::set<int>>> bad(42, 55);
    bad.capture(mp);
    EXPECT_THROW(bad.invoke(), std::out_of_range);
}

TEST_F(BacktrackableMapAtEraseTest, InvokeWithMissingInnerValueThrows) {
    mp.at(1).erase(55);
    EXPECT_THROW(m.invoke(), std::logic_error);
}
