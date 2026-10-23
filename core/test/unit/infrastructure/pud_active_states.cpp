#include <gtest/gtest.h>
#include "infrastructure/pud_active_states.hpp"

namespace {

struct PudActiveStatesTest : public ::testing::Test {
    pud_active_states states;

    pud_active_state with_query(pud_body_goal_id goal, pud_mhq_head_id head) {
        return pud_active_state{.bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(goal, head)};
    }
};

}

TEST_F(PudActiveStatesTest, AStoredStateReadsBack) {
    states.store(2, with_query(0, 7));
    EXPECT_EQ(states.get(2), with_query(0, 7));
}

TEST_F(PudActiveStatesTest, AnErasedNodeCanNoLongerBeRead) {
    states.store(0, with_query(0, 1));
    states.store(1, with_query(0, 2));
    states.erase(0);
    EXPECT_THROW(states.get(0), std::bad_optional_access);
    EXPECT_EQ(states.get(1), with_query(0, 2));
}
