#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <optional>
#include <stdexcept>
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
// NiceMock handles the new query_fp_.query() call from the not-already-bound
// assert by returning nullopt (default for optional), so these tests are
// unaffected.
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
// bind() — safety assert: double-bind throws in debug builds.
// Catch bug: bind() does not check whether the variable is already bound.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, BindThrowsIfVariableAlreadyBound) {
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_THROW(hbm_.bind(k_key_0, functor_fe_), std::logic_error);
}

// ---------------------------------------------------------------------------
// whnf() on a functor — must return immediately, touching neither globalize,
// query_fp_, nor record_fp_.
// Catch bug: whnf falls through to the var branch for non-vars.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfOnFunctorReturnsFunctorUnchanged) {
    EXPECT_CALL(globalizer_, globalize(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(query_mock_, query(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(record_mock_, record(::testing::_, ::testing::_,
                                     ::testing::_, ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(functor_fe_);
    EXPECT_EQ(result, functor_fe_);
}

// ---------------------------------------------------------------------------
// whnf() on an unbound variable — query returns nullopt, returns original fe,
// no compression record.
// Catch bug: whnf returns a default-constructed framed_expr on nullopt, or
// calls record even when the variable is unbound.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfOnUnboundVarReturnsOriginalFe) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));
    EXPECT_CALL(record_mock_, record(::testing::_, ::testing::_,
                                     ::testing::_, ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, var0_fe_);
}

// ---------------------------------------------------------------------------
// whnf() single-hop: var0 → functor.
// Compression: record(open_, close_, k_key_0, functor_fe_) is called.
// Catch bug: whnf discards the resolved value, or omits the compression record.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfSingleHopVarToFunctorReturnsFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, functor_fe_))
        .Times(1);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

// ---------------------------------------------------------------------------
// whnf() multi-hop: var0 → var1 → functor.
// Compression fires twice: inner call compresses var1 → functor, then outer
// call compresses var0 → functor.
// Catch bug: whnf only follows one hop, or compresses only the outermost hop.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfMultiHopFollowsChainToFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_1, functor_fe_))
        .Times(1);
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, functor_fe_))
        .Times(1);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

// ---------------------------------------------------------------------------
// whnf() on an unbound var1 directly: single lookup, no compression record.
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfMultiHopEndsAtUnboundVar) {
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));
    EXPECT_CALL(record_mock_, record(::testing::_, ::testing::_,
                                     ::testing::_, ::testing::_)).Times(0);

    const framed_expr result = hbm_.whnf(var1_fe_);
    EXPECT_EQ(result, var1_fe_);
}

// var0 → var1, var1 unbound: result is var1_fe_.
// var0 → var1, var1 unbound: inner whnf(var1) returns early (nullopt, no record).
// Outer: resolved = var1_fe_, record(k_key_0, var1_fe_) fires unconditionally.
// Catch bug: whnf skips compression when chain ends at an unbound var.
TEST_F(HierarchicalBindMapTest, WhnfChainEndingInUnboundVarReturnsLastVar) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .WillOnce(::testing::Return(std::optional<framed_expr>(std::nullopt)));
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, var1_fe_))
        .Times(1);

    const framed_expr result = hbm_.whnf(var0_fe_);
    EXPECT_EQ(result, var1_fe_);
}

// ---------------------------------------------------------------------------
// whnf() path compression eliminates chain on second call.
//
// First whnf(var0): two hops (var0 → var1 → functor), compression fires.
// Second whnf(var0): one hop (var0 → functor directly from compressed entry).
//
// The Times(1) constraints on globalize(7u,1u) and query(k_key_1) prove the
// second call never traverses the chain to var1.
// Catch bug: compression record is written but does not actually shorten the
// next lookup (wrong key, wrong value, or wrong interval recorded).
// ---------------------------------------------------------------------------

TEST_F(HierarchicalBindMapTest, WhnfCompressionEliminatesChainOnSecondCall) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u))
        .WillOnce(::testing::Return(k_key_0))
        .WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u))
        .Times(1)
        .WillOnce(::testing::Return(k_key_1));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(var1_fe_)))
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    EXPECT_CALL(query_mock_, query(SameLabel(open_), k_key_1))
        .Times(1)
        .WillOnce(::testing::Return(std::optional<framed_expr>(functor_fe_)));
    // First call: var0→var1→functor. Inner whnf(functor) fires record(k_key_1).
    //             Outer fires record(k_key_0).
    // Second call: var0→functor (compressed). whnf(functor) is immediate; fires record(k_key_0) again.
    // Total: record(k_key_1) × 1, record(k_key_0) × 2.
    // Times(1) on globalize(7u,1u) and query(k_key_1) proves second call skipped the chain.
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_1, functor_fe_))
        .Times(1);
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, functor_fe_))
        .Times(2);

    const framed_expr first  = hbm_.whnf(var0_fe_);
    const framed_expr second = hbm_.whnf(var0_fe_);
    EXPECT_EQ(first,  functor_fe_);
    EXPECT_EQ(second, functor_fe_);
}

// ---------------------------------------------------------------------------
// Two hierarchical_bind_maps over the same underlying array but different
// node intervals: each must query and compress at its own open/close labels.
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

    const framed_expr result_a = make_functor_fe(77);
    const framed_expr result_b = make_functor_fe(88);

    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(query_mock_,  query(SameLabel(open_),       k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(result_a)));
    EXPECT_CALL(record_mock_,
                record(SameLabel(open_), SameLabel(close_), k_key_0, result_a))
        .Times(1);

    EXPECT_CALL(glob2, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(qry2,  query(SameLabel(other_open), k_key_0))
        .WillOnce(::testing::Return(std::optional<framed_expr>(result_b)));
    EXPECT_CALL(rec2,
                record(SameLabel(other_open), SameLabel(other_close), k_key_0, result_b))
        .Times(1);

    EXPECT_EQ(hbm_.whnf(var0_fe_),       result_a);
    EXPECT_EQ(other_hbm.whnf(var0_fe_),  result_b);
}
