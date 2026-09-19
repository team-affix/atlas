// pud_query_reinitializer: O(depth) path replay; fail at an internal node skips below.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_reinitializer.hpp"
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

struct MockTryParent {
    MOCK_METHOD(const pud_rule_id*, try_parent, (const pud_rule_id*), ());
};

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

struct MockRecordBinding {
    MOCK_METHOD(void, record, (om_interval, uint32_t, framed_expr), ());
};

struct MockWhnf {
    MOCK_METHOD(framed_expr, whnf, (framed_expr), ());
};

struct MockUnify {
    MOCK_METHOD(bool, unify, (framed_expr), ());
};

using test_reinit_t = pud_query_reinitializer<
    NiceMock<MockTryParent>,
    NiceMock<MockGetNode>,
    NiceMock<MockRecordBinding>,
    NiceMock<MockWhnf>,
    NiceMock<MockUnify>>;

struct PudQueryReinitializerTest : public ::testing::Test {
    PudQueryReinitializerTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , val_root_{expr::functor{1, {}}}
        , val_mid_{expr::functor{2, {}}}
        , val_leaf_{expr::functor{3, {}}}
        , root_{pud_rule_id::axiom{0}}
        , mid_{pud_rule_id::inference{&root_, 0, &root_}}
        , leaf_{pud_rule_id::inference{&mid_, 0, &root_}}
        , node_root_{interval_, {{0, &val_root_}}, {}, 1}
        , node_mid_{interval_, {{1, &val_mid_}}, {}, 2}
        , node_leaf_{interval_, {{2, &val_leaf_}}, {}, 3}
        , reinit_(try_parent_, get_node_, record_, whnf_, unify_) {
        ON_CALL(whnf_, whnf(_)).WillByDefault([](framed_expr fe) { return fe; });
    }

    pud_query make_query(const pud_rule_id* cursor) {
        return pud_query{interval_, &body_, {pud_candidate_search_context{cursor, {}}}};
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    expr val_root_;
    expr val_mid_;
    expr val_leaf_;
    pud_rule_id root_;
    pud_rule_id mid_;
    pud_rule_id leaf_;
    pud_db_node node_root_;
    pud_db_node node_mid_;
    pud_db_node node_leaf_;
    NiceMock<MockTryParent> try_parent_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockRecordBinding> record_;
    NiceMock<MockWhnf> whnf_;
    NiceMock<MockUnify> unify_;
    test_reinit_t reinit_;
};

TEST_F(PudQueryReinitializerTest, ThreeNodeChainRecordsEachNode) {
    pud_query query = make_query(&leaf_);
    EXPECT_CALL(try_parent_, try_parent(&leaf_)).WillRepeatedly(Return(&mid_));
    EXPECT_CALL(try_parent_, try_parent(&mid_)).WillRepeatedly(Return(&root_));
    EXPECT_CALL(try_parent_, try_parent(&root_)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(get_node_, get_node(&root_)).WillRepeatedly(ReturnRef(node_root_));
    EXPECT_CALL(get_node_, get_node(&mid_)).WillRepeatedly(ReturnRef(node_mid_));
    EXPECT_CALL(get_node_, get_node(&leaf_)).WillRepeatedly(ReturnRef(node_leaf_));
    EXPECT_CALL(record_, record(_, 0, _));
    EXPECT_CALL(record_, record(_, 1, _));
    EXPECT_CALL(record_, record(_, 2, _));
    EXPECT_CALL(unify_, unify(_)).Times(3).WillRepeatedly(Return(true));

    reinit_.reinit(query, 3);
}

TEST_F(PudQueryReinitializerTest, FailAtInternalNodeDoesNotVisitBelow) {
    pud_query query = make_query(&leaf_);
    EXPECT_CALL(try_parent_, try_parent(&leaf_)).WillRepeatedly(Return(&mid_));
    EXPECT_CALL(try_parent_, try_parent(&mid_)).WillRepeatedly(Return(&root_));
    EXPECT_CALL(try_parent_, try_parent(&root_)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(get_node_, get_node(&root_)).WillRepeatedly(ReturnRef(node_root_));
    EXPECT_CALL(get_node_, get_node(&mid_)).WillRepeatedly(ReturnRef(node_mid_));
    EXPECT_CALL(get_node_, get_node(&leaf_)).Times(0);
    EXPECT_CALL(record_, record(_, 0, _));
    EXPECT_CALL(record_, record(_, 1, _));
    EXPECT_CALL(unify_, unify(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    reinit_.reinit(query, 3);
}
