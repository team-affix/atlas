#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lazy_activity_mover.hpp"
#include "pud_test_node.hpp"

using ::testing::IsEmpty;

namespace {



}

TEST(PudLazyActivityMoverTest, NoChildArrivesRefuted) {
    pud_lazy_activity_mover mover;
    EXPECT_THAT(mover.move(pud_test_node(1), {pud_test_node(2), pud_test_node(3)}), IsEmpty());
}
