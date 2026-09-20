// pud_unify_head: unify_head/reinit take the query; path replay into nested or query interval.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/pud_unify_head.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockTryParent {
    MOCK_METHOD(const pud_rule_id*, try_parent, (const pud_rule_id*), ());
};

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

struct MockRecordBinding {
    MOCK_METHOD(void, record, (om_interval, uint32_t, framed_expr), ());
};

struct MockQueryBinding {
    MOCK_METHOD(std::optional<framed_expr>, query, (om_label, uint32_t), ());
};

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t), ());
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, const std::vector<const expr*>&), ());
};

using test_unify_head_t = pud_unify_head<
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockTryParent>,
    NiceMock<MockGetNode>,
    NiceMock<MockRecordBinding>,
    NiceMock<MockQueryBinding>,
    NiceMock<MockGlobalize>,
    NiceMock<MockMakeVar>,
    NiceMock<MockMakeFunctor>>;

struct PudUnifyHeadTest : public ::testing::Test {
    PudUnifyHeadTest()
        : open_(10)
        , close_(40)
        , nested_open_(15)
        , nested_close_(20)
        , interval_{om_label(&open_), om_label(&close_)}
        , nested_{om_label(&nested_open_), om_label(&nested_close_)}
        , pred_{expr::functor{7, {}}}
        , var0_{expr::var{0}}
        , axiom_{pud_rule_id::axiom{0}}
        , mid_{pud_rule_id::inference{&axiom_, 0, &axiom_}}
        , leaf_{pud_rule_id::inference{&mid_, 0, &axiom_}}
        , node_{interval_, {{0, &pred_}}, {}, 1}
        , node_mid_{interval_, {{1, &pred_}}, {}, 2}
        , node_leaf_{interval_, {{2, &pred_}}, {}, 3}
        , query_{interval_, &pred_, {pud_candidate_search_context{&axiom_, {}}}, 1}
        , unify_head_(allocate_, try_parent_, get_node_, record_, query_binding_,
                      globalize_, make_var_, make_functor_) {
        ON_CALL(try_parent_, try_parent(_)).WillByDefault(Return(nullptr));
        ON_CALL(get_node_, get_node(&axiom_)).WillByDefault(ReturnRef(node_));
        ON_CALL(get_node_, get_node(&mid_)).WillByDefault(ReturnRef(node_mid_));
        ON_CALL(get_node_, get_node(&leaf_)).WillByDefault(ReturnRef(node_leaf_));
        ON_CALL(allocate_, allocate_child_of(_)).WillByDefault(Return(nested_));
        ON_CALL(make_var_, make_var(0)).WillByDefault(Return(&var0_));
        ON_CALL(globalize_, globalize(_, _)).WillByDefault([](uint32_t frame, uint32_t idx) {
            return frame + idx;
        });
        ON_CALL(query_binding_, query(_, 0)).WillByDefault(Return(framed_expr{&pred_, 0}));
    }

    uint64_t open_;
    uint64_t close_;
    uint64_t nested_open_;
    uint64_t nested_close_;
    om_interval interval_;
    om_interval nested_;
    expr pred_;
    expr var0_;
    pud_rule_id axiom_;
    pud_rule_id mid_;
    pud_rule_id leaf_;
    pud_db_node node_;
    pud_db_node node_mid_;
    pud_db_node node_leaf_;
    pud_query query_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockTryParent> try_parent_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockRecordBinding> record_;
    NiceMock<MockQueryBinding> query_binding_;
    NiceMock<MockGlobalize> globalize_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockMakeFunctor> make_functor_;
    test_unify_head_t unify_head_;
};

TEST_F(PudUnifyHeadTest, UnifyHeadRecordsPathAndSucceedsWhenHeadMatches) {
    EXPECT_CALL(allocate_, allocate_child_of(_)).WillOnce(Return(nested_));
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    EXPECT_TRUE(unify_head_.unify_head(query_, &axiom_));
}

TEST_F(PudUnifyHeadTest, UnifyHeadFailsWhenRecordedHeadDiffersFromBody) {
    expr other{expr::functor{8, {}}};
    ON_CALL(query_binding_, query(_, 0)).WillByDefault(Return(framed_expr{&other, 0}));
    EXPECT_FALSE(unify_head_.unify_head(query_, &axiom_));
}

TEST_F(PudUnifyHeadTest, ReinitRecordsEachNodeOnAThreeNodeChain) {
    query_.axiom_contexts = {pud_candidate_search_context{&leaf_, {}}};
    query_.frame_offset = 3;
    EXPECT_CALL(try_parent_, try_parent(&leaf_)).WillRepeatedly(Return(&mid_));
    EXPECT_CALL(try_parent_, try_parent(&mid_)).WillRepeatedly(Return(&axiom_));
    EXPECT_CALL(try_parent_, try_parent(&axiom_)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(record_, record(_, 0, _)).Times(::testing::AtLeast(1));
    EXPECT_CALL(record_, record(_, 1, _)).Times(::testing::AtLeast(1));
    EXPECT_CALL(record_, record(_, 2, _)).Times(::testing::AtLeast(1));
    unify_head_.reinit(query_);
}
