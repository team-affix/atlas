#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_cycle_detector.hpp"
#include "value_objects/expr.hpp"
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

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_cycle_detector_t = pud_cycle_detector<NiceMock<MockGlobalize>, NiceMock<MockFind>>;

struct PudCycleDetectorTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockFind> find;
    test_cycle_detector_t detector{globalize, find};

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
    }

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }

    void bind(uint32_t root, const expr* value, uint32_t frame_offset) {
        ON_CALL(find, find(root)).WillByDefault(
            Return(ubs_class{.root = root, .size = 1, .rep = root, .value = framed_expr{value, frame_offset}}));
    }
};

}

TEST_F(PudCycleDetectorTest, NoVarsHaveNoCycle) {
    EXPECT_FALSE(detector.detect_cycle({}));
}

TEST_F(PudCycleDetectorTest, AnUnboundClassHasNoCycle) {
    EXPECT_FALSE(detector.detect_cycle({4}));
}

TEST_F(PudCycleDetectorTest, AClassBoundToATermWithoutItselfHasNoCycle) {
    bind(0, functor(1, {var(1), functor(2, {})}), 0);
    EXPECT_FALSE(detector.detect_cycle({0}));
}

TEST_F(PudCycleDetectorTest, AClassBoundToATermContainingItselfIsACycle) {
    bind(0, functor(1, {var(0)}), 0);
    EXPECT_TRUE(detector.detect_cycle({0}));
}

TEST_F(PudCycleDetectorTest, AValueIsReadInItsOwnFrame) {
    bind(0, functor(1, {var(0)}), 5);
    EXPECT_FALSE(detector.detect_cycle({0}));
    bind(5, functor(1, {var(0)}), 5);
    EXPECT_TRUE(detector.detect_cycle({0}));
}

TEST_F(PudCycleDetectorTest, ACycleThroughTwoClassesIsFound) {
    bind(0, functor(1, {var(1)}), 0);
    bind(1, functor(2, {var(0)}), 0);
    EXPECT_TRUE(detector.detect_cycle({1}));
}

TEST_F(PudCycleDetectorTest, TwoRoutesToOneClassAreNoCycle) {
    bind(0, functor(1, {var(1), var(1)}), 0);
    bind(1, functor(2, {}), 0);
    EXPECT_FALSE(detector.detect_cycle({0}));
}

TEST_F(PudCycleDetectorTest, ASharedSubtermIsSearchedOnce) {
    const expr* shared = functor(2, {var(1)});
    bind(0, functor(1, {shared, shared}), 0);
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(1)).Times(1);
    EXPECT_FALSE(detector.detect_cycle({0}));
}

TEST_F(PudCycleDetectorTest, ASharedSubtermReachedFromInsideItselfIsACycle) {
    const expr* shared = functor(2, {var(1)});
    bind(0, functor(1, {shared}), 0);
    bind(1, shared, 0);
    EXPECT_TRUE(detector.detect_cycle({0}));
}

TEST_F(PudCycleDetectorTest, AClassSearchedFromAnEarlierVarIsNotSearchedAgain) {
    bind(0, functor(1, {var(1)}), 0);
    bind(1, functor(2, {var(2)}), 0);
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(2)).Times(1);
    EXPECT_FALSE(detector.detect_cycle({0, 1}));
}
