#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_frontier.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 4, .node = id};
}

struct PudLodestarFrontierTest : public ::testing::Test {
    pud_lodestar_frontier frontier;
};

}

TEST_F(PudLodestarFrontierTest, ANewFrontierIsEmpty) {
    EXPECT_THAT(frontier.live(), IsEmpty());
}

TEST_F(PudLodestarFrontierTest, RegisterAddsOffspringAndDropsRefuted) {
    frontier.apply_register(pud_frontier_change{.offspring = {node(0), node(1), node(2)}, .refuted = {node(1)}});
    EXPECT_THAT(frontier.live(), ElementsAre(node(0), node(2)));
}

TEST_F(PudLodestarFrontierTest, UnfoldReplacesTheParentWithItsOffspring) {
    frontier.apply_register(pud_frontier_change{.offspring = {node(0)}, .refuted = {}});
    frontier.apply_unfold(node(0), pud_frontier_change{.offspring = {node(3), node(1)}, .refuted = {node(3)}});
    EXPECT_THAT(frontier.live(), ElementsAre(node(1)));
}

TEST_F(PudLodestarFrontierTest, EraseDropsOneLiveNodeAndIgnoresAMissingOne) {
    frontier.apply_register(pud_frontier_change{.offspring = {node(0), node(2)}, .refuted = {}});
    frontier.erase(node(0));
    frontier.erase(node(9));
    EXPECT_THAT(frontier.live(), ElementsAre(node(2)));
}
