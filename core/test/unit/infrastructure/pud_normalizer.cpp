#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_normalizer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "pud_moduled_functor_helper.hpp"
#include "value_objects/ubs_class.hpp"

using ::testing::_;
using ::testing::InSequence;
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

struct MockDemoduleFunctor {
    MOCK_METHOD(const expr*, demodule, (moduled_functor));
};

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

using test_normalizer_t = pud_normalizer<
    NiceMock<MockGlobalize>, NiceMock<MockMakeFunctor>, NiceMock<MockMakeVar>, NiceMock<MockFind>,
    NiceMock<MockDemoduleFunctor>>;

struct PudNormalizerTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockMakeFunctor> make_functor;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockFind> find;
    NiceMock<MockDemoduleFunctor> demodule_functor;
    test_normalizer_t normalizer{globalize, make_functor, make_var, find, demodule_functor};

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(demodule_functor, demodule(_)).WillByDefault([this](moduled_functor bound) {
            return functor(bound.functor->id, bound.functor->args);
        });
    }
};

}

TEST_F(PudNormalizerTest, UnboundVarNormalizesToItsRepresentative) {
    const expr* rep = var(2);
    ON_CALL(find, find(7)).WillByDefault(Return(ubs_class{.root = 7, .size = 1, .rep = 2, .value = std::nullopt}));
    ON_CALL(make_var, make_var(2)).WillByDefault(Return(rep));
    EXPECT_EQ(normalizer.normalize({var(4), 3}), rep);
}

TEST_F(PudNormalizerTest, SameLocalUnderDifferentOffsetsIsDifferent) {
    ON_CALL(find, find(0)).WillByDefault(Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = std::nullopt}));
    ON_CALL(make_var, make_var(0)).WillByDefault(Return(var(0)));
    ON_CALL(find, find(1)).WillByDefault(Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = std::nullopt}));
    ON_CALL(make_var, make_var(1)).WillByDefault(Return(var(1)));
    EXPECT_NE(normalizer.normalize({var(0), 0}), normalizer.normalize({var(0), 1}));
}

TEST_F(PudNormalizerTest, BoundVarNormalizesThroughItsValue) {
    const expr* bound_shape = functor(9, {});
    const expr* canonical = functor(9, {});
    ON_CALL(find, find(1)).WillByDefault(Return(ubs_class{
        .root = 1, .size = 1, .rep = 1, .value = pud_test_moduled_functor(bound_shape, 0)}));
    EXPECT_CALL(demodule_functor, demodule(_)).WillOnce(Return(bound_shape));
    EXPECT_CALL(make_functor, make_functor(9u, _)).WillOnce(Return(canonical));
    EXPECT_EQ(normalizer.normalize({var(1), 0}), canonical);
}

TEST_F(PudNormalizerTest, ABoundValueIsReadInItsOwnFrame) {
    const expr* bound_shape = functor(3, {var(2)});
    const expr* rep = var(12);
    const expr* rebuilt = functor(3, {rep});
    ON_CALL(find, find(0)).WillByDefault(Return(ubs_class{
        .root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(bound_shape, 10)}));
    ON_CALL(find, find(12)).WillByDefault(Return(ubs_class{.root = 12, .size = 1, .rep = 12, .value = std::nullopt}));
    InSequence in_order;
    EXPECT_CALL(demodule_functor, demodule(_)).WillOnce(Return(bound_shape));
    ON_CALL(make_var, make_var(12)).WillByDefault(Return(rep));
    EXPECT_CALL(make_functor, make_functor(3u, std::vector<const expr*>{rep})).WillOnce(Return(rebuilt));
    EXPECT_EQ(normalizer.normalize({var(0), 0}), rebuilt);
}

TEST_F(PudNormalizerTest, ChainsOfBoundVarsNormalizeFully) {
    const expr* ground = functor(99, {});
    ON_CALL(find, find(0)).WillByDefault(Return(ubs_class{
        .root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(10, {var(1)}), 0)}));
    ON_CALL(find, find(1)).WillByDefault(Return(ubs_class{
        .root = 1, .size = 1, .rep = 1, .value = pud_test_moduled_functor(functor(10, {var(2)}), 0)}));
    ON_CALL(find, find(2)).WillByDefault(Return(ubs_class{
        .root = 2, .size = 1, .rep = 2, .value = pud_test_moduled_functor(functor(99, {}), 0)}));
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
    ON_CALL(find, find(0)).WillByDefault(Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(a, 0)}));
    ON_CALL(find, find(1)).WillByDefault(Return(ubs_class{.root = 1, .size = 1, .rep = 5, .value = std::nullopt}));
    ON_CALL(make_var, make_var(5)).WillByDefault(Return(rep_y));
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
        .WillOnce(Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(3, {}), 0)}));
    ON_CALL(make_var, make_var(0u)).WillByDefault(Return(unbound_node));
    ON_CALL(make_functor, make_functor(3u, _)).WillByDefault(Return(canonical));
    EXPECT_EQ(normalizer.normalize({var(0), 0}), unbound_node);
    EXPECT_EQ(normalizer.normalize({var(0), 0}), canonical);
}

TEST_F(PudNormalizerTest, SameClassDifferentLocalsShareRepresentative) {
    const expr* rep = var(2);
    ON_CALL(find, find(3)).WillByDefault(Return(ubs_class{.root = 3, .size = 1, .rep = 2, .value = std::nullopt}));
    ON_CALL(make_var, make_var(2)).WillByDefault(Return(rep));
    ON_CALL(find, find(4)).WillByDefault(Return(ubs_class{.root = 4, .size = 1, .rep = 2, .value = std::nullopt}));
    ON_CALL(make_var, make_var(2)).WillByDefault(Return(rep));
    EXPECT_EQ(normalizer.normalize({var(0), 3}), rep);
    EXPECT_EQ(normalizer.normalize({var(1), 3}), rep);
}
