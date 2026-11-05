#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_chunked_register_applier.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_frontier_change.hpp"

using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {

struct MockApplyRegister {
    MOCK_METHOD(void, apply_register, (const pud_frontier_change&));
};
struct MockTrimCaches {
    MOCK_METHOD(void, trim, ());
};

struct PudChunkedRegisterApplierTest : public ::testing::Test {
    NiceMock<MockApplyRegister> frontier;
    NiceMock<MockTrimCaches> caches;
    pud_chunked_register_applier<NiceMock<MockApplyRegister>, NiceMock<MockTrimCaches>> applier{frontier, caches, 2};
};

}

TEST_F(PudChunkedRegisterApplierTest, OffspringAreAppliedInChunksWithTheCachesTrimmedAfterEach) {
    InSequence in_order;
    const pud_frontier_change first{.offspring = {pud_test_node(1), pud_test_node(2)}, .refuted = {pud_test_node(9)}};
    EXPECT_CALL(frontier, apply_register(first));
    EXPECT_CALL(caches, trim());
    EXPECT_CALL(frontier, apply_register(pud_frontier_change{.offspring = {pud_test_node(3)}, .refuted = {}}));
    EXPECT_CALL(caches, trim());
    applier.apply_register(pud_frontier_change{
        .offspring = {pud_test_node(1), pud_test_node(2), pud_test_node(3)}, .refuted = {pud_test_node(9)}});
}

TEST_F(PudChunkedRegisterApplierTest, AChangeWithoutOffspringIsStillApplied) {
    EXPECT_CALL(frontier, apply_register(pud_frontier_change{.offspring = {}, .refuted = {pud_test_node(9)}}));
    applier.apply_register(pud_frontier_change{.offspring = {}, .refuted = {pud_test_node(9)}});
}
