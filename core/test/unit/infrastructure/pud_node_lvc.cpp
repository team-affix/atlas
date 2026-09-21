// pud_node_lvc: store and get by id.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_node_lvc.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeLvcTest : public ::testing::Test {
    PudNodeLvcTest()
        : axiom_{pud_rule_id::axiom{0}}
        , lvc_() {}

    pud_rule_id axiom_;
    pud_node_lvc lvc_;
};

TEST_F(PudNodeLvcTest, StoreThenGet) {
    lvc_.store(&axiom_, 3);
    EXPECT_EQ(lvc_.get(&axiom_), 3u);
}

TEST_F(PudNodeLvcTest, GetUnknownThrows) {
    EXPECT_THROW(lvc_.get(&axiom_), std::out_of_range);
}
