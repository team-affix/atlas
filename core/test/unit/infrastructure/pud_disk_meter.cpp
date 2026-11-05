#include <cstddef>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_disk_meter.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMeasureBytes {
    MOCK_METHOD(size_t, bytes, (), (const));
};

struct PudDiskMeterTest : public ::testing::Test {
    NiceMock<MockMeasureBytes> first;
    NiceMock<MockMeasureBytes> second;
    pud_disk_meter<NiceMock<MockMeasureBytes>> meter;
};

}

TEST_F(PudDiskMeterTest, TheRegisteredModulesFilesAreAddedUp) {
    ON_CALL(first, bytes()).WillByDefault(Return(300u));
    ON_CALL(second, bytes()).WillByDefault(Return(45u));
    meter.add(first);
    meter.add(second);
    EXPECT_EQ(meter.bytes(), 345u);
}

TEST_F(PudDiskMeterTest, NoModulesTakeNoBytes) {
    EXPECT_EQ(meter.bytes(), 0u);
}
