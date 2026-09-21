// pud_query_starter: intern query root, allocate child of forest, bind hole.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include "infrastructure/pud_query_starter.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockMakeInference {
    MOCK_METHOD(const pud_rule_id*, make_inference,
                (const pud_rule_id*, size_t, const pud_rule_id*), ());
};

struct MockGetInterval {
    MOCK_METHOD(const om_interval&, get, (const pud_rule_id*), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockStoreInterval {
    MOCK_METHOD(void, store, (const pud_rule_id*, om_interval), ());
};

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t), ());
};

struct MockRecordBinding {
    MOCK_METHOD(void, record, (om_interval, uint32_t, framed_expr), ());
};

struct MockQueryBinding {
    MOCK_METHOD(std::optional<framed_expr>, query, (om_label, uint32_t), ());
};

using test_starter_t = pud_query_starter<
    NiceMock<MockMakeInference>,
    NiceMock<MockGetInterval>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockStoreInterval>,
    NiceMock<MockGlobalize>,
    NiceMock<MockRecordBinding>,
    NiceMock<MockQueryBinding>>;

struct PudQueryStarterTest : public ::testing::Test {
    PudQueryStarterTest()
        : body_{expr::functor{1, {}}}
        , leaf_{pud_rule_id::axiom{0}}
        , query_key_{pud_rule_id::inference{&leaf_, 0, nullptr}}
        , forest_open_(10)
        , forest_close_(40)
        , query_open_(20)
        , query_close_(30)
        , forest_{om_label(&forest_open_), om_label(&forest_close_)}
        , query_interval_{om_label(&query_open_), om_label(&query_close_)}
        , starter_(make_inference_, get_interval_, allocate_, store_interval_,
                   globalize_, record_, query_binding_) {
        ON_CALL(make_inference_, make_inference(&leaf_, 0, nullptr))
            .WillByDefault(Return(&query_key_));
        ON_CALL(get_interval_, get(&leaf_)).WillByDefault(ReturnRef(forest_));
        ON_CALL(allocate_, allocate_child_of(_)).WillByDefault(Return(query_interval_));
        ON_CALL(globalize_, globalize(_, _))
            .WillByDefault([](uint32_t offset, uint32_t idx) { return offset + idx; });
        ON_CALL(query_binding_, query(_, _))
            .WillByDefault(Return(std::nullopt));
    }

    expr body_;
    pud_rule_id leaf_;
    pud_rule_id query_key_;
    uint64_t forest_open_;
    uint64_t forest_close_;
    uint64_t query_open_;
    uint64_t query_close_;
    om_interval forest_;
    om_interval query_interval_;
    NiceMock<MockMakeInference> make_inference_;
    NiceMock<MockGetInterval> get_interval_;
    NiceMock<MockAllocateChildInterval> allocate_;
    NiceMock<MockStoreInterval> store_interval_;
    NiceMock<MockGlobalize> globalize_;
    NiceMock<MockRecordBinding> record_;
    NiceMock<MockQueryBinding> query_binding_;
    test_starter_t starter_;
};

TEST_F(PudQueryStarterTest, InternsNullCalleeAndStoresChildOfForest) {
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, nullptr))
        .WillOnce(Return(&query_key_));
    EXPECT_CALL(get_interval_, get(&leaf_)).WillOnce(ReturnRef(forest_));
    EXPECT_CALL(allocate_, allocate_child_of(forest_)).WillOnce(Return(query_interval_));
    EXPECT_CALL(store_interval_, store(&query_key_, query_interval_));
    starter_.start(&leaf_, 0, &body_, 4);
}

TEST_F(PudQueryStarterTest, BindsHoleAtQueryLvcNotCallerZero) {
    framed_expr bound{nullptr, 0};
    uint32_t bound_key = 0;
    EXPECT_CALL(record_, record(query_interval_, _, _))
        .WillOnce([&](om_interval, uint32_t key, framed_expr value) {
            bound_key = key;
            bound = value;
        });
    starter_.start(&leaf_, 0, &body_, 4);
    EXPECT_NE(bound_key, 0u);
    EXPECT_EQ(bound_key, 4u);
    EXPECT_EQ(bound.skeleton, &body_);
    EXPECT_EQ(bound.frame_offset, 0u);
}
