#include <deque>
#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unit_unfold_finder.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindQueryHolder {
    MOCK_METHOD(std::optional<pud_unfold>, find_holder, (pud_mhq_head_id));
};

using unit_queries_t = std::deque<pud_mhq_head_id>;

struct PudUnitUnfoldFinderTest : public ::testing::Test {
    unit_queries_t unit_queries;
    NiceMock<MockFindQueryHolder> holders;
    pud_unit_unfold_finder<unit_queries_t, unit_queries_t, unit_queries_t, NiceMock<MockFindQueryHolder>> finder{
        unit_queries, unit_queries, unit_queries, holders};
};

}

TEST_F(PudUnitUnfoldFinderTest, NoUnitQueryForcesNothing) {
    EXPECT_FALSE(finder.find_unit_unfold().has_value());
}

TEST_F(PudUnitUnfoldFinderTest, TheOldestUnitQueryIsServedWhileALeafHoldsIt) {
    unit_queries = {3, 4};
    ON_CALL(holders, find_holder(3)).WillByDefault(Return(pud_unfold{.leaf = 9, .goal = 1}));
    ON_CALL(holders, find_holder(4)).WillByDefault(Return(pud_unfold{.leaf = 8, .goal = 0}));
    EXPECT_EQ(finder.find_unit_unfold(), (pud_unfold{.leaf = 9, .goal = 1}));
    EXPECT_EQ(unit_queries, (unit_queries_t{3, 4}));
}

TEST_F(PudUnitUnfoldFinderTest, AUnitQueryNoLeafHoldsAnyMoreIsDropped) {
    unit_queries = {3, 4};
    ON_CALL(holders, find_holder(4)).WillByDefault(Return(pud_unfold{.leaf = 8, .goal = 0}));
    EXPECT_EQ(finder.find_unit_unfold(), (pud_unfold{.leaf = 8, .goal = 0}));
    EXPECT_EQ(unit_queries, (unit_queries_t{4}));
}
