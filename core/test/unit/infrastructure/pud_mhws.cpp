#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhws.hpp"

using ::testing::_;
using ::testing::Return;

namespace {

struct HeadOps {
    MOCK_METHOD((std::optional<const pud_node*>), resume, ());
};

struct MockHead {
    static HeadOps* ops;
    std::optional<const pud_node*> resume() const { return ops->resume(); }
};

HeadOps* MockHead::ops = nullptr;

struct MockMakeHead {
    MOCK_METHOD(MockHead, make, (pud_query_position<int>));
};

struct MockForkHead {
    MOCK_METHOD(MockHead, fork, (const MockHead&, int));
};

using test_mhws_t = pud_mhws<int, int, MockHead, MockMakeHead, MockForkHead>;

struct PudMhwsTest : public ::testing::Test {
    HeadOps ops;
    MockMakeHead make_head;
    MockForkHead fork_head;
    test_mhws_t mhws{make_head, fork_head};
    pud_node leaf_a{};
    pud_node leaf_b{};

    void SetUp() override {
        MockHead::ops = &ops;
        ON_CALL(make_head, make(_)).WillByDefault(Return(MockHead{}));
        ON_CALL(fork_head, fork(_, _)).WillByDefault(Return(MockHead{}));
    }
};

TEST_F(PudMhwsTest, AddThatFindsNoLeafIsNotRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(mhws.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf_a}).has_value());
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
}

TEST_F(PudMhwsTest, RemoveOfUnknownIdLeavesALiveSearch) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a)).WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf_a});
    ASSERT_TRUE(id.has_value());
    mhws.remove_head(*id + 100);
    auto gone = mhws.invalidate_leaf(&leaf_a);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhwsTest, InvalidateOfUnoccupiedLeafIsEmpty) {
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
}

TEST_F(PudMhwsTest, ForkThatFindsNoLeafIsNotRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a)).WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf_a});
    ASSERT_TRUE(id.has_value());
    EXPECT_FALSE(mhws.try_fork_head(*id, 2).has_value());
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_b).empty());
}

TEST_F(PudMhwsTest, InvalidateMovesSearchOntoTheNewLeaf) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(&leaf_a))
        .WillOnce(Return(&leaf_b))
        .WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf_a});
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
    auto found = mhws.invalidate_leaf(&leaf_b);
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0], *id);
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
}

TEST_F(PudMhwsTest, InvalidateOfALeafWithNoNewLeafReturnsThatId) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a)).WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf_a});
    ASSERT_TRUE(id.has_value());
    auto gone = mhws.invalidate_leaf(&leaf_a);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

} // namespace
