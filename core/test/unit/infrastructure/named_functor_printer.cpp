#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "infrastructure/named_functor_printer.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockFunctorNames {
    MOCK_METHOD(bool, is_named, (uint32_t), (const));
    MOCK_METHOD(const std::string&, name, (uint32_t), (const));
};

struct NamedFunctorPrinterTest : public ::testing::Test {
    NiceMock<MockFunctorNames> fn;
    named_functor_printer<NiceMock<MockFunctorNames>> printer{fn};
    std::ostringstream os;
};

TEST_F(NamedFunctorPrinterTest, NamedIdPrintsStoredName) {
    const std::string foo = "foo";
    EXPECT_CALL(fn, is_named(2)).WillOnce(Return(true));
    EXPECT_CALL(fn, name(2)).WillOnce(ReturnRef(foo));
    printer.print(os, 2);
    EXPECT_EQ(os.str(), "foo");
}

TEST_F(NamedFunctorPrinterTest, UnnamedIdPrintsBangAndId) {
    EXPECT_CALL(fn, is_named(2)).WillOnce(Return(false));
    printer.print(os, 2);
    EXPECT_EQ(os.str(), "!2");
}

TEST_F(NamedFunctorPrinterTest, NamedFZeroDoesNotCaptureUnnamedOne) {
    const std::string f_zero = "f0";
    EXPECT_CALL(fn, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(fn, name(0)).WillOnce(ReturnRef(f_zero));
    printer.print(os, 0);
    EXPECT_EQ(os.str(), "f0");

    os.str("");
    os.clear();
    EXPECT_CALL(fn, is_named(1)).WillOnce(Return(false));
    printer.print(os, 1);
    EXPECT_EQ(os.str(), "!1");
}
