#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_call_sites.hpp"

struct PudCallSitesTest : public ::testing::Test {
    pud_call_sites call_sites;
    pud_node_id node = 1;
};

TEST_F(PudCallSitesTest, StoredZeroReadsBackZero) {
    call_sites.store(node, 0);
    EXPECT_EQ(call_sites.get(node), 0u);
}

TEST_F(PudCallSitesTest, SecondStoreOfSameNodeThrows) {
    call_sites.store(node, 1);
    EXPECT_THROW(call_sites.store(node, 2), std::logic_error);
}

TEST_F(PudCallSitesTest, StoreThenGet) {
    call_sites.store(node, 4);
    EXPECT_EQ(call_sites.get(node), 4u);
}
