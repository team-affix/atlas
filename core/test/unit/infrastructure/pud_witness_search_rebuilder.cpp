#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_rebuilder.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_global_node_id));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}

// Below the search root 0: children 1, 2, 3; below 2: children 4, 5; below 5: child 6.
struct PudWitnessSearchRebuilderTest : public ::testing::Test {
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockGetChildren> children;
    pud_witness_search_rebuilder<NiceMock<MockDescendReplay>, NiceMock<MockGetChildren>> rebuilder{descend, children};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists{{0, {1, 2, 3}}, {2, {4, 5}}, {5, {6}}};
    pud_replay root{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}};

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault(Invoke(&pud_enter_child));
        for (const auto& [node, listed] : child_lists)
            ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(child_lists.at(node)));
    }
};

}

TEST_F(PudWitnessSearchRebuilderTest, EachFrameStandsOnItsChainNodeAndContinuesAfterIt) {
    const pud_witness_search_stack search =
        rebuilder.rebuild(root, {pud_test_node(2), pud_test_node(5), pud_test_node(6)});
    ASSERT_EQ(search.frames.size(), 3u);
    EXPECT_EQ(search.frames.at(0).current->node, pud_test_node(2));
    EXPECT_EQ(search.frames.at(0).next_sibling, 2u);
    EXPECT_EQ(search.frames.at(1).current->node, pud_test_node(5));
    EXPECT_EQ(search.frames.at(1).next_sibling, 2u);
    EXPECT_EQ(search.frames.at(2).current->node, pud_test_node(6));
    EXPECT_EQ(search.frames.at(2).next_sibling, 1u);
}

TEST_F(PudWitnessSearchRebuilderTest, EachNodeIsReplayedFromTheOneAboveIt) {
    EXPECT_CALL(descend, descend(Field(&pud_replay::node, pud_test_node(0)), pud_test_node(2)));
    EXPECT_CALL(descend, descend(Field(&pud_replay::node, pud_test_node(2)), pud_test_node(5)));
    rebuilder.rebuild(root, {pud_test_node(2), pud_test_node(5)});
}

TEST_F(PudWitnessSearchRebuilderTest, ANodeThatNoLongerReplaysEndsTheStackWithoutANode) {
    ON_CALL(descend, descend(_, pud_test_node(5))).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, _)).Times(AnyNumber());
    EXPECT_CALL(descend, descend(_, pud_test_node(6))).Times(0);
    const pud_witness_search_stack search =
        rebuilder.rebuild(root, {pud_test_node(2), pud_test_node(5), pud_test_node(6)});
    ASSERT_EQ(search.frames.size(), 2u);
    EXPECT_FALSE(search.frames.at(1).current.has_value());
    EXPECT_EQ(search.frames.at(1).next_sibling, 2u);
}

TEST_F(PudWitnessSearchRebuilderTest, AnEmptyChainRebuildsAnEmptyStack) {
    EXPECT_THAT(rebuilder.rebuild(root, {}).frames, IsEmpty());
}
