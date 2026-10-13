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
    MOCK_METHOD(const expr*, make, (uint32_t id, (const std::vector<const expr*>& args)));
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
    EXPECT_CALL(make_functor, make(9u, testing::_)).WillOnce(testing::Return(canonical));
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
    EXPECT_CALL(make_functor, make(11u, testing::_)).Times(2).WillRepeatedly(testing::Return(canonical));
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
    EXPECT_CALL(make_functor, make(3u, testing::_)).WillOnce(testing::Return(canonical));
    EXPECT_EQ(normalizer.normalize({local, 0u}), unbound);
    EXPECT_EQ(normalizer.normalize({local, 0u}), canonical);
}
