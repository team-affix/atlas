#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_search_driver.hpp"

namespace {

coroutine<pud_search_step, void> unfold_then_solve() {
    co_yield pud_search_step::unfolded;
    co_yield pud_search_step::solved;
}

}

TEST(PudSearchDriverTest, EachNextAdvancesOneStepAndRemembersIt) {
    pud_search_driver driver{unfold_then_solve()};
    ASSERT_TRUE(driver.next());
    EXPECT_EQ(driver.last_step(), pud_search_step::unfolded);
    ASSERT_TRUE(driver.next());
    EXPECT_EQ(driver.last_step(), pud_search_step::solved);
}

TEST(PudSearchDriverTest, AFinishedSearchHasNoNextStep) {
    pud_search_driver driver{unfold_then_solve()};
    driver.next();
    driver.next();
    EXPECT_FALSE(driver.next());
    EXPECT_FALSE(driver.next());
}

TEST(PudSearchDriverTest, ThereIsNoLastStepBeforeTheFirst) {
    const pud_search_driver driver{unfold_then_solve()};
    EXPECT_THROW((void)driver.last_step(), std::logic_error);
}
