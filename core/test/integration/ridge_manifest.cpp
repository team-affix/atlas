// Integration: ridge_manifest — locator wiring smoke test.

#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_manifest.hpp"

namespace {

class RidgeManifestIntegrationTest : public ::testing::Test {
protected:
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;
};

TEST_F(RidgeManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    /*
     * Intent: ridge_manifest constructs without throwing on an empty problem.
     * initial goals: (none)
     * rules: (none)
     */
    EXPECT_NO_THROW((ridge_manifest{
        database,
        initial_goals,
        kInitialVarCount,
        kMaxResolutions,
        kSeed,
        kExplorationConstant}));
}

} // namespace
