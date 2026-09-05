#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "infrastructure/named_var_printer.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockVarNames {
    MOCK_METHOD(bool, is_named, (uint32_t), (const));
    MOCK_METHOD(const std::string&, name, (uint32_t), (const));
};

struct NamedVarPrinterTest : public ::testing::Test {
    NiceMock<MockVarNames> vn;
    named_var_printer<NiceMock<MockVarNames>> printer{vn};
    std::ostringstream os;
};

TEST_F(NamedVarPrinterTest, NamedIndexPrintsStoredName) {
    const std::string x = "X";
    EXPECT_CALL(vn, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(vn, name(0)).WillOnce(ReturnRef(x));
    printer.print(os, 0);
    EXPECT_EQ(os.str(), "X");
}

TEST_F(NamedVarPrinterTest, UnnamedIndexPrintsQuestionMarkAndIndex) {
    EXPECT_CALL(vn, is_named(0)).WillOnce(Return(false));
    printer.print(os, 0);
    EXPECT_EQ(os.str(), "?0");
}

TEST_F(NamedVarPrinterTest, NamedUnderscoreZeroDoesNotCaptureUnnamedOne) {
    const std::string underscore_zero = "_0";
    EXPECT_CALL(vn, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(vn, name(0)).WillOnce(ReturnRef(underscore_zero));
    printer.print(os, 0);
    EXPECT_EQ(os.str(), "_0");

    os.str("");
    os.clear();
    EXPECT_CALL(vn, is_named(1)).WillOnce(Return(false));
    printer.print(os, 1);
    EXPECT_EQ(os.str(), "?1");
}
