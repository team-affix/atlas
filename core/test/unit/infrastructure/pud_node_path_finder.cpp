#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_path_finder.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;

namespace {

struct MockCheckRoot {
    MOCK_METHOD(bool, check_root, (pud_node_id));
};

struct MockGetParent {
    MOCK_METHOD(pud_node_id, get, (pud_node_id));
};

struct PudNodePathFinderTest : public ::testing::Test {
    NiceMock<MockCheckRoot> roots;
    NiceMock<MockGetParent> parents;
    pud_node_path_finder<NiceMock<MockCheckRoot>, NiceMock<MockGetParent>> finder{roots, parents};
    std::unordered_set<pud_node_id> root_nodes{1};
    std::unordered_map<pud_node_id, pud_node_id> parent_of{{2, 1}, {3, 2}, {4, 3}, {5, 1}};

    void SetUp() override {
        ON_CALL(roots, check_root(_)).WillByDefault([&](pud_node_id node) { return root_nodes.contains(node); });
        ON_CALL(parents, get(_)).WillByDefault([&](pud_node_id node) { return parent_of.at(node); });
    }
};

TEST_F(PudNodePathFinderTest, RootPathIsJustTheRoot) {
    EXPECT_THAT(finder.find_path(1), ElementsAre(1u));
}

TEST_F(PudNodePathFinderTest, PathRunsRootFirst) {
    EXPECT_THAT(finder.find_path(4), ElementsAre(1u, 2u, 3u, 4u));
}

TEST_F(PudNodePathFinderTest, ChildOfTheRootHasATwoNodePath) {
    EXPECT_THAT(finder.find_path(5), ElementsAre(1u, 5u));
}

TEST_F(PudNodePathFinderTest, RootsParentIsNeverLookedUp) {
    EXPECT_CALL(parents, get(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(parents, get(1u)).Times(0);
    finder.find_path(4);
}

TEST_F(PudNodePathFinderTest, LongChainIsWalkedToTheRoot) {
    parent_of.clear();
    for (pud_node_id node = 2; node <= 300; ++node)
        parent_of[node] = node - 1;
    const std::vector<pud_node_id> path = finder.find_path(300);
    ASSERT_EQ(path.size(), 300u);
    for (pud_node_id index = 0; index < path.size(); ++index)
        EXPECT_EQ(path.at(index), index + 1);
}

}  // namespace
