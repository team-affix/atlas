#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_refuter.hpp"

using ::testing::NiceMock;

namespace {

struct MockMarkNodeState {
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
};
struct MockPropagateRefutation {
    MOCK_METHOD(void, refute, (pud_node_id));
};

using test_refuter_t = pud_leaf_refuter<NiceMock<MockMarkNodeState>, NiceMock<MockPropagateRefutation>>;

struct PudLeafRefuterTest : public ::testing::Test {
    NiceMock<MockMarkNodeState> states;
    NiceMock<MockPropagateRefutation> propagator;
    test_refuter_t refuter{states, propagator};
};

}

TEST_F(PudLeafRefuterTest, RefuteMarksAndPropagates) {
    EXPECT_CALL(states, mark_refuted(6));
    EXPECT_CALL(propagator, refute(6));
    refuter.refute(6);
}
