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
    MOCK_METHOD((std::optional<pud_witness_advance_result<int, int>>), advance, ());
};

struct MockHead {
    static HeadOps* ops;
    std::optional<const pud_node*> resume() const { return ops->resume(); }
    std::optional<pud_witness_advance_result<int, int>> advance() { return ops->advance(); }
};

HeadOps* MockHead::ops = nullptr;

struct MockMakeHead {
    MOCK_METHOD(MockHead, make, (int));
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
    EXPECT_FALSE(mhws.try_add_head(1).has_value());
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
}

TEST_F(PudMhwsTest, RemoveOfUnknownIdLeavesALiveSearch) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a)).WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(1);
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
    auto id = mhws.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    EXPECT_FALSE(mhws.try_fork_head(*id, 2).has_value());
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_b).empty());
}

TEST_F(PudMhwsTest, InvalidateMovesSearchOntoTheNewLeaf) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(&leaf_a))
        .WillOnce(Return(&leaf_b))
        .WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
    auto found = mhws.invalidate_leaf(&leaf_b);
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0], *id);
    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
}

TEST_F(PudMhwsTest, InvalidateOfALeafWithNoNewLeafReturnsThatId) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a)).WillOnce(Return(std::nullopt));
    auto id = mhws.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    auto gone = mhws.invalidate_leaf(&leaf_a);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhwsTest, AdvanceWithResultLeavesHeadLinked) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a));
    auto id = mhws.try_add_head(1);
    ASSERT_TRUE(id.has_value());

    pud_witness_advance_result<int, int> peel{.root_handle = 2, .root_next_sibling_it = 0, .root_end_sibling_it = 0};
    EXPECT_CALL(ops, advance()).WillOnce(Return(std::optional<pud_witness_advance_result<int, int>>{peel}));
    auto result = mhws.advance_head(*id);
    ASSERT_TRUE(result.has_value());

    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto gone = mhws.invalidate_leaf(&leaf_a);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhwsTest, AdvanceWithNulloptRemovesHead) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(&leaf_a));
    auto id = mhws.try_add_head(1);
    ASSERT_TRUE(id.has_value());

    EXPECT_CALL(ops, advance()).WillOnce(Return(std::nullopt));
    auto result = mhws.advance_head(*id);
    EXPECT_FALSE(result.has_value());

    EXPECT_TRUE(mhws.invalidate_leaf(&leaf_a).empty());
}

} // namespace
