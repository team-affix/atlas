// pud_node_base_interval: bind_root / bind_child allocate and store.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include "infrastructure/pud_node_base_interval.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

struct MockAllocateRootInterval {
    MOCK_METHOD(om_interval, allocate_root, (), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

using test_interval_t = pud_node_base_interval<
    NiceMock<MockAllocateRootInterval>,
    NiceMock<MockAllocateChildInterval>>;

struct PudNodeBaseIntervalTest : public ::testing::Test {
    PudNodeBaseIntervalTest()
        : root_open_(10)
        , root_close_(40)
        , child_open_(15)
        , child_close_(20)
        , root_interval_{om_label(&root_open_), om_label(&root_close_)}
        , child_interval_{om_label(&child_open_), om_label(&child_close_)}
        , axiom_{pud_rule_id::axiom{0}}
        , child_{pud_rule_id::inference{&axiom_, 0, &axiom_}}
        , intervals_(allocate_root_, allocate_child_) {}

    uint64_t root_open_;
    uint64_t root_close_;
    uint64_t child_open_;
    uint64_t child_close_;
    om_interval root_interval_;
    om_interval child_interval_;
    pud_rule_id axiom_;
    pud_rule_id child_;
    NiceMock<MockAllocateRootInterval> allocate_root_;
    NiceMock<MockAllocateChildInterval> allocate_child_;
    test_interval_t intervals_;
};

TEST_F(PudNodeBaseIntervalTest, BindRootStoresAllocatedInterval) {
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_));
    intervals_.bind_root(&axiom_);
    EXPECT_EQ(intervals_.get(&axiom_).open.rank_ptr(), root_interval_.open.rank_ptr());
}

TEST_F(PudNodeBaseIntervalTest, BindChildNestsUnderParent) {
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));
    intervals_.bind_root(&axiom_);
    intervals_.bind_child(&child_, &axiom_);
    EXPECT_EQ(intervals_.get(&child_).open.rank_ptr(), child_interval_.open.rank_ptr());
}

TEST_F(PudNodeBaseIntervalTest, GetUnknownThrows) {
    EXPECT_THROW(intervals_.get(&axiom_), std::out_of_range);
}
