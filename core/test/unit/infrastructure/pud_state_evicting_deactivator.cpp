#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_state_evicting_deactivator.hpp"
#include "pud_test_node.hpp"

using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {

struct MockEvictNodeState {
    MOCK_METHOD(void, evict, (pud_global_node_id));
};

}

TEST(PudStateEvictingDeactivatorTest, TheNodesStateIsDroppedAndNoLeafIsStranded) {
    NiceMock<MockEvictNodeState> states;
    pud_state_evicting_deactivator<NiceMock<MockEvictNodeState>> deactivator{states};
    EXPECT_CALL(states, evict(pud_test_node(3)));
    EXPECT_THAT(deactivator.deactivate(pud_test_node(3)), IsEmpty());
}
