#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lvc_descent.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetNodeAddedVarCount {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};

using test_descent_t = pud_lvc_descent<MockGetNodeAddedVarCount>;

struct PudLvcDescentTest : public ::testing::Test {
    NiceMock<MockGetNodeAddedVarCount> get_var_count;
    pud_node_id root = 7;
    pud_node_id child = 2;
    pud_node_id sibling = 3;
};

TEST_F(PudLvcDescentTest, RootUsesRootVarCount) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(3));
    test_descent_t descent{get_var_count, root};
    EXPECT_EQ(descent.lvc(), 3u);
}

TEST_F(PudLvcDescentTest, DescendAccumulatesChildVarCount) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(5));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(2));
    test_descent_t at_root{get_var_count, root};
    EXPECT_EQ(at_root.descend(child).lvc(), 7u);
}

TEST_F(PudLvcDescentTest, DescendAccumulatesAcrossLevels) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(1));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(2));
    EXPECT_CALL(get_var_count, get(sibling)).WillOnce(Return(4));
    test_descent_t at_root{get_var_count, root};
    EXPECT_EQ(at_root.descend(child).descend(sibling).lvc(), 7u);
}

TEST_F(PudLvcDescentTest, DescendLeavesParentUntouched) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(5));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(2));
    test_descent_t at_root{get_var_count, root};
    at_root.descend(child);
    EXPECT_EQ(at_root.lvc(), 5u);
}

TEST_F(PudLvcDescentTest, SiblingsDescendIndependently) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(1));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(2));
    EXPECT_CALL(get_var_count, get(sibling)).WillOnce(Return(10));
    test_descent_t at_root{get_var_count, root};
    EXPECT_EQ(at_root.descend(child).lvc(), 3u);
    EXPECT_EQ(at_root.descend(sibling).lvc(), 11u);
}

TEST_F(PudLvcDescentTest, RootQueriesOnlyTheRoot) {
    ::testing::StrictMock<MockGetNodeAddedVarCount> strict_get_var_count;
    EXPECT_CALL(strict_get_var_count, get(root)).WillOnce(Return(4));
    test_descent_t descent{strict_get_var_count, root};
    EXPECT_EQ(descent.lvc(), 4u);
}

TEST_F(PudLvcDescentTest, DescendQueriesOnlyTheChild) {
    ::testing::StrictMock<MockGetNodeAddedVarCount> strict_get_var_count;
    EXPECT_CALL(strict_get_var_count, get(root)).WillOnce(Return(4));
    test_descent_t at_root{strict_get_var_count, root};
    EXPECT_CALL(strict_get_var_count, get(child)).WillOnce(Return(1));
    EXPECT_EQ(at_root.descend(child).lvc(), 5u);
}

TEST_F(PudLvcDescentTest, RootWithNoVarsStartsAtZero) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(0));
    EXPECT_EQ((test_descent_t{get_var_count, root}).lvc(), 0u);
}

TEST_F(PudLvcDescentTest, ChildWithNoVarsKeepsTheCount) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(6));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(0));
    test_descent_t at_root{get_var_count, root};
    EXPECT_EQ(at_root.descend(child).lvc(), 6u);
}

TEST_F(PudLvcDescentTest, DeepChainAccumulatesEveryLevel) {
    const uint32_t depth = 1000;
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(2));
    test_descent_t at{get_var_count, root};
    for (uint32_t level = 1; level <= depth; ++level) {
        const pud_node_id level_node = 100 + level;
        EXPECT_CALL(get_var_count, get(level_node)).WillOnce(Return(level % 4));
        at = at.descend(level_node);
    }
    uint32_t expected = 2;
    for (uint32_t level = 1; level <= depth; ++level)
        expected += level % 4;
    EXPECT_EQ(at.lvc(), expected);
}

TEST_F(PudLvcDescentTest, DescentIsAssignable) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(1));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(2));
    test_descent_t current{get_var_count, root};
    current = current.descend(child);
    EXPECT_EQ(current.lvc(), 3u);
}

}  // namespace
