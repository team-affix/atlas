#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_cache_trimming_step.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockUnfoldOne {
    MOCK_METHOD(pud_unfold_outcome, unfold_one, ());
};
struct MockTrimCache {
    MOCK_METHOD(void, trim, ());
};

}

TEST(PudCacheTrimmingStepTest, TheCacheIsTrimmedAfterTheStepAndTheStepsOutcomeReturned) {
    NiceMock<MockUnfoldOne> step;
    NiceMock<MockTrimCache> cache;
    pud_cache_trimming_step<NiceMock<MockUnfoldOne>, NiceMock<MockTrimCache>> trimming{step, cache};
    InSequence in_order;
    EXPECT_CALL(step, unfold_one()).WillOnce(Return(pud_unfold_outcome::off_focus));
    EXPECT_CALL(cache, trim());
    EXPECT_EQ(trimming.unfold_one(), pud_unfold_outcome::off_focus);
}
