#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_normalizer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/ubs_class.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};
struct MockFind {
    MOCK_METHOD(ubs_class, find, (uint32_t));
};

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

using test_normalizer_t = pud_normalizer<
    NiceMock<MockGlobalize>, NiceMock<MockMakeFunctor>, NiceMock<MockMakeVar>, NiceMock<MockFind>>;

struct PudNormalizerTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockMakeFunctor> make_functor;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockFind> find;
    test_normalizer_t normalizer{globalize, make_functor, make_var, find};

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }

    void unbound(uint32_t global, uint32_t rep, const expr* rep_node) {
        ON_CALL(find, find(global)).WillByDefault(Return(ubs_class{.root = global, .size = 1, .rep = rep, .value = std::nullopt}));
        ON_CALL(make_var, make_var(rep)).WillByDefault(Return(rep_node));
    }

    void bound(uint32_t global, framed_expr value) {
        ON_CALL(find, find(global)).WillByDefault(Return(ubs_class{.root = global, .size = 1, .rep = global, .value = value}));
    }

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
    }
};

}

TEST_F(PudNormalizerTest, UnboundVarNormalizesToItsRepresentative) {
    const expr* rep = var(2);
    unbound(7, 2, rep);
    EXPECT_EQ(normalizer.normalize({var(4), 3}), rep);
}

TEST_F(PudNormalizerTest, SameLocalUnderDifferentOffsetsIsDifferent) {
    unbound(0, 0, var(0));
    unbound(1, 1, var(1));
    EXPECT_NE(normalizer.normalize({var(0), 0}), normalizer.normalize({var(0), 1}));
}

TEST_F(PudNormalizerTest, BoundVarNormalizesThroughItsValue) {
    const expr* canonical = functor(9, {});
    bound(1, framed_expr{functor(9, {}), 0});
    EXPECT_CALL(make_functor, make_functor(9u, _)).WillOnce(Return(canonical));
    EXPECT_EQ(normalizer.normalize({var(1), 0}), canonical);
}

TEST_F(PudNormalizerTest, ABoundValueIsReadInItsOwnFrame) {
    const expr* rep = var(12);
    const expr* rebuilt = functor(3, {rep});
    bound(0, framed_expr{functor(3, {var(2)}), 10});
    unbound(12, 12, rep);
    EXPECT_CALL(make_functor, make_functor(3u, std::vector<const expr*>{rep})).WillOnce(Return(rebuilt));
    EXPECT_EQ(normalizer.normalize({var(0), 0}), rebuilt);
}

TEST_F(PudNormalizerTest, ChainsOfBoundVarsNormalizeFully) {
    const expr* ground = functor(99, {});
    bound(0, framed_expr{functor(10, {var(1)}), 0});
    bound(1, framed_expr{functor(10, {var(2)}), 0});
    bound(2, framed_expr{functor(99, {}), 0});
    ON_CALL(make_functor, make_functor(_, _)).WillByDefault(Return(ground));
    EXPECT_EQ(normalizer.normalize({var(0), 0}), ground);
}

TEST_F(PudNormalizerTest, GroundFunctorRebuildsViaMakeFunctor) {
    const expr* canonical = functor(11, {});
    EXPECT_CALL(make_functor, make_functor(11u, _)).Times(2).WillRepeatedly(Return(canonical));
    const expr* skeleton = functor(11, {});
    EXPECT_EQ(normalizer.normalize({skeleton, 0}), canonical);
    EXPECT_EQ(normalizer.normalize({skeleton, 5}), canonical);
}

TEST_F(PudNormalizerTest, MixedBoundAndUnboundArgsKeepRepresentatives) {
    const expr* a = functor(1, {});
    const expr* rep_y = var(5);
    const expr* rebuilt = functor(9, {a, rep_y});
    bound(0, framed_expr{a, 0});
    unbound(1, 5, rep_y);
    EXPECT_CALL(make_functor, make_functor(1u, _)).WillRepeatedly(Return(a));
    EXPECT_CALL(make_functor, make_functor(9u, std::vector<const expr*>{a, rep_y})).WillOnce(Return(rebuilt));
    EXPECT_EQ(normalizer.normalize({functor(9, {var(0), var(1)}), 0}), rebuilt);
}

TEST_F(PudNormalizerTest, ASharedSubtermIsRebuiltOnce) {
    const expr* shared = functor(2, {});
    const expr* rebuilt_shared = functor(2, {});
    const expr* rebuilt = functor(1, {rebuilt_shared, rebuilt_shared});
    EXPECT_CALL(make_functor, make_functor(2u, _)).WillOnce(Return(rebuilt_shared));
    EXPECT_CALL(make_functor, make_functor(1u, std::vector<const expr*>{rebuilt_shared, rebuilt_shared}))
        .WillOnce(Return(rebuilt));
    EXPECT_EQ(normalizer.normalize({functor(1, {shared, shared}), 0}), rebuilt);
}

TEST_F(PudNormalizerTest, LaterBindingIsVisibleOnTheNextNormalize) {
    const expr* unbound_node = var(0);
    const expr* canonical = functor(3, {});
    EXPECT_CALL(find, find(0u))
        .WillOnce(Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = std::nullopt}))
        .WillOnce(Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_expr{functor(3, {}), 0}}));
    ON_CALL(make_var, make_var(0u)).WillByDefault(Return(unbound_node));
    ON_CALL(make_functor, make_functor(3u, _)).WillByDefault(Return(canonical));
    EXPECT_EQ(normalizer.normalize({var(0), 0}), unbound_node);
    EXPECT_EQ(normalizer.normalize({var(0), 0}), canonical);
}

TEST_F(PudNormalizerTest, SameClassDifferentLocalsShareRepresentative) {
    const expr* rep = var(2);
    unbound(3, 2, rep);
    unbound(4, 2, rep);
    EXPECT_EQ(normalizer.normalize({var(0), 3}), rep);
    EXPECT_EQ(normalizer.normalize({var(1), 3}), rep);
}
