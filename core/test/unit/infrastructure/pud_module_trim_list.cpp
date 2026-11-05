#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_module_trim_list.hpp"

using ::testing::NiceMock;

namespace {

struct MockTrimModule {
    MOCK_METHOD(void, trim, ());
};

struct PudModuleTrimListTest : public ::testing::Test {
    NiceMock<MockTrimModule> first;
    NiceMock<MockTrimModule> second;
    pud_module_trim_list<NiceMock<MockTrimModule>> list;
};

}

TEST_F(PudModuleTrimListTest, EveryAddedModuleIsTrimmed) {
    list.add(first);
    list.add(second);
    EXPECT_CALL(first, trim());
    EXPECT_CALL(second, trim());
    list.trim();
}

TEST_F(PudModuleTrimListTest, AnEmptyListTrimsNothing) {
    list.trim();
}
