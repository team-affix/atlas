#include <gtest/gtest.h>
#include "infrastructure/var_names.hpp"

struct VarNamesTest : public ::testing::Test {
protected:
    var_names names;
};

TEST_F(VarNamesTest, UnnamedIndexIsNotNamed) {
    EXPECT_FALSE(names.is_named(0));
}

TEST_F(VarNamesTest, SetNameMakesIndexNamed) {
    names.set_name(3, "X");
    EXPECT_TRUE(names.is_named(3));
    EXPECT_EQ(names.name(3), "X");
}

TEST_F(VarNamesTest, DifferentIndicesHaveIndependentNames) {
    names.set_name(1, "a");
    names.set_name(2, "b");
    EXPECT_EQ(names.name(1), "a");
    EXPECT_EQ(names.name(2), "b");
    EXPECT_FALSE(names.is_named(0));
}

TEST_F(VarNamesTest, NameOnUnnamedIndexThrows) {
    EXPECT_THROW(names.name(0), std::out_of_range);
}

TEST_F(VarNamesTest, SetNameDoesNotOverwriteExisting) {
    names.set_name(1, "first");
    names.set_name(1, "second");
    EXPECT_EQ(names.name(1), "first");
}
