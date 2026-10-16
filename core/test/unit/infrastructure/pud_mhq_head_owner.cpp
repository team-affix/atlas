#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/box.hpp>
#include <immer/map.hpp>
#include "infrastructure/pud_mhq_head_owner.hpp"

using ::testing::StrictMock;

namespace {

struct MockRemoveQueryHead {
    MOCK_METHOD(void, remove_head, (pud_mhq_head_id));
};

using test_owner_t = pud_mhq_head_owner<StrictMock<MockRemoveQueryHead>>;

struct PudMhqHeadOwnerTest : public ::testing::Test {
    StrictMock<MockRemoveQueryHead> remove_query_head;
};

TEST_F(PudMhqHeadOwnerTest, ExposesItsHeadId) {
    EXPECT_CALL(remove_query_head, remove_head(7u));
    test_owner_t owner{remove_query_head, 7};
    EXPECT_EQ(owner.head_id(), 7u);
}

TEST_F(PudMhqHeadOwnerTest, DestructionRemovesTheHead) {
    EXPECT_CALL(remove_query_head, remove_head(3u)).Times(1);
    { test_owner_t owner{remove_query_head, 3}; }
}

TEST_F(PudMhqHeadOwnerTest, ConstructionDoesNotRemoveAnything) {
    std::optional<test_owner_t> owner;
    owner.emplace(remove_query_head, 3);
    ::testing::Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(3u));
}

TEST_F(PudMhqHeadOwnerTest, SharedBoxRemovesTheHeadOnlyWhenTheLastCopyDies) {
    std::optional<immer::box<test_owner_t>> first{immer::box<test_owner_t>{remove_query_head, 5}};
    std::optional<immer::box<test_owner_t>> second = first;
    first.reset();
    ::testing::Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(5u)).Times(1);
    second.reset();
}

TEST_F(PudMhqHeadOwnerTest, OwnerSharedThroughPersistentMapsDiesWithTheLastMap) {
    using map_t = immer::map<int, immer::box<test_owner_t>>;
    std::optional<map_t> parent{map_t{}.set(0, immer::box<test_owner_t>{remove_query_head, 9})};
    std::optional<map_t> child{parent->set(1, immer::box<test_owner_t>{remove_query_head, 10})};
    EXPECT_CALL(remove_query_head, remove_head(10u)).Times(1);
    child.reset();
    ::testing::Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(9u)).Times(1);
    parent.reset();
}

TEST_F(PudMhqHeadOwnerTest, ShadowingAnEntryKeepsTheParentsOwnerAlive) {
    using map_t = immer::map<int, immer::box<test_owner_t>>;
    std::optional<map_t> parent{map_t{}.set(0, immer::box<test_owner_t>{remove_query_head, 1})};
    std::optional<map_t> child{parent->set(0, immer::box<test_owner_t>{remove_query_head, 2})};
    EXPECT_EQ(parent->find(0)->get().head_id(), 1u);
    EXPECT_EQ(child->find(0)->get().head_id(), 2u);
    EXPECT_CALL(remove_query_head, remove_head(1u)).Times(1);
    parent.reset();
    ::testing::Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(2u)).Times(1);
    child.reset();
}

}  // namespace
