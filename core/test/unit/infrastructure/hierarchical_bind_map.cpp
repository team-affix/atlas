#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <optional>
#include <stdexcept>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t frame_offset, uint32_t var_index), ());
};

struct MockRecordFPArrayBinding {
    MOCK_METHOD(void, record,
                (om_interval interval, uint32_t var_id, framed_expr value), ());
};

struct MockQueryFPArrayBinding {
    MOCK_METHOD(std::optional<framed_expr>, query,
                (om_label open_label, uint32_t var_id), ());
};

using test_hbm_t = hierarchical_bind_map<
    ::testing::NiceMock<MockGlobalize>,
    ::testing::NiceMock<MockRecordFPArrayBinding>,
    ::testing::NiceMock<MockQueryFPArrayBinding>>;

namespace {

framed_expr make_functor_fe(uint32_t functor_id, uint32_t frame_offset = 0) {
    static std::deque<expr> exprs;
    exprs.push_back(expr{expr::functor{functor_id, {}}});
    return framed_expr{&exprs.back(), frame_offset};
}

framed_expr make_var_fe(uint32_t var_index, uint32_t frame_offset) {
    static std::deque<expr> exprs;
    exprs.push_back(expr{expr::var{var_index}});
    return framed_expr{&exprs.back(), frame_offset};
}

::testing::Matcher<om_label> SameLabel(const om_label& expected) {
    return ::testing::Property(&om_label::rank_ptr, expected.rank_ptr());
}

::testing::Matcher<om_interval> SameInterval(const om_interval& expected) {
    return ::testing::AllOf(
        ::testing::Field(&om_interval::open, SameLabel(expected.open)),
        ::testing::Field(&om_interval::close, SameLabel(expected.close)));
}

}

struct HierarchicalBindMapTest : public ::testing::Test {
    HierarchicalBindMapTest()
        : open_rank_(10)
        , close_rank_(20)
        , interval_{om_label(&open_rank_), om_label(&close_rank_)}
        , hbm_(globalizer_, record_mock_, query_mock_, interval_)
        , functor_fe_(make_functor_fe(42))
        , var0_fe_(make_var_fe(0, 5))
        , var1_fe_(make_var_fe(1, 7)) {}

    uint64_t open_rank_;
    uint64_t close_rank_;
    om_interval interval_;
    ::testing::NiceMock<MockGlobalize>             globalizer_;
    ::testing::NiceMock<MockRecordFPArrayBinding>  record_mock_;
    ::testing::NiceMock<MockQueryFPArrayBinding>   query_mock_;
    test_hbm_t hbm_;

    const framed_expr functor_fe_;
    const framed_expr var0_fe_;
    const framed_expr var1_fe_;

    static constexpr uint32_t k_key_0 = 100;
    static constexpr uint32_t k_key_1 = 101;
};

TEST_F(HierarchicalBindMapTest, BindDelegatesToRecordWithConstructionLabels) {
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, functor_fe_))
        .Times(1);
    hbm_.bind(k_key_0, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, BindPassesThroughVarFe) {
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_1, var0_fe_))
        .Times(1);
    hbm_.bind(k_key_1, var0_fe_);
}

TEST_F(HierarchicalBindMapTest, MultipleBindsEachDelegateSeparately) {
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, functor_fe_))
        .Times(1);
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_1, var0_fe_))
        .Times(1);
    hbm_.bind(k_key_0, functor_fe_);
    hbm_.bind(k_key_1, var0_fe_);
}

TEST_F(HierarchicalBindMapTest, BindThrowsIfVariableAlreadyBound) {
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_THROW(hbm_.bind(k_key_0, functor_fe_), std::logic_error);
}

TEST_F(HierarchicalBindMapTest, WhnfOnFunctorReturnsFunctorUnchanged) {
    EXPECT_CALL(globalizer_, globalize(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(query_mock_, query(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(record_mock_, record(::testing::_, ::testing::_,
                                     ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(functor_fe_);
    EXPECT_EQ(result, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfOnUnboundVarReturnsOriginalFe) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));
    EXPECT_CALL(record_mock_, record(::testing::_, ::testing::_,
                                     ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, var0_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfSingleHopVarToFunctorReturnsFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, functor_fe_))
        .Times(1);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfMultiHopFollowsChainToFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_1, functor_fe_))
        .Times(1);
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, functor_fe_))
        .Times(1);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfMultiHopEndsAtUnboundVar) {
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));
    EXPECT_CALL(record_mock_, record(::testing::_, ::testing::_,
                                     ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(var1_fe_);
    EXPECT_EQ(result, var1_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfChainEndingInUnboundVarReturnsLastVar) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, var1_fe_))
        .Times(1);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, var1_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfCompressionEliminatesChainOnSecondCall) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u))
        .WillOnce(::testing::Return(k_key_0))
        .WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u))
        .Times(1)
        .WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(interval_.open), k_key_1))
        .Times(1)
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));

    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_1, functor_fe_))
        .Times(1);
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, functor_fe_))
        .Times(2);

    const framed_expr first  = hbm_.whnf(var0_fe_);
    const framed_expr second = hbm_.whnf(var0_fe_);
    EXPECT_EQ(first,  functor_fe_);
    EXPECT_EQ(second, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, TwoMapsWithDifferentLabelsQueryAtOwnLabel) {
    uint64_t other_open_rank  = 50;
    uint64_t other_close_rank = 60;
    om_interval other_interval{om_label(&other_open_rank), om_label(&other_close_rank)};

    ::testing::NiceMock<MockGlobalize>             glob2;
    ::testing::NiceMock<MockRecordFPArrayBinding>  rec2;
    ::testing::NiceMock<MockQueryFPArrayBinding>   qry2;
    test_hbm_t other_hbm(glob2, rec2, qry2, other_interval);

    const framed_expr result_a = make_functor_fe(77);
    const framed_expr result_b = make_functor_fe(88);

    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_,  query(SameLabel(interval_.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(result_a)));
    EXPECT_CALL(record_mock_,
                record(SameInterval(interval_), k_key_0, result_a))
        .Times(1);

    EXPECT_CALL(glob2, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(qry2,  query(SameLabel(other_interval.open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(result_b)));
    EXPECT_CALL(rec2,
                record(SameInterval(other_interval), k_key_0, result_b))
        .Times(1);

    EXPECT_EQ(hbm_.whnf(var0_fe_),       result_a);
    EXPECT_EQ(other_hbm.whnf(var0_fe_),  result_b);
}
