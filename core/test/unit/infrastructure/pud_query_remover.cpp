#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_remover.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;

namespace {

struct MockWithdrawIntroduction {
    MOCK_METHOD(void, withdraw, (pud_mhq_head_id));
};
struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};

struct PudQueryRemoverTest : public ::testing::Test {
    NiceMock<MockWithdrawIntroduction> introducers;
    NiceMock<MockRemoveQuery> mhq;
    pud_query_remover<NiceMock<MockWithdrawIntroduction>, NiceMock<MockRemoveQuery>> remover{introducers, mhq};
};

}

TEST_F(PudQueryRemoverTest, ARemovedQueryIsWithdrawnThenItsHeadIsRemoved) {
    InSequence order;
    EXPECT_CALL(introducers, withdraw(40u));
    EXPECT_CALL(mhq, remove(40u));
    remover.remove(40);
}
