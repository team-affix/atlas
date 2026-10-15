#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lvc_descender.hpp"

namespace {

struct MockGetNodeAddedVarCount {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};

struct PudLvcDescenderTest : public ::testing::Test {
    ::testing::NiceMock<MockGetNodeAddedVarCount> get_var_count;
    pud_lvc_descender<MockGetNodeAddedVarCount> descender{get_var_count};
};

TEST_F(PudLvcDescenderTest, RootDescentUsesRootVarCount) {
    const pud_node_id root = 7;
    EXPECT_CALL(get_var_count, get(root)).WillOnce(::testing::Return(3));
    pud_lvc_descent descent = descender.root_descent(root);
    EXPECT_EQ(descent.lvc, 3u);
}

TEST_F(PudLvcDescenderTest, DescendAccumulatesChildVarCount) {
    const pud_node_id child = 2;
    pud_lvc_descent at_parent{.lvc = 5};
    EXPECT_CALL(get_var_count, get(child)).WillOnce(::testing::Return(2));
    pud_lvc_descent at_child = descender.descend(at_parent, child);
    EXPECT_EQ(at_child.lvc, 7u);
}

}
