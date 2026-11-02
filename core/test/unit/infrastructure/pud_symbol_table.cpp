#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_symbol_table.hpp"

TEST(PudSymbolTableTest, NilAndConsAreTheFirstGlobalIds) {
    pud_symbol_table symbols;
    EXPECT_EQ(symbols.intern_functor_name("[]"), 0u);
    EXPECT_EQ(symbols.intern_functor_name("cons"), 1u);
    EXPECT_EQ(symbols.get_name_by_global_functor_id(0), "[]");
    EXPECT_EQ(symbols.get_name_by_global_functor_id(1), "cons");
}

TEST(PudSymbolTableTest, ANewNameGetsTheNextGlobalIdAndCanBeReadBothWays) {
    pud_symbol_table symbols;
    EXPECT_EQ(symbols.intern_functor_name("nat"), 2u);
    EXPECT_EQ(symbols.intern_functor_name("nat"), 2u);
    EXPECT_EQ(symbols.get_name_by_global_functor_id(2), "nat");
}

TEST(PudSymbolTableTest, AMissingGlobalIdThrows) {
    const pud_symbol_table symbols;
    EXPECT_THROW((void)symbols.get_name_by_global_functor_id(2), std::out_of_range);
}
