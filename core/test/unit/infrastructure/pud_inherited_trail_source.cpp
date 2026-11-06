#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_inherited_trail_source.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_node_trails.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id), (const));
};
struct MockFindTrails {
    MOCK_METHOD(const pud_node_trails*, find, (pud_global_node_id));
};

pud_goal_trails trails_on(pud_node_id candidate, bool forked) {
    const pud_candidate_trail trail{.candidate = pud_test_node(candidate), .witness_leaves = {pud_test_node(candidate)}};
    return pud_goal_trails{.candidates = {trail}, .forked = forked};
}

// The path is 0 -> 1 -> 2 -> 3; node 0 holds trails for goals 4 and 5, one of them already forked.
struct PudInheritedTrailSourceTest : public ::testing::Test {
    NiceMock<MockGetParent> parents;
    NiceMock<MockFindTrails> trails;
    pud_inherited_trail_source<NiceMock<MockGetParent>, NiceMock<MockFindTrails>> source{parents, trails};
    pud_node_trails root_trails = pud_node_trails{}.set(4, trails_on(7, false)).set(5, trails_on(8, true));
    pud_node_trails own_trails = pud_node_trails{}.set(6, trails_on(9, false));

    void SetUp() override {
        ON_CALL(parents, get_parent(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(parents, get_parent(pud_test_node(1))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(parents, get_parent(pud_test_node(2))).WillByDefault(Return(pud_test_node(1)));
        ON_CALL(parents, get_parent(pud_test_node(3))).WillByDefault(Return(pud_test_node(2)));
        ON_CALL(trails, find(_)).WillByDefault(Return(nullptr));
        ON_CALL(trails, find(pud_test_node(0))).WillByDefault(Return(&root_trails));
    }
};

}

TEST_F(PudInheritedTrailSourceTest, ALeafsOwnTrailsAreUsedAsTheyAre) {
    ON_CALL(trails, find(pud_test_node(3))).WillByDefault(Return(&own_trails));
    EXPECT_EQ(source.get(pud_test_node(3)), own_trails);
}

TEST_F(PudInheritedTrailSourceTest, ALeafWithoutTrailsTakesItsNearestAncestorsMarkedForked) {
    const pud_node_trails inherited = source.get(pud_test_node(3));
    ASSERT_EQ(inherited.size(), 2u);
    EXPECT_EQ(inherited.find(4)->get(), trails_on(7, true));
    EXPECT_EQ(inherited.find(5)->get(), trails_on(8, true));
}

TEST_F(PudInheritedTrailSourceTest, ANearerAncestorsTrailsAreTakenOverAFartherOnes) {
    ON_CALL(trails, find(pud_test_node(2))).WillByDefault(Return(&own_trails));
    const pud_node_trails inherited = source.get(pud_test_node(3));
    ASSERT_EQ(inherited.size(), 1u);
    EXPECT_EQ(inherited.find(6)->get(), trails_on(9, true));
}

TEST_F(PudInheritedTrailSourceTest, ALeafWithNoTrailsOnItsPathHasNone) {
    ON_CALL(trails, find(pud_test_node(0))).WillByDefault(Return(nullptr));
    EXPECT_EQ(source.get(pud_test_node(3)).size(), 0u);
}
