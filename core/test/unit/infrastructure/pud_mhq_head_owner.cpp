#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/box.hpp>
#include <immer/map.hpp>
#include "infrastructure/pud_mhq_head_owner.hpp"

using ::testing::StrictMock;

namespace {

struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};

using test_owner_t = pud_mhq_head_owner<StrictMock<MockRemoveQuery>>;
using test_owner_box_t = immer::box<test_owner_t>;
using test_map_t = immer::map<int, test_owner_box_t>;

struct PudMhqHeadOwnerTest : public ::testing::Test {
    StrictMock<MockRemoveQuery> remover;
};

}

TEST_F(PudMhqHeadOwnerTest, AnOwnerKnowsItsHead) {
    EXPECT_CALL(remover, remove(5u));
    const test_owner_t owner{remover, 5};
    EXPECT_EQ(owner.head_id(), 5u);
}

TEST_F(PudMhqHeadOwnerTest, TheHeadIsRemovedOnlyWhenTheLastBoxSharingTheOwnerGoes) {
    std::optional<test_owner_box_t> first{test_owner_box_t{remover, 5}};
    std::optional<test_owner_box_t> second = first;
    first.reset();
    ::testing::Mock::VerifyAndClearExpectations(&remover);
    EXPECT_CALL(remover, remove(5u));
    second.reset();
}

TEST_F(PudMhqHeadOwnerTest, AChildMapKeepsTheHeadsItSharesWithItsParentAfterTheParentGoes) {
    std::optional<test_map_t> parent{
        test_map_t{}.set(0, test_owner_box_t{remover, 9}).set(1, test_owner_box_t{remover, 10})};
    std::optional<test_map_t> child{parent->erase(1).set(2, test_owner_box_t{remover, 11})};
    EXPECT_CALL(remover, remove(10u));
    parent.reset();
    ::testing::Mock::VerifyAndClearExpectations(&remover);
    EXPECT_CALL(remover, remove(9u));
    EXPECT_CALL(remover, remove(11u));
    child.reset();
}
