#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_cache_trimmer.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;

namespace {

struct MockTrimCache {
    MOCK_METHOD(void, trim, ());
};

}

TEST(PudCacheTrimmerTest, BothCachesAreTrimmed) {
    NiceMock<MockTrimCache> first;
    NiceMock<MockTrimCache> second;
    pud_cache_trimmer<NiceMock<MockTrimCache>, NiceMock<MockTrimCache>> trimmer{first, second};
    InSequence in_order;
    EXPECT_CALL(first, trim());
    EXPECT_CALL(second, trim());
    trimmer.trim();
}
