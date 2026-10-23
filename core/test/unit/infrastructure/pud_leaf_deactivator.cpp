#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_deactivator.hpp"

using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockReleaseLeaf {
    MOCK_METHOD(void, release, (pud_node_id));
};
struct MockInvalidateQueryLeaf {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, invalidate_leaf, (pud_node_id));
};

using test_deactivator_t = pud_leaf_deactivator<NiceMock<MockReleaseLeaf>, NiceMock<MockInvalidateQueryLeaf>>;

struct PudLeafDeactivatorTest : public ::testing::Test {
    NiceMock<MockReleaseLeaf> releaser;
    NiceMock<MockInvalidateQueryLeaf> queries;
    test_deactivator_t deactivator{releaser, queries};
};

}

TEST_F(PudLeafDeactivatorTest, DeactivateReleasesThenInvalidatesThatLeaf) {
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(queries, invalidate_leaf(4u)).WillOnce(Return(std::vector<pud_mhq_head_id>{30}));
    EXPECT_THAT(deactivator.deactivate(4), ElementsAre(30u));
}
