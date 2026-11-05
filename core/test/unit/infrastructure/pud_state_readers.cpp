#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_state_axioms.hpp"
#include "infrastructure/pud_state_bindings.hpp"
#include "infrastructure/pud_state_depths.hpp"
#include "infrastructure/pud_state_goals.hpp"
#include "infrastructure/pud_state_lvcs.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_node_state.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeState {
    MOCK_METHOD(const pud_node_state&, get, (pud_global_node_id));
};

// The five readers each read one field of the same state of node 4.
struct PudStateReadersTest : public ::testing::Test {
    NiceMock<MockGetNodeState> states;
    pud_state_goals<NiceMock<MockGetNodeState>> goals{states};
    pud_state_axioms<NiceMock<MockGetNodeState>> axioms{states};
    pud_state_depths<NiceMock<MockGetNodeState>> depths{states};
    pud_state_lvcs<NiceMock<MockGetNodeState>> lvcs{states};
    pud_state_bindings<NiceMock<MockGetNodeState>> bindings{states};
    pud_node_state state{
        .goals = pud_goal_list{.goals = {}, .next_id = 6},
        .axiom = pud_test_node(1),
        .depth = 3,
        .lvc = 9,
        .bindings = pud_ubs_state{.parents = ubs_map<uint32_t>{}.set(2, 5), .classes = {}}};

    void SetUp() override { ON_CALL(states, get(pud_test_node(4))).WillByDefault(ReturnRef(state)); }
};

}

TEST_F(PudStateReadersTest, EachReaderReadsItsFieldOfTheNodesState) {
    EXPECT_EQ(goals.get(pud_test_node(4)), state.goals);
    EXPECT_EQ(axioms.get(pud_test_node(4)), pud_test_node(1));
    EXPECT_EQ(depths.get(pud_test_node(4)), 3u);
    EXPECT_EQ(lvcs.get(pud_test_node(4)), 9u);
    EXPECT_EQ(bindings.get(pud_test_node(4)), state.bindings);
}

TEST_F(PudStateReadersTest, AReaderReturnsTheFieldOfTheStateItself) {
    EXPECT_EQ(&goals.get(pud_test_node(4)), &state.goals);
    EXPECT_EQ(&bindings.get(pud_test_node(4)), &state.bindings);
}
