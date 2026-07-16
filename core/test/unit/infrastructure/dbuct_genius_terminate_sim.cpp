// dbuct_genius_terminate_sim: delegates only; value delta stays node-aware.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/dbuct_genius_terminate_sim.hpp"
#include "value_objects/lineage.hpp"

using ::testing::Return;

struct MockTerminateDbuct {
    MOCK_METHOD((std::vector<const resolution_lineage*>), terminate, ());
};

using test_dbuct_genius_terminate_sim_t = dbuct_genius_terminate_sim<MockTerminateDbuct>;

struct DbuctGeniusTerminateSimTest : public ::testing::Test {
    MockTerminateDbuct terminate_dbuct;
    test_dbuct_genius_terminate_sim_t terminate{terminate_dbuct};
};

TEST_F(DbuctGeniusTerminateSimTest, DelegatesAndForwardsEliminations) {
    resolution_lineage rl{nullptr, 0};
    std::vector<const resolution_lineage*> elims{&rl};

    EXPECT_CALL(terminate_dbuct, terminate()).WillOnce(Return(elims));
    EXPECT_EQ(terminate.terminate(), elims);
}
