#include <cstddef>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_disk_budget_step.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockUnfoldOne {
    MOCK_METHOD(pud_unfold_outcome, unfold_one, ());
};
struct MockMeasureDiskBytes {
    MOCK_METHOD(size_t, bytes, (), (const));
};

struct PudDiskBudgetStepTest : public ::testing::Test {
    NiceMock<MockUnfoldOne> step;
    NiceMock<MockMeasureDiskBytes> disk;
    pud_disk_budget_step<NiceMock<MockUnfoldOne>, NiceMock<MockMeasureDiskBytes>> budget{step, disk, 1000};

    void SetUp() override { ON_CALL(step, unfold_one()).WillByDefault(Return(pud_unfold_outcome::on_focus)); }
};

}

TEST_F(PudDiskBudgetStepTest, UnderBudgetTheStepIsMade) {
    ON_CALL(disk, bytes()).WillByDefault(Return(999u));
    EXPECT_CALL(step, unfold_one()).WillOnce(Return(pud_unfold_outcome::off_focus));
    EXPECT_EQ(budget.unfold_one(), pud_unfold_outcome::off_focus);
    EXPECT_FALSE(budget.exceeded());
}

TEST_F(PudDiskBudgetStepTest, AtBudgetNoStepIsMadeAndTheBudgetIsExceeded) {
    ON_CALL(disk, bytes()).WillByDefault(Return(1000u));
    EXPECT_CALL(step, unfold_one()).Times(0);
    EXPECT_EQ(budget.unfold_one(), pud_unfold_outcome::none);
    EXPECT_TRUE(budget.exceeded());
}
