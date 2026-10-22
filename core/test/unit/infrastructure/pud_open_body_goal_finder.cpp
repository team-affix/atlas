#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_open_body_goal_finder.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {
struct MockGetBodyGoals {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

struct MockGetCallSite {
    MOCK_METHOD(pud_body_goal_id, get, (pud_node_id));
};

const expr* goal_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudOpenBodyGoalFinderTest : public ::testing::Test {
    NiceMock<MockGetBodyGoals> body_goals;
    NiceMock<MockGetCallSite> call_sites;
    pud_open_body_goal_finder<NiceMock<MockGetBodyGoals>, NiceMock<MockGetCallSite>> finder{body_goals, call_sites};
    std::unordered_map<pud_node_id, std::vector<const expr*>> goals_of;
    std::unordered_map<pud_node_id, pud_body_goal_id> call_site_of;

    void SetUp() override {
        ON_CALL(body_goals, get(_)).WillByDefault([&](pud_node_id node) -> const std::vector<const expr*>& {
            return goals_of.at(node);
        });
        ON_CALL(call_sites, get(_)).WillByDefault([&](pud_node_id node) { return call_site_of.at(node); });
    }
};

TEST_F(PudOpenBodyGoalFinderTest, RootPathHasTheRootsGoals) {
    goals_of[1] = {goal_at(0x10), goal_at(0x20)};
    EXPECT_THAT(finder.find_open_body_goals({1}), ElementsAre(goal_at(0x10), goal_at(0x20)));
}

TEST_F(PudOpenBodyGoalFinderTest, FactRootHasNoGoals) {
    goals_of[1] = {};
    EXPECT_THAT(finder.find_open_body_goals({1}), IsEmpty());
}

TEST_F(PudOpenBodyGoalFinderTest, ChildClosesItsParentsCallSiteAndAddsItsGoalsLast) {
    goals_of[1] = {goal_at(0x10), goal_at(0x20)};
    goals_of[2] = {goal_at(0x30)};
    call_site_of[1] = 0;
    EXPECT_THAT(finder.find_open_body_goals({1, 2}), ElementsAre(goal_at(0x20), goal_at(0x30)));
}

TEST_F(PudOpenBodyGoalFinderTest, BodyGoalIdsContinueAcrossThePath) {
    goals_of[1] = {goal_at(0x10), goal_at(0x20)};
    goals_of[2] = {goal_at(0x30), goal_at(0x40)};
    goals_of[3] = {};
    call_site_of[1] = 1;
    call_site_of[2] = 3;
    EXPECT_THAT(finder.find_open_body_goals({1, 2, 3}), ElementsAre(goal_at(0x10), goal_at(0x30)));
}

TEST_F(PudOpenBodyGoalFinderTest, ClosingEveryGoalLeavesNothingOpen) {
    goals_of[1] = {goal_at(0x10)};
    goals_of[2] = {};
    call_site_of[1] = 0;
    EXPECT_THAT(finder.find_open_body_goals({1, 2}), IsEmpty());
}

TEST_F(PudOpenBodyGoalFinderTest, LastNodesCallSiteIsNotClosed) {
    goals_of[1] = {goal_at(0x10)};
    EXPECT_CALL(call_sites, get(_)).Times(0);
    EXPECT_THAT(finder.find_open_body_goals({1}), ElementsAre(goal_at(0x10)));
}

}
