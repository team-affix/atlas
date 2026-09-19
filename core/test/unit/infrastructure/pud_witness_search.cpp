// pud_witness_search: accept-first resume; DFS descend then next sibling; stop at edge_root.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

struct MockIsLeaf {
    MOCK_METHOD(bool, is_leaf, (const pud_rule_id*), ());
};

struct MockOrderedChildren {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_children, (const pud_rule_id*), ());
};

struct MockParent {
    MOCK_METHOD(const pud_rule_id*, parent, (const pud_rule_id*), ());
};

struct MockUnifyHead {
    MOCK_METHOD(bool, unify_head, (const pud_rule_id*), ());
};

using test_search_t = pud_witness_search<NiceMock<MockIsLeaf>,
                                         NiceMock<MockOrderedChildren>,
                                         NiceMock<MockParent>,
                                         NiceMock<MockUnifyHead>>;

struct PudWitnessSearchTest : public ::testing::Test {
    PudWitnessSearchTest()
        : a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , c1_{pud_rule_id::inference{&a0_, 1, &a0_}}
        , g0_{pud_rule_id::inference{&c0_, 0, &a0_}}
        , search_(is_leaf_, children_, parent_, unify_) {}

    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    pud_rule_id g0_;
    NiceMock<MockIsLeaf> is_leaf_;
    NiceMock<MockOrderedChildren> children_;
    NiceMock<MockParent> parent_;
    NiceMock<MockUnifyHead> unify_;
    test_search_t search_;
};

TEST_F(PudWitnessSearchTest, AcceptsCurrentIfItIsUnifyingLeaf) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&a0_)).WillRepeatedly(Return(true));
    const pud_witness_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, DescendsIntoChildrenWhenCurrentIsNoLongerALeaf) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    const pud_witness_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &c0_);
}

TEST_F(PudWitnessSearchTest, PrunesSubtreeWhenUnifyFails) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(_)).Times(0);
    const pud_witness_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::failed>(result.content));
}

TEST_F(PudWitnessSearchTest, TriesNextSiblingInIdOrderAfterFailedChild) {
    pud_witness_search_context ctx{&a0_, &c0_};
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(is_leaf_, is_leaf(&c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&c0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(&c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(parent_, parent(&c0_)).WillRepeatedly(Return(&a0_));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    const pud_witness_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, StopsAtEdgeRootAndFailsWhenNoSiblingWorks) {
    pud_witness_search_context ctx{&c0_, &g0_};
    EXPECT_CALL(is_leaf_, is_leaf(&g0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(parent_, parent(&g0_)).WillRepeatedly(Return(&c0_));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&g0_}));
    const pud_witness_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::failed>(result.content));
}

TEST_F(PudWitnessSearchTest, SelfNodeMayBeAWitness) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&a0_)).WillRepeatedly(Return(true));
    const pud_witness_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
}
