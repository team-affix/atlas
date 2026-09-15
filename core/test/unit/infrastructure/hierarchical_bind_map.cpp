#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <optional>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_label.hpp"

// ---------------------------------------------------------------------------
// Mocks
// ---------------------------------------------------------------------------

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t frame_offset, uint32_t var_index), ());
};

struct MockRecordFPArrayBinding {
    MOCK_METHOD(void, record,
                (om_label open, om_label close, uint32_t var_id, framed_expr value), ());
};

struct MockQueryFPArrayBinding {
    MOCK_METHOD(std::optional<framed_expr>, query,
                (om_label open_label, uint32_t var_id), ());
};

using test_hbm_t = hierarchical_bind_map<
    ::testing::NiceMock<MockGlobalize>,
    ::testing::NiceMock<MockRecordFPArrayBinding>,
    ::testing::NiceMock<MockQueryFPArrayBinding>>;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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

// Matches an om_label by identity of its rank pointer.
::testing::Matcher<om_label> SameLabel(const om_label& expected) {
    return ::testing::Property(&om_label::rank_ptr, expected.rank_ptr());
}

} // namespace

// ---------------------------------------------------------------------------
// Fixture
//
// open_rank_ / close_rank_ are stable uint64_t members whose addresses are
// used to construct om_label objects.  The hbm_ member is constructed last
// (declared last) so all injected references are already valid.
// ---------------------------------------------------------------------------

struct HierarchicalBindMapTest : public ::testing::Test {
    HierarchicalBindMapTest()
        : open_rank_(10)
        , close_rank_(20)
        , open_(&open_rank_)
        , close_(&close_rank_)
        , hbm_(globalizer_, record_mock_, query_mock_, open_, close_)
        , functor_fe_(make_functor_fe(42))
        , var0_fe_(make_var_fe(0, 5))
        , var1_fe_(make_var_fe(1, 7)) {}

    uint64_t open_rank_;
    uint64_t close_rank_;
    om_label open_;
    om_label close_;
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

// ---------------------------------------------------------------------------
// bind() — delegation tests
// Verify that bind() passes the map's own open/close labels and the caller's
// key/value to record_fp_.record(), unchanged.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, BindDelegatesToRecordWithConstructionLabels) {
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, functor_fe_))
        .Times(1);
    hbm_.bind(k_key_0, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, BindPassesThroughVarFe) {
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_1, var0_fe_))
        .Times(1);
    hbm_.bind(k_key_1, var0_fe_);
}

TEST_F(HierarchicalBindMapTest, MultipleBindsEachDelegateSeparately) {
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, functor_fe_))
        .Times(1);
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_1, var0_fe_))
        .Times(1);
    hbm_.bind(k_key_0, functor_fe_);
    hbm_.bind(k_key_1, var0_fe_);
}

// ---------------------------------------------------------------------------
// whnf() on a functor — must return immediately, touching neither globalize
// nor query_fp_.
// Catch bug: whnf falls through to the var branch for non-vars.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfOnFunctorReturnsFunctorUnchanged) {
    EXPECT_CALL(globalizer_, globalize(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(query_mock_, query(::testing::_, ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(functor_fe_);
    EXPECT_EQ(result, functor_fe_);
}

// ---------------------------------------------------------------------------
// whnf() on an unbound variable — query returns nullopt, so the original
// var framed_expr is returned as-is.
// Catch bug: whnf returns a default-constructed framed_expr on nullopt.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfOnUnboundVarReturnsOriginalFe) {
    // var0_fe_: var_index=0, frame_offset=5 → global_key=k_key_0
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, var0_fe_);
}

// ---------------------------------------------------------------------------
// whnf() single-hop: var → functor.
// Catch bug: whnf discards the resolved value and returns the input instead.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfSingleHopVarToFunctorReturnsFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

// ---------------------------------------------------------------------------
// whnf() multi-hop: var0 → var1 → functor.
// Verifies that the recursive call correctly re-enters whnf on the
// intermediate result, resolving the full chain.
// Catch bug: whnf only follows one hop and returns a var when it should
// keep resolving.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfMultiHopFollowsChainToFunctor) {
    // hop 1: var0 (frame=5, idx=0) → global k_key_0 → resolves to var1_fe_
    // hop 2: var1 (frame=7, idx=1) → global k_key_1 → resolves to functor_fe_
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

// ---------------------------------------------------------------------------
// whnf() on an unbound var1 directly: single lookup, returns var1_fe_.
// Catch bug: whnf keeps looping past the nullopt and crashes or returns wrong.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfMultiHopEndsAtUnboundVar) {
    // whnf(var1_fe_) directly: var1 (frame=7, idx=1) → k_key_1 → nullopt.
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));

    const framed_expr result = hbm_.whnf(var1_fe_);
    EXPECT_EQ(result, var1_fe_);
}

// whnf(var0_fe_) where var0→var1 and var1 is unbound: result is var1_fe_.
TEST_F(HierarchicalBindMapTest, WhnfChainEndingInUnboundVarReturnsLastVar) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, var1_fe_);
}

// ---------------------------------------------------------------------------
// Two hierarchical_bind_maps over the same underlying array but different
// node intervals: each must query at its own open label.
// Catch bug: label stored by value but silently shared/aliased.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, TwoMapsWithDifferentLabelsQueryAtOwnLabel) {
    uint64_t other_open_rank  = 50;
    uint64_t other_close_rank = 60;
    om_label other_open(&other_open_rank);
    om_label other_close(&other_close_rank);

    ::testing::NiceMock<MockGlobalize>             glob2;
    ::testing::NiceMock<MockRecordFPArrayBinding>  rec2;
    ::testing::NiceMock<MockQueryFPArrayBinding>   qry2;
    test_hbm_t other_hbm(glob2, rec2, qry2, other_open, other_close);

    // hbm_ queries at open_; other_hbm queries at other_open.
    const framed_expr result_a = make_functor_fe(77);
    const framed_expr result_b = make_functor_fe(88);

    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_,  query(SameLabel(open_),       k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(result_a)));

    EXPECT_CALL(glob2, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(qry2,  query(SameLabel(other_open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(result_b)));

    EXPECT_EQ(hbm_.whnf(var0_fe_),       result_a);
    EXPECT_EQ(other_hbm.whnf(var0_fe_),  result_b);
}
