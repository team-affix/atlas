#include <deque>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_normalizer.hpp"
#include "value_objects/expr.hpp"

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t frame_offset, uint32_t local_index), ());
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t id, (const std::vector<const expr*>& args)));
};

struct MockResolve {
    MOCK_METHOD(const expr*, resolve, (uint32_t));
};

using test_normalizer_t = global_normalizer<MockGlobalize, MockMakeFunctor, MockResolve>;

struct expr_fixture {
    std::deque<expr> owned;

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, const std::vector<const expr*>& args) {
        owned.push_back(expr{expr::functor{id, args}});
        return &owned.back();
    }
};

} // namespace

struct PudNormalizerTest : public ::testing::Test {
    expr_fixture terms;
    ::testing::NiceMock<MockGlobalize> globalize;
    ::testing::NiceMock<MockMakeFunctor> make_functor;
    ::testing::NiceMock<MockResolve> resolve;
    test_normalizer_t normalizer{globalize, make_functor, resolve};
};

TEST_F(PudNormalizerTest, FreshVarNormalizesToGlobalVarNode) {
    const expr* local = terms.var(2u);
    const expr* global = terms.var(7u);
    EXPECT_CALL(globalize, globalize(5u, 2u)).WillOnce(testing::Return(7u));
    EXPECT_CALL(resolve, resolve(7u)).WillOnce(testing::Return(global));
    EXPECT_EQ(normalizer.normalize({local, 5u}), global);
}

TEST_F(PudNormalizerTest, BoundVarNormalizesToStoredFunctorPointer) {
    const expr* bound = terms.functor(9u, {});
    const expr* canonical = terms.functor(9u, {});
    const expr* local = terms.var(1u);
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(bound));
    EXPECT_CALL(make_functor, make_functor(9u, testing::_)).WillOnce(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({local, 0u}), canonical);
}

TEST_F(PudNormalizerTest, UnboundMergedClassNormalizesToRepresentative) {
    const expr* local = terms.var(4u);
    const expr* rep = terms.var(2u);
    EXPECT_CALL(globalize, globalize(2u, 4u)).WillOnce(testing::Return(6u));
    EXPECT_CALL(resolve, resolve(6u)).WillOnce(testing::Return(rep));
    EXPECT_EQ(normalizer.normalize({local, 2u}), rep);
}

TEST_F(PudNormalizerTest, UnboundVarKeepsGlobalIndexWithoutRename) {
    const expr* local = terms.var(3u);
    const expr* global = terms.var(8u);
    EXPECT_CALL(globalize, globalize(5u, 3u)).WillOnce(testing::Return(8u));
    EXPECT_CALL(resolve, resolve(8u)).WillOnce(testing::Return(global));
    EXPECT_EQ(normalizer.normalize({local, 5u}), global);
}

TEST_F(PudNormalizerTest, SameLocalUnderDifferentOffsetsAreDifferent) {
    const expr* local = terms.var(0u);
    const expr* g0 = terms.var(0u);
    const expr* g1 = terms.var(1u);
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(1u, 0u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(g0));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(g1));
    EXPECT_NE(normalizer.normalize({local, 0u}), normalizer.normalize({local, 1u}));
}

TEST_F(PudNormalizerTest, GroundFunctorRebuildsViaMakeFunctor) {
    const expr* skeleton = terms.functor(11u, {});
    const expr* canonical = terms.functor(11u, {});
    EXPECT_CALL(make_functor, make_functor(11u, testing::_)).Times(2).WillRepeatedly(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({skeleton, 0u}), canonical);
    EXPECT_EQ(normalizer.normalize({skeleton, 5u}), canonical);
}

TEST_F(PudNormalizerTest, NormalizingTwiceYieldsSamePointer) {
    const expr* local = terms.var(1u);
    const expr* global = terms.var(1u);
    EXPECT_CALL(globalize, globalize(0u, 1u)).Times(2).WillRepeatedly(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(1u)).Times(2).WillRepeatedly(testing::Return(global));
    EXPECT_EQ(normalizer.normalize({local, 0u}), normalizer.normalize({local, 0u}));
}

TEST_F(PudNormalizerTest, LaterBindingIsVisibleOnNextNormalize) {
    const expr* local = terms.var(0u);
    const expr* unbound = terms.var(0u);
    const expr* bound = terms.functor(3u, {});
    const expr* canonical = terms.functor(3u, {});
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(2).WillRepeatedly(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(unbound)).WillOnce(testing::Return(bound));
    EXPECT_CALL(make_functor, make_functor(3u, testing::_)).WillOnce(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({local, 0u}), unbound);
    EXPECT_EQ(normalizer.normalize({local, 0u}), canonical);
}

TEST_F(PudNormalizerTest, ZeroArityFunctorNormalizesToItself) {
    const expr* constant = terms.functor(0u, {});
    const expr* canonical = terms.functor(0u, {});
    EXPECT_CALL(make_functor, make_functor(0u, testing::_)).WillOnce(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({constant, 0u}), canonical);
}

TEST_F(PudNormalizerTest, AllBoundArgsBecomeFullyGroundFunctor) {
    const expr* a = terms.functor(1u, {});
    const expr* b = terms.functor(2u, {});
    const expr* x = terms.var(0u);
    const expr* y = terms.var(1u);
    const expr* skeleton = terms.functor(9u, {x, y});
    const expr* ground = terms.functor(9u, {a, b});
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(a));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(b));
    EXPECT_CALL(make_functor, make_functor(1u, testing::_)).WillOnce(testing::Return(a));
    EXPECT_CALL(make_functor, make_functor(2u, testing::_)).WillOnce(testing::Return(b));
    EXPECT_CALL(make_functor, make_functor(9u, testing::_)).WillOnce(testing::Return(ground));
    EXPECT_EQ(normalizer.normalize({skeleton, 0u}), ground);
}

TEST_F(PudNormalizerTest, MixedBoundAndUnboundKeepsRepresentatives) {
    const expr* a = terms.functor(1u, {});
    const expr* local_x = terms.var(0u);
    const expr* local_y = terms.var(1u);
    const expr* rep_y = terms.var(5u);
    const expr* skeleton = terms.functor(9u, {local_x, local_y});
    const expr* rebuilt = terms.functor(9u, {a, rep_y});
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(5u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(a));
    EXPECT_CALL(resolve, resolve(5u)).WillOnce(testing::Return(rep_y));
    EXPECT_CALL(make_functor, make_functor(1u, testing::_)).WillOnce(testing::Return(a));
    EXPECT_CALL(make_functor, make_functor(9u, testing::_)).WillOnce(testing::Return(rebuilt));
    EXPECT_EQ(normalizer.normalize({skeleton, 0u}), rebuilt);
}

TEST_F(PudNormalizerTest, BoundFunctorContainingBoundVarNormalizesTwoLevels) {
    const expr* inner_bound = terms.functor(2u, {});
    const expr* outer_bound_skeleton = terms.functor(3u, {terms.var(1u)});
    const expr* local = terms.var(0u);
    const expr* rebuilt = terms.functor(3u, {inner_bound});
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(outer_bound_skeleton));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(inner_bound));
    EXPECT_CALL(make_functor, make_functor(2u, testing::_)).WillOnce(testing::Return(inner_bound));
    EXPECT_CALL(make_functor, make_functor(3u, testing::_)).WillOnce(testing::Return(rebuilt));
    EXPECT_EQ(normalizer.normalize({local, 0u}), rebuilt);
}

TEST_F(PudNormalizerTest, FiveVariableBindingChainNormalizesFully) {
    const expr* ground = terms.functor(99u, {});
    const expr* v4 = terms.var(4u);
    const expr* f3 = terms.functor(10u, {v4});
    const expr* v3 = terms.var(3u);
    const expr* f2 = terms.functor(10u, {v3});
    const expr* v2 = terms.var(2u);
    const expr* f1 = terms.functor(10u, {v2});
    const expr* v1 = terms.var(1u);
    const expr* f0 = terms.functor(10u, {v1});
    const expr* v0 = terms.var(0u);

    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(f0));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(f1));
    EXPECT_CALL(globalize, globalize(0u, 2u)).WillOnce(testing::Return(2u));
    EXPECT_CALL(resolve, resolve(2u)).WillOnce(testing::Return(f2));
    EXPECT_CALL(globalize, globalize(0u, 3u)).WillOnce(testing::Return(3u));
    EXPECT_CALL(resolve, resolve(3u)).WillOnce(testing::Return(f3));
    EXPECT_CALL(globalize, globalize(0u, 4u)).WillOnce(testing::Return(4u));
    EXPECT_CALL(resolve, resolve(4u)).WillOnce(testing::Return(ground));
    EXPECT_CALL(make_functor, make_functor(testing::_, testing::_))
        .WillRepeatedly(testing::Return(ground));
    EXPECT_EQ(normalizer.normalize({v0, 0u}), ground);
}

TEST_F(PudNormalizerTest, RepeatedVariablePositionNormalizesToSameNode) {
    const expr* local = terms.var(0u);
    const expr* rep = terms.var(0u);
    const expr* skeleton = terms.functor(4u, {local, local});
    const expr* canonical = terms.functor(4u, {rep, rep});
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(2).WillRepeatedly(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).Times(2).WillRepeatedly(testing::Return(rep));
    EXPECT_CALL(make_functor, make_functor(4u, testing::_)).WillOnce(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({skeleton, 0u}), canonical);
}

TEST_F(PudNormalizerTest, SameClassDifferentLocalsShareRepresentative) {
    const expr* local_a = terms.var(0u);
    const expr* local_b = terms.var(1u);
    const expr* rep = terms.var(2u);
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(3u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(4u));
    EXPECT_CALL(resolve, resolve(3u)).WillOnce(testing::Return(rep));
    EXPECT_CALL(resolve, resolve(4u)).WillOnce(testing::Return(rep));
    EXPECT_EQ(normalizer.normalize({local_a, 0u}), rep);
    EXPECT_EQ(normalizer.normalize({local_b, 0u}), rep);
}

TEST_F(PudNormalizerTest, StoredGroundSkeletonSamePointerAcrossOffsets) {
    const expr* skeleton = terms.functor(8u, {});
    const expr* canonical = terms.functor(8u, {});
    EXPECT_CALL(make_functor, make_functor(8u, testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({skeleton, 0u}), canonical);
    EXPECT_EQ(normalizer.normalize({skeleton, 9u}), canonical);
}

TEST_F(PudNormalizerTest, NonGroundSkeletonDiffersAcrossOffsets) {
    const expr* local = terms.var(0u);
    const expr* skeleton = terms.functor(8u, {local});
    const expr* g0 = terms.var(10u);
    const expr* g9 = terms.var(19u);
    const expr* out0 = terms.functor(8u, {g0});
    const expr* out9 = terms.functor(8u, {g9});
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(10u));
    EXPECT_CALL(globalize, globalize(9u, 0u)).WillOnce(testing::Return(19u));
    EXPECT_CALL(resolve, resolve(10u)).WillOnce(testing::Return(g0));
    EXPECT_CALL(resolve, resolve(19u)).WillOnce(testing::Return(g9));
    EXPECT_CALL(make_functor, make_functor(8u, testing::_))
        .WillOnce(testing::Return(out0))
        .WillOnce(testing::Return(out9));
    EXPECT_NE(normalizer.normalize({skeleton, 0u}), normalizer.normalize({skeleton, 9u}));
}

TEST_F(PudNormalizerTest, StoredValueAtOffsetZeroStableWhenBindingsUnchanged) {
    const expr* skeleton = terms.functor(5u, {});
    const expr* canonical = terms.functor(5u, {});
    EXPECT_CALL(make_functor, make_functor(5u, testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return(canonical));
    const expr* first = normalizer.normalize({skeleton, 0u});
    const expr* second = normalizer.normalize({skeleton, 0u});
    EXPECT_EQ(first, second);
}
