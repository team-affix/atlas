#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_unit_goal_finder.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};
using test_node_queries_t = pud_node_queries<NiceMock<MockRemoveQuery>>;
using test_owner_box_t = immer::box<pud_mhq_head_owner<NiceMock<MockRemoveQuery>>>;

struct MockFindQueryIntroduction {
    MOCK_METHOD(std::optional<pud_query_introduction>, find_introduction, (pud_mhq_head_id), (const));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id), (const));
};
struct MockFindNodeQueries {
    MOCK_METHOD(const test_node_queries_t*, find, (pud_global_node_id), (const));
};

using test_finder_t = pud_lodestar_unit_goal_finder<
    NiceMock<MockFindQueryIntroduction>, NiceMock<MockGetQueryCandidates>, NiceMock<MockFindNodeQueries>>;

// Leaf 5 holds query 40 on goal 2, query 41 on goal 7 and query 42 on goal 3. Queries 40, 41 and 42 were
// introduced on those goals, and each has a single candidate unless a test says otherwise.
struct PudLodestarUnitGoalFinderTest : public ::testing::Test {
    PudLodestarUnitGoalFinderTest();
    NiceMock<MockRemoveQuery> remover;
    NiceMock<MockFindQueryIntroduction> introducers;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockFindNodeQueries> node_queries;
    test_finder_t finder;
    test_node_queries_t held;

    void SetUp() override {
        ON_CALL(introducers, find_introduction(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(introducers, find_introduction(40))
            .WillByDefault(Return(pud_query_introduction{.node = pud_test_node(1), .goal = 2}));
        ON_CALL(introducers, find_introduction(41))
            .WillByDefault(Return(pud_query_introduction{.node = pud_test_node(1), .goal = 7}));
        ON_CALL(introducers, find_introduction(42))
            .WillByDefault(Return(pud_query_introduction{.node = pud_test_node(1), .goal = 3}));
        ON_CALL(candidates, candidates(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30}));
        ON_CALL(node_queries, find(_)).WillByDefault(Return(nullptr));
        ON_CALL(node_queries, find(pud_test_node(5))).WillByDefault(Return(&held));
    }
};

PudLodestarUnitGoalFinderTest::PudLodestarUnitGoalFinderTest()
    : remover()
    , introducers()
    , candidates()
    , node_queries()
    , finder(introducers, candidates, node_queries)
    , held(test_node_queries_t{}
          .set(2, test_owner_box_t{remover, 40})
          .set(7, test_owner_box_t{remover, 41})
          .set(3, test_owner_box_t{remover, 42})) {}

}

TEST_F(PudLodestarUnitGoalFinderTest, NoUnitQueryGivesNoUnitGoal) {
    EXPECT_FALSE(finder.find_unit_goal(pud_test_node(5)).has_value());
}

TEST_F(PudLodestarUnitGoalFinderTest, AUnitQueryTheLeafHoldsGivesItsGoal) {
    finder.push_back(41);
    EXPECT_EQ(finder.find_unit_goal(pud_test_node(5)), 7u);
}

TEST_F(PudLodestarUnitGoalFinderTest, TheSmallestOfTheLeafsUnitGoalsIsFound) {
    finder.push_back(41);
    finder.push_back(42);
    finder.push_back(40);
    EXPECT_EQ(finder.find_unit_goal(pud_test_node(5)), 2u);
}

TEST_F(PudLodestarUnitGoalFinderTest, AUnitQueryTheLeafDoesNotHoldGivesNothing) {
    finder.push_back(43);
    ON_CALL(introducers, find_introduction(43))
        .WillByDefault(Return(pud_query_introduction{.node = pud_test_node(1), .goal = 2}));
    finder.push_back(44);
    ON_CALL(introducers, find_introduction(44))
        .WillByDefault(Return(pud_query_introduction{.node = pud_test_node(1), .goal = 9}));
    EXPECT_FALSE(finder.find_unit_goal(pud_test_node(5)).has_value());
}

TEST_F(PudLodestarUnitGoalFinderTest, ALeafWithoutQueriesHasNoUnitGoal) {
    finder.push_back(40);
    EXPECT_FALSE(finder.find_unit_goal(pud_test_node(6)).has_value());
}

TEST_F(PudLodestarUnitGoalFinderTest, ARemovedQueryIsDroppedAndNotAskedAgain) {
    finder.push_back(40);
    ON_CALL(introducers, find_introduction(40)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(finder.find_unit_goal(pud_test_node(5)).has_value());
    EXPECT_CALL(introducers, find_introduction(40)).Times(0);
    finder.find_unit_goal(pud_test_node(5));
}

TEST_F(PudLodestarUnitGoalFinderTest, AQueryNoLongerUnitIsDroppedAndNotAskedAgain) {
    finder.push_back(40);
    ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{}));
    EXPECT_FALSE(finder.find_unit_goal(pud_test_node(5)).has_value());
    EXPECT_CALL(introducers, find_introduction(40)).Times(0);
    finder.find_unit_goal(pud_test_node(5));
}

TEST_F(PudLodestarUnitGoalFinderTest, AUnitQueryStaysKnownForOtherLeaves) {
    finder.push_back(40);
    EXPECT_FALSE(finder.find_unit_goal(pud_test_node(6)).has_value());
    EXPECT_EQ(finder.find_unit_goal(pud_test_node(5)), 2u);
}
