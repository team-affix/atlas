#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_ancestor_path_finder.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id), (const));
};

// The stored tree is 0 -> 2 -> 5 -> 9.
struct PudAncestorPathFinderTest : public ::testing::Test {
    NiceMock<MockGetParent> parents;
    pud_ancestor_path_finder<NiceMock<MockGetParent>> finder{parents};

    void SetUp() override {
        ON_CALL(parents, get_parent(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(parents, get_parent(pud_test_node(2))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(parents, get_parent(pud_test_node(5))).WillByDefault(Return(pud_test_node(2)));
        ON_CALL(parents, get_parent(pud_test_node(9))).WillByDefault(Return(pud_test_node(5)));
    }
};

}

TEST_F(PudAncestorPathFinderTest, ThePathFromTheRootRunsDownToTheNode) {
    EXPECT_THAT(
        finder.find_path_from_root(pud_test_node(9)),
        ElementsAre(pud_test_node(0), pud_test_node(2), pud_test_node(5), pud_test_node(9)));
}

TEST_F(PudAncestorPathFinderTest, ARootsPathIsItself) {
    EXPECT_THAT(finder.find_path_from_root(pud_test_node(0)), ElementsAre(pud_test_node(0)));
}

TEST_F(PudAncestorPathFinderTest, TheChainBelowAnAncestorLeavesTheAncestorOut) {
    EXPECT_THAT(
        finder.find_path_below(pud_test_node(2), pud_test_node(9)), ElementsAre(pud_test_node(5), pud_test_node(9)));
}

TEST_F(PudAncestorPathFinderTest, TheChainBelowANodeItselfIsEmpty) {
    EXPECT_THAT(finder.find_path_below(pud_test_node(5), pud_test_node(5)), IsEmpty());
}
