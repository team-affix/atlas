#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_stored_solution_dropper.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockListSolvedLeaves {
    MOCK_METHOD(std::vector<pud_global_node_id>, list, (), (const));
};
struct MockGetNodeParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id), (const));
};
struct MockEraseFrontierNode {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};

using test_dropper_t = pud_stored_solution_dropper<
    NiceMock<MockListSolvedLeaves>, NiceMock<MockGetNodeParent>, NiceMock<MockEraseFrontierNode>>;

// Solved focus leaves 0 (the focus axiom, no parent), 4 and 5 (inferences) are on the frontier.
struct PudStoredSolutionDropperTest : public ::testing::Test {
    NiceMock<MockListSolvedLeaves> finder;
    NiceMock<MockGetNodeParent> modules;
    NiceMock<MockEraseFrontierNode> frontier;

    void SetUp() override {
        ON_CALL(finder, list())
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(0), pud_test_node(4), pud_test_node(5)}));
        ON_CALL(modules, get_parent(pud_test_node(0))).WillByDefault(Return(std::nullopt));
        ON_CALL(modules, get_parent(pud_test_node(4))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(modules, get_parent(pud_test_node(5))).WillByDefault(Return(pud_test_node(4)));
    }
};

}

TEST_F(PudStoredSolutionDropperTest, EveryDerivedSolvedLeafLeavesTheFrontierUnreported) {
    test_dropper_t dropper{finder, modules, frontier, false};
    EXPECT_CALL(frontier, erase(pud_test_node(4)));
    EXPECT_CALL(frontier, erase(pud_test_node(5)));
    dropper.drop();
}

TEST_F(PudStoredSolutionDropperTest, AFocusAxiomThatIsAFactStaysToBeReported) {
    test_dropper_t dropper{finder, modules, frontier, false};
    EXPECT_CALL(frontier, erase(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(frontier, erase(pud_test_node(0))).Times(0);
    dropper.drop();
}

TEST_F(PudStoredSolutionDropperTest, WhenStoredSolutionsAreReportedItDropsNothing) {
    test_dropper_t dropper{finder, modules, frontier, true};
    EXPECT_CALL(finder, list()).Times(0);
    EXPECT_CALL(frontier, erase(_)).Times(0);
    dropper.drop();
}
