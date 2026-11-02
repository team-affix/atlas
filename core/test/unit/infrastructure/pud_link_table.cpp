#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_link_table.hpp"

TEST(PudLinkTableTest, TheFirstLinkCreatesTheModuleRow) {
    pud_link_table links;
    EXPECT_EQ(links.find_local_functor_id(4, 7), std::nullopt);
    links.insert_link(4, 2, 7);
    EXPECT_EQ(links.get_global_functor_id_from_local_functor_id(4, 2), 7u);
    EXPECT_EQ(links.find_local_functor_id(4, 7), 2u);
}

TEST(PudLinkTableTest, TwoModulesCanBindTheSameLocalIdToDifferentGlobals) {
    pud_link_table links;
    links.insert_link(1, 2, 5);
    links.insert_link(2, 2, 9);
    EXPECT_EQ(links.get_global_functor_id_from_local_functor_id(1, 2), 5u);
    EXPECT_EQ(links.get_global_functor_id_from_local_functor_id(2, 2), 9u);
}

TEST(PudLinkTableTest, AMissingModuleOrSlotThrowsOrIsNotFound) {
    pud_link_table links;
    links.insert_link(1, 2, 5);
    EXPECT_THROW((void)links.get_global_functor_id_from_local_functor_id(9, 2), std::out_of_range);
    EXPECT_THROW((void)links.get_global_functor_id_from_local_functor_id(1, 3), std::out_of_range);
    EXPECT_EQ(links.find_local_functor_id(1, 8), std::nullopt);
}
