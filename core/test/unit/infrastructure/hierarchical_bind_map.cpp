#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t frame_offset, uint32_t var_index), ());
};

using test_hbm_t = hierarchical_bind_map<
    ::testing::NiceMock<MockGlobalize>,
    immer::map<uint32_t, framed_expr>::transient_type>;

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

}

struct HierarchicalBindMapTest : public ::testing::Test {
    ::testing::NiceMock<MockGlobalize> globalizer_;

    const framed_expr functor_fe_ = make_functor_fe(42);
    const framed_expr var0_fe_    = make_var_fe(0, 5);
    const framed_expr var1_fe_    = make_var_fe(1, 7);

    static constexpr uint32_t k_key_0 = 100;
    static constexpr uint32_t k_key_1 = 101;
};

TEST_F(HierarchicalBindMapTest, PersistentOfEmptyBaseReturnsEmptyMap) {
    auto transient = immer::map<uint32_t, framed_expr>{}.transient();
    test_hbm_t hbm(globalizer_, transient);
    const auto frozen = std::move(transient).persistent();
    EXPECT_EQ(frozen.size(), 0u);
}

TEST_F(HierarchicalBindMapTest, BindAppearsInPersistentMap) {
    auto transient = immer::map<uint32_t, framed_expr>{}.transient();
    test_hbm_t hbm(globalizer_, transient);
    hbm.bind(k_key_0, functor_fe_);
    const auto frozen = std::move(transient).persistent();
    const auto* found = frozen.find(k_key_0);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, MultipleBindsAllAppearInPersistentMap) {
    auto transient = immer::map<uint32_t, framed_expr>{}.transient();
    test_hbm_t hbm(globalizer_, transient);
    hbm.bind(k_key_0, functor_fe_);
    hbm.bind(k_key_1, var0_fe_);
    const auto frozen = std::move(transient).persistent();
    EXPECT_EQ(frozen.size(), 2u);
    ASSERT_NE(frozen.find(k_key_0), nullptr);
    ASSERT_NE(frozen.find(k_key_1), nullptr);
    EXPECT_EQ(*frozen.find(k_key_0), functor_fe_);
    EXPECT_EQ(*frozen.find(k_key_1), var0_fe_);
}

TEST_F(HierarchicalBindMapTest, BaseBindingsInheritedInPersistentMap) {
    const immer::map<uint32_t, framed_expr> base =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    hbm.bind(k_key_1, var0_fe_);
    const auto frozen = std::move(transient).persistent();
    EXPECT_EQ(frozen.size(), 2u);
    ASSERT_NE(frozen.find(k_key_0), nullptr);
    EXPECT_EQ(*frozen.find(k_key_0), functor_fe_);
    ASSERT_NE(frozen.find(k_key_1), nullptr);
    EXPECT_EQ(*frozen.find(k_key_1), var0_fe_);
}

TEST_F(HierarchicalBindMapTest, BindThrowsIfVariableAlreadyBound) {
    const immer::map<uint32_t, framed_expr> base =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    EXPECT_THROW(hbm.bind(k_key_0, functor_fe_), std::exception);
}

TEST_F(HierarchicalBindMapTest, WhnfOnFunctorReturnsFunctorUnchanged) {
    EXPECT_CALL(globalizer_, globalize(::testing::_, ::testing::_)).Times(0);
    auto transient = immer::map<uint32_t, framed_expr>{}.transient();
    test_hbm_t hbm(globalizer_, transient);
    const framed_expr result = hbm.whnf(functor_fe_);
    EXPECT_EQ(result, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfOnUnboundVarReturnsOriginalFe) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    auto transient = immer::map<uint32_t, framed_expr>{}.transient();
    test_hbm_t hbm(globalizer_, transient);
    const framed_expr result = hbm.whnf(var0_fe_);
    EXPECT_EQ(result, var0_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfSingleHopVarToFunctorReturnsFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    const immer::map<uint32_t, framed_expr> base =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    const framed_expr result = hbm.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfSingleHopPathCompressesInPersistentMap) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    const immer::map<uint32_t, framed_expr> base =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    hbm.whnf(var0_fe_);
    const auto frozen = std::move(transient).persistent();
    ASSERT_NE(frozen.find(k_key_0), nullptr);
    EXPECT_EQ(*frozen.find(k_key_0), functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfMultiHopFollowsChainToFunctor) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    const immer::map<uint32_t, framed_expr> base = immer::map<uint32_t, framed_expr>{}
        .set(k_key_0, var1_fe_)
        .set(k_key_1, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    const framed_expr result = hbm.whnf(var0_fe_);
    EXPECT_EQ(result, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfMultiHopCompressesBothHopsInPersistentMap) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    const immer::map<uint32_t, framed_expr> base = immer::map<uint32_t, framed_expr>{}
        .set(k_key_0, var1_fe_)
        .set(k_key_1, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    hbm.whnf(var0_fe_);
    const auto frozen = std::move(transient).persistent();
    EXPECT_EQ(*frozen.find(k_key_0), functor_fe_);
    EXPECT_EQ(*frozen.find(k_key_1), functor_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfChainEndingInUnboundVarReturnsLastVar) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u)).WillOnce(::testing::Return(k_key_1));
    const immer::map<uint32_t, framed_expr> base =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, var1_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    const framed_expr result = hbm.whnf(var0_fe_);
    EXPECT_EQ(result, var1_fe_);
}

TEST_F(HierarchicalBindMapTest, WhnfCompressionEliminatesChainOnSecondCall) {
    EXPECT_CALL(globalizer_, globalize(5u, 0u))
        .WillOnce(::testing::Return(k_key_0))
        .WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(globalizer_, globalize(7u, 1u))
        .Times(1)
        .WillOnce(::testing::Return(k_key_1));
    const immer::map<uint32_t, framed_expr> base = immer::map<uint32_t, framed_expr>{}
        .set(k_key_0, var1_fe_)
        .set(k_key_1, functor_fe_);
    auto transient = base.transient();
    test_hbm_t hbm(globalizer_, transient);
    const framed_expr first  = hbm.whnf(var0_fe_);
    const framed_expr second = hbm.whnf(var0_fe_);
    EXPECT_EQ(first,  functor_fe_);
    EXPECT_EQ(second, functor_fe_);
}

TEST_F(HierarchicalBindMapTest, TwoMapsWithDifferentBasesQueryIndependently) {
    const framed_expr result_a = make_functor_fe(77);
    const framed_expr result_b = make_functor_fe(88);

    const immer::map<uint32_t, framed_expr> base_a =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, result_a);
    const immer::map<uint32_t, framed_expr> base_b =
        immer::map<uint32_t, framed_expr>{}.set(k_key_0, result_b);

    ::testing::NiceMock<MockGlobalize> glob_a, glob_b;
    EXPECT_CALL(glob_a, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));
    EXPECT_CALL(glob_b, globalize(5u, 0u)).WillOnce(::testing::Return(k_key_0));

    auto transient_a = base_a.transient();
    auto transient_b = base_b.transient();
    test_hbm_t hbm_a(glob_a, transient_a);
    test_hbm_t hbm_b(glob_b, transient_b);

    EXPECT_EQ(hbm_a.whnf(var0_fe_), result_a);
    EXPECT_EQ(hbm_b.whnf(var0_fe_), result_b);
}
