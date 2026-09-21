// pud_node_touched_reps: store overwrites; get throws on miss.

#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "infrastructure/pud_node_touched_reps.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeTouchedRepsTest : public ::testing::Test {
    PudNodeTouchedRepsTest()
        : axiom_{pud_rule_id::axiom{0}}
        , reps_() {}

    pud_rule_id axiom_;
    pud_node_touched_reps reps_;
};

TEST_F(PudNodeTouchedRepsTest, StoreThenGet) {
    reps_.store(&axiom_, {3, 5});
    EXPECT_EQ(reps_.get(&axiom_), (std::vector<uint32_t>{3, 5}));
}

TEST_F(PudNodeTouchedRepsTest, GetUnknownThrows) {
    EXPECT_THROW(reps_.get(&axiom_), std::out_of_range);
}

TEST_F(PudNodeTouchedRepsTest, StoreOverwrites) {
    reps_.store(&axiom_, {1});
    reps_.store(&axiom_, {9, 8});
    EXPECT_EQ(reps_.get(&axiom_), (std::vector<uint32_t>{9, 8}));
}
