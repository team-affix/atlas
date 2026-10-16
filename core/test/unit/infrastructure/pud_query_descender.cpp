#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_descender.hpp"

using ::testing::Return;

namespace {

struct ReplayOps {
    MOCK_METHOD(std::optional<int>, descend, (int, pud_node_id));
};

ReplayOps* replay_ops = nullptr;

struct MockReplay {
    int tag;
    std::optional<MockReplay> descend(pud_node_id child) const {
        std::optional<int> child_tag = replay_ops->descend(tag, child);
        if (!child_tag.has_value())
            return std::nullopt;
        return MockReplay{child_tag.value()};
    }
};

struct PudQueryDescenderTest : public ::testing::Test {
    ReplayOps ops;
    pud_query_descender<MockReplay> descender;
    pud_query_descent<MockReplay> parent{.node = 1, .replay = MockReplay{10}};
    pud_node_id child = 2;

    void SetUp() override { replay_ops = &ops; }
    void TearDown() override { replay_ops = nullptr; }
};

TEST_F(PudQueryDescenderTest, ChildDescentIsPositionedAtTheChild) {
    EXPECT_CALL(ops, descend(10, child)).WillOnce(Return(11));
    auto descended = descender.descend(parent, child);
    ASSERT_TRUE(descended.has_value());
    EXPECT_EQ(descended->node, child);
    EXPECT_EQ(descended->replay.tag, 11);
}

TEST_F(PudQueryDescenderTest, FailedReplayHasNoChildDescent) {
    EXPECT_CALL(ops, descend(10, child)).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(descender.descend(parent, child).has_value());
}

TEST_F(PudQueryDescenderTest, ParentIsLeftUntouched) {
    EXPECT_CALL(ops, descend(10, child)).WillOnce(Return(11));
    descender.descend(parent, child);
    EXPECT_EQ(parent.node, 1u);
    EXPECT_EQ(parent.replay.tag, 10);
}

}  // namespace
