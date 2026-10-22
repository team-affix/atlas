#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_normalizer.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
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
struct MockFindRep {
    MOCK_METHOD(uint32_t, find_rep, (uint32_t));
};
struct MockFindValue {
    MOCK_METHOD(std::optional<framed_expr>, find_value, (uint32_t));
};

using test_normalizer_t = global_normalizer<
    NiceMock<MockGlobalize>, NiceMock<MockMakeFunctor>, NiceMock<MockMakeVar>,
    NiceMock<MockFindRep>, NiceMock<MockFindValue>>;

struct PudNormalizerTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockMakeFunctor> make_functor;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockFindRep> find_rep;
    NiceMock<MockFindValue> find_value;
    test_normalizer_t normalizer{globalize, make_functor, make_var, find_rep, find_value};

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }

    void unbound(uint32_t global, uint32_t rep, const expr* rep_node) {
        ON_CALL(find_value, find_value(global)).WillByDefault(Return(std::nullopt));
        ON_CALL(find_rep, find_rep(global)).WillByDefault(Return(rep));
        ON_CALL(make_var, make_var(rep)).WillByDefault(Return(rep_node));
    }

    void bound(uint32_t global, framed_expr value) {
        ON_CALL(find_value, find_value(global)).WillByDefault(Return(value));
    }

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault([](uint32_t offset, uint32_t index) { return offset + index; });
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

TEST_F(PudNormalizerTest, LaterBindingIsVisibleOnTheNextNormalize) {
    const expr* unbound_node = var(0);
    const expr* canonical = functor(3, {});
    EXPECT_CALL(find_value, find_value(0u))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(framed_expr{functor(3, {}), 0}));
    ON_CALL(find_rep, find_rep(0u)).WillByDefault(Return(0u));
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
