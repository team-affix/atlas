#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_module_registrar.hpp"

using ::testing::InSequence;
using ::testing::Return;

namespace {

struct registered_module {
    pud_module_id get_module_id() const { return 9; }
};

struct MockLinkFunctorNames {
    MOCK_METHOD(void, link, (registered_module&));
};

struct MockRegisterModule {
    MOCK_METHOD(void, register_module, (registered_module&));
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

struct PudLodestarModuleRegistrarTest : public ::testing::Test {
    MockLinkFunctorNames linker;
    MockRegisterModule modules;
    MockSeedFocus seeder;
    MockActivateModule activator;
    MockApplyRegister frontier;
    MockAddPendingConflicts pending;
    pud_lodestar_module_registrar<
        registered_module, MockLinkFunctorNames, MockRegisterModule, MockSeedFocus, MockActivateModule,
        MockApplyRegister, MockAddPendingConflicts>
        registrar{linker, modules, seeder, activator, frontier, pending};
    registered_module module;
};

}

TEST_F(PudLodestarModuleRegistrarTest, RegistrationLinksSeedsActivatesAndCountsRefutations) {
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
    registrar.register_module(module);
}

TEST_F(PudLodestarModuleRegistrarTest, ARegistrationWithNoRefutationsAddsNoConflicts) {
    const pud_frontier_change change{};
    EXPECT_CALL(linker, link);
    EXPECT_CALL(modules, register_module);
    EXPECT_CALL(seeder, seed(9u));
    EXPECT_CALL(activator, activate_module(9u)).WillOnce(Return(change));
    EXPECT_CALL(frontier, apply_register(change));
    EXPECT_CALL(pending, add(0u));
    registrar.register_module(module);
}
