#include <gtest/gtest.h>
#include <sstream>
#include "infrastructure/id_functor_printer.hpp"

struct IdFunctorPrinterTest : public ::testing::Test {
    id_functor_printer printer;
    std::ostringstream os;
};

TEST_F(IdFunctorPrinterTest, PrintsFThenId) {
    printer.print(os, 0);
    EXPECT_EQ(os.str(), "f0");
}
