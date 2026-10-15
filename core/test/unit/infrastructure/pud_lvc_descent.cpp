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

TEST_F(PudLvcDescentTest, DescentIsAssignable) {
    EXPECT_CALL(get_var_count, get(root)).WillOnce(Return(1));
    EXPECT_CALL(get_var_count, get(child)).WillOnce(Return(2));
    test_descent_t current{get_var_count, root};
    current = current.descend(child);
    EXPECT_EQ(current.lvc(), 3u);
}

}  // namespace
