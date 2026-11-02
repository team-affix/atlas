#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_cycle_detector.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "pud_moduled_functor_helper.hpp"
#include "value_objects/ubs_class.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};
struct MockFind {
    MOCK_METHOD(ubs_class, find, (uint32_t));
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};

struct MockDemoduleFunctor {
    MOCK_METHOD(const expr*, demodule_functor, (moduled_functor));
};

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_cycle_detector_t = pud_cycle_detector<NiceMock<MockGlobalize>, NiceMock<MockFind>, NiceMock<MockDemoduleFunctor>>;

struct PudCycleDetectorTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockFind> find;
    NiceMock<MockDemoduleFunctor> demodule_functor;
    test_cycle_detector_t detector{globalize, find, demodule_functor};

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(demodule_functor, demodule_functor(_)).WillByDefault([this](moduled_functor bound) {
            return functor(bound.functor->id, bound.functor->args);
        });
    }

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }
};

}

TEST_F(PudCycleDetectorTest, NoEntryRootsHaveNoCycle) {
    EXPECT_FALSE(detector.find_cycle_from_classes({}));
}

TEST_F(PudCycleDetectorTest, AnUnboundClassHasNoCycle) {
    EXPECT_FALSE(detector.find_cycle_from_classes({4}));
}

TEST_F(PudCycleDetectorTest, AClassBoundToATermWithoutItselfHasNoCycle) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{
            .root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {var(1), functor(2, {})}), 0)}));
    EXPECT_FALSE(detector.find_cycle_from_classes({0}));
}

TEST_F(PudCycleDetectorTest, AClassBoundToATermContainingItselfIsACycle) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {var(0)}), 0)}));
    EXPECT_TRUE(detector.find_cycle_from_classes({0}));
}

TEST_F(PudCycleDetectorTest, AValueIsReadInItsOwnFrame) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {var(0)}), 5)}));
    EXPECT_FALSE(detector.find_cycle_from_classes({0}));
    ON_CALL(find, find(5)).WillByDefault(
        Return(ubs_class{.root = 5, .size = 1, .rep = 5, .value = pud_test_moduled_functor(functor(1, {var(0)}), 5)}));
    EXPECT_TRUE(detector.find_cycle_from_classes({0}));
}

TEST_F(PudCycleDetectorTest, ACycleThroughTwoClassesIsFound) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {var(1)}), 0)}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = pud_test_moduled_functor(functor(2, {var(0)}), 0)}));
    EXPECT_TRUE(detector.find_cycle_from_classes({1}));
}

TEST_F(PudCycleDetectorTest, TwoRoutesToOneClassAreNoCycle) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {var(1), var(1)}), 0)}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = pud_test_moduled_functor(functor(2, {}), 0)}));
    EXPECT_FALSE(detector.find_cycle_from_classes({0}));
}

TEST_F(PudCycleDetectorTest, ASharedSubtermIsSearchedOnce) {
    const expr* shared = functor(2, {var(1)});
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {shared, shared}), 0)}));
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(1)).Times(1);
    EXPECT_FALSE(detector.find_cycle_from_classes({0}));
}

TEST_F(PudCycleDetectorTest, ASharedSubtermReachedFromInsideItselfIsACycle) {
    const expr* shared = functor(2, {var(1)});
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {shared}), 0)}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = pud_test_moduled_functor(shared, 0)}));
    EXPECT_TRUE(detector.find_cycle_from_classes({0}));
}

TEST_F(PudCycleDetectorTest, AClassSearchedFromAnEarlierRootIsNotSearchedAgain) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = pud_test_moduled_functor(functor(1, {var(1)}), 0)}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = pud_test_moduled_functor(functor(2, {var(2)}), 0)}));
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(2)).Times(1);
    EXPECT_FALSE(detector.find_cycle_from_classes({0, 1}));
}
