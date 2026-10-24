#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_active_states.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudActiveStatesTest : public ::testing::Test {
    pud_active_states states;
};

}

TEST_F(PudActiveStatesTest, AStoredStateReadsBack) {
    states.store(pud_test_node(2), pud_active_state{
        .lvc = 0, .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 7)});
    EXPECT_EQ(states.get(pud_test_node(2)), (pud_active_state{
        .lvc = 0, .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 7)}));
}

TEST_F(PudActiveStatesTest, AnErasedNodeCanNoLongerBeRead) {
    states.store(pud_test_node(0), pud_active_state{
        .lvc = 0, .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 1)});
    states.store(pud_test_node(1), pud_active_state{
        .lvc = 0, .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 2)});
    states.erase(pud_test_node(0));
    EXPECT_THROW(states.get(pud_test_node(0)), std::out_of_range);
    EXPECT_EQ(states.get(pud_test_node(1)), (pud_active_state{
        .lvc = 0, .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 2)}));
}
