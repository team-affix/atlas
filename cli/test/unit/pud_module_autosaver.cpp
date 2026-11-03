#include <cstddef>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_module_autosaver.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetUnfoldCount {
    MOCK_METHOD(size_t, unfold_count, (), (const));
};
struct MockSaveAllModules {
    MOCK_METHOD(void, save_all, (), (const));
};
struct MockHasPendingSolution {
    MOCK_METHOD(bool, has_pending_solution, (), (const));
};

using test_autosaver_t = pud_module_autosaver<
    NiceMock<MockGetUnfoldCount>, NiceMock<MockSaveAllModules>, NiceMock<MockHasPendingSolution>>;

struct PudModuleAutosaverTest : public ::testing::Test {
    NiceMock<MockGetUnfoldCount> runtime;
    NiceMock<MockSaveAllModules> modules;
    NiceMock<MockHasPendingSolution> pending;

    test_autosaver_t make_autosaver(size_t unfolds_per_save) {
        return test_autosaver_t{runtime, modules, pending, unfolds_per_save};
    }
};

}

TEST_F(PudModuleAutosaverTest, WithoutAnIntervalItNeverSaves) {
    test_autosaver_t autosaver = make_autosaver(0);
    ON_CALL(runtime, unfold_count()).WillByDefault(Return(1000));
    EXPECT_CALL(modules, save_all()).Times(0);
    autosaver.try_save();
    autosaver.try_save_final();
}

TEST_F(PudModuleAutosaverTest, ItSavesOnceEachIntervalOfUnfoldsHasPassed) {
    test_autosaver_t autosaver = make_autosaver(5);
    EXPECT_CALL(runtime, unfold_count())
        .WillOnce(Return(3))
        .WillOnce(Return(5))
        .WillOnce(Return(9))
        .WillOnce(Return(12));
    EXPECT_CALL(modules, save_all()).Times(2);
    autosaver.try_save();
    autosaver.try_save();
    autosaver.try_save();
    autosaver.try_save();
}

TEST_F(PudModuleAutosaverTest, TheIntervalCountsFromTheLastSaveEvenWhenATickCrossesSeveral) {
    test_autosaver_t autosaver = make_autosaver(5);
    EXPECT_CALL(runtime, unfold_count()).WillOnce(Return(17)).WillOnce(Return(21)).WillOnce(Return(22));
    EXPECT_CALL(modules, save_all()).Times(2);
    autosaver.try_save();
    autosaver.try_save();
    autosaver.try_save();
}

TEST_F(PudModuleAutosaverTest, TheFinalSaveAlwaysWritesWhenSavingIsOn) {
    test_autosaver_t autosaver = make_autosaver(5);
    ON_CALL(runtime, unfold_count()).WillByDefault(Return(0));
    EXPECT_CALL(modules, save_all()).Times(1);
    autosaver.try_save_final();
}

TEST_F(PudModuleAutosaverTest, ADueSaveWaitsWhileASolutionIsPendingAndHappensOnceItIsReported) {
    test_autosaver_t autosaver = make_autosaver(5);
    ON_CALL(runtime, unfold_count()).WillByDefault(Return(6));
    EXPECT_CALL(pending, has_pending_solution()).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(modules, save_all()).Times(1);
    autosaver.try_save();
    autosaver.try_save();
}
