#include <gtest/gtest.h>
#include "infrastructure/pud_leaf_states.hpp"

namespace {

struct PudLeafStatesTest : public ::testing::Test {
    pud_leaf_states states;

    pud_leaf_state with_query(pud_body_goal_id goal, pud_mhq_head_id head) {
        return pud_leaf_state{.bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(goal, head)};
    }
};

}

TEST_F(PudLeafStatesTest, AStoredStateReadsBack) {
    states.store(2, with_query(0, 7));
    EXPECT_EQ(states.get(2), with_query(0, 7));
}

TEST_F(PudLeafStatesTest, AnErasedLeafCanNoLongerBeRead) {
    states.store(0, with_query(0, 1));
    states.store(1, with_query(0, 2));
    states.erase(0);
    EXPECT_THROW(states.get(0), std::bad_optional_access);
    EXPECT_EQ(states.get(1), with_query(0, 2));
}
