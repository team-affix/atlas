#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_depth_extender.hpp"
#include "pud_test_node.hpp"

using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockGetNodeDepth {
    MOCK_METHOD(size_t, get_depth, (pud_global_node_id), (const));
};
struct MockInsertNodeDepth {
    MOCK_METHOD(void, insert, (pud_global_node_id, size_t));
};

struct PudNodeDepthExtenderTest : public ::testing::Test {
    StrictMock<MockGetNodeDepth> get_depths;
    StrictMock<MockInsertNodeDepth> insert_depths;
    pud_node_depth_extender<StrictMock<MockGetNodeDepth>, StrictMock<MockInsertNodeDepth>> extender{
        get_depths, insert_depths};
};

}

TEST_F(PudNodeDepthExtenderTest, EachChildIsOneDeeperThanTheUnfoldedParent) {
    EXPECT_CALL(get_depths, get_depth(pud_test_node(4))).WillOnce(Return(3u));
    EXPECT_CALL(insert_depths, insert(pud_test_node(10), 4u));
    EXPECT_CALL(insert_depths, insert(pud_test_node(11), 4u));
    extender.extend(pud_test_node(4), {pud_test_node(10), pud_test_node(11)});
}
