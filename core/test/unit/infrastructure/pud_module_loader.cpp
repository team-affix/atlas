#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_module_loader.hpp"

using ::testing::InSequence;
using ::testing::Return;

namespace {

struct MockModule {
    MOCK_METHOD(pud_module_id, get_module_id, (), (const));
};

using module_t = ::testing::NiceMock<MockModule>;

struct MockLinkFunctorNames {
    MOCK_METHOD(void, link, (module_t&));
};

struct MockRegisterModule {
    MOCK_METHOD(void, register_module, (module_t&));
};

struct MockSeedFocus {
    MOCK_METHOD(void, seed, (pud_module_id));
};

struct MockActivateModule {
    MOCK_METHOD(pud_frontier_change, activate_module, (pud_module_id));
};

struct MockApplyRegister {
    MOCK_METHOD(void, apply_register, (const pud_frontier_change&));
};

struct MockAddPendingConflicts {
    MOCK_METHOD(void, add, (size_t));
};

struct PudModuleLoaderTest : public ::testing::Test {
    MockLinkFunctorNames linker;
    MockRegisterModule modules;
    MockSeedFocus seeder;
    MockActivateModule activator;
    MockApplyRegister frontier;
    MockAddPendingConflicts pending;
    pud_module_loader<
        module_t, MockLinkFunctorNames, MockRegisterModule, MockSeedFocus, MockActivateModule,
        MockApplyRegister, MockAddPendingConflicts>
        loader{linker, modules, seeder, activator, frontier, pending};
    module_t module;

    void SetUp() override {
        ON_CALL(module, get_module_id()).WillByDefault(Return(9));
    }
};

}

TEST_F(PudModuleLoaderTest, RegistrationLinksSeedsActivatesAndCountsRefutations) {
    const pud_frontier_change change{.offspring = {pud_global_node_id{.module = 9, .node = 0}}, .refuted = {
        pud_global_node_id{.module = 9, .node = 1},
        pud_global_node_id{.module = 9, .node = 2},
    }};
    InSequence order;
    EXPECT_CALL(linker, link);
    EXPECT_CALL(modules, register_module);
    EXPECT_CALL(seeder, seed(9u));
    EXPECT_CALL(activator, activate_module(9u)).WillOnce(Return(change));
    EXPECT_CALL(frontier, apply_register(change));
    EXPECT_CALL(pending, add(2u));
    loader.load(module);
}

TEST_F(PudModuleLoaderTest, ARegistrationWithNoRefutationsAddsNoConflicts) {
    const pud_frontier_change change{};
    EXPECT_CALL(linker, link);
    EXPECT_CALL(modules, register_module);
    EXPECT_CALL(seeder, seed(9u));
    EXPECT_CALL(activator, activate_module(9u)).WillOnce(Return(change));
    EXPECT_CALL(frontier, apply_register(change));
    EXPECT_CALL(pending, add(0u));
    loader.load(module);
}
