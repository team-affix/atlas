// pud_path_enter: materialize root→dest via try_enter; return last entered.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/pud_path_enter.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

struct MockTryEnter {
    MOCK_METHOD(bool, try_enter, (pud_witness_search_context&, const pud_rule_id*), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_rule_id*, get, (const pud_rule_id*), ());
};

using test_enter_t = pud_path_enter<NiceMock<MockTryEnter>, NiceMock<MockGetParent>>;

struct PudPathEnterTest : public ::testing::Test {
    PudPathEnterTest()
        : query_leaf_{pud_rule_id::axiom{99}}
        , a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , g0_{pud_rule_id::inference{&c0_, 0, &a0_}}
        , enter_(try_enter_, get_parent_) {
        ON_CALL(get_parent_, get(&a0_)).WillByDefault(Return(nullptr));
        ON_CALL(get_parent_, get(&c0_)).WillByDefault(Return(&a0_));
        ON_CALL(get_parent_, get(&g0_)).WillByDefault(Return(&c0_));
        ON_CALL(try_enter_, try_enter(_, _)).WillByDefault(Return(true));
    }

    pud_rule_id query_leaf_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id g0_;
    NiceMock<MockTryEnter> try_enter_;
    NiceMock<MockGetParent> get_parent_;
    test_enter_t enter_;
};

TEST_F(PudPathEnterTest, ReachDestEntersRootThenDest) {
    InSequence seq;
    EXPECT_CALL(try_enter_, try_enter(_, &a0_)).WillOnce(Return(true));
    EXPECT_CALL(try_enter_, try_enter(_, &c0_)).WillOnce(Return(true));
    EXPECT_CALL(try_enter_, try_enter(_, &g0_)).WillOnce(Return(true));
    EXPECT_EQ(enter_.enter_to(&query_leaf_, 0, 1, &g0_), &g0_);
}

TEST_F(PudPathEnterTest, FailAtAxiomReturnsNullptr) {
    EXPECT_CALL(try_enter_, try_enter(_, &a0_)).WillOnce(Return(false));
    EXPECT_CALL(try_enter_, try_enter(_, &c0_)).Times(0);
    EXPECT_EQ(enter_.enter_to(&query_leaf_, 0, 1, &c0_), nullptr);
}

TEST_F(PudPathEnterTest, FailMidPathReturnsLastEntered) {
    InSequence seq;
    EXPECT_CALL(try_enter_, try_enter(_, &a0_)).WillOnce(Return(true));
    EXPECT_CALL(try_enter_, try_enter(_, &c0_)).WillOnce(Return(false));
    EXPECT_CALL(try_enter_, try_enter(_, &g0_)).Times(0);
    EXPECT_EQ(enter_.enter_to(&query_leaf_, 0, 1, &g0_), &a0_);
}

TEST_F(PudPathEnterTest, NullDestReturnsNullptr) {
    EXPECT_CALL(try_enter_, try_enter(_, _)).Times(0);
    EXPECT_EQ(enter_.enter_to(&query_leaf_, 0, 1, nullptr), nullptr);
}
