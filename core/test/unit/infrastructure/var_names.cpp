#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/var_names.hpp"

TEST(VarNamesTest, UnnamedIndexIsNotNamed) {
    var_names names;
    EXPECT_FALSE(names.is_named(0));
}

TEST(VarNamesTest, SetNameMakesIndexNamed) {
    var_names names;
    names.set_name(3, "X");
    EXPECT_TRUE(names.is_named(3));
    EXPECT_EQ(names.name(3), "X");
}

TEST(VarNamesTest, DifferentIndicesHaveIndependentNames) {
    var_names names;
    names.set_name(1, "a");
    names.set_name(2, "b");
    EXPECT_EQ(names.name(1), "a");
    EXPECT_EQ(names.name(2), "b");
    EXPECT_FALSE(names.is_named(0));
}
