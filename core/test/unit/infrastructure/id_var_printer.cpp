#include <gtest/gtest.h>
#include <sstream>
#include "infrastructure/id_var_printer.hpp"

struct IdVarPrinterTest : public ::testing::Test {
    id_var_printer printer;
    std::ostringstream os;
};

TEST_F(IdVarPrinterTest, PrintsUnderscoreThenIndex) {
    printer.print(os, 0);
    EXPECT_EQ(os.str(), "_0");
}
