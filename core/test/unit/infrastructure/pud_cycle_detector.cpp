#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_cycle_detector.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_functor.hpp"
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

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_cycle_detector_t = pud_cycle_detector<NiceMock<MockGlobalize>, NiceMock<MockFind>, NiceMock<MockMakeFunctor>>;

struct PudCycleDetectorTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockFind> find;
    NiceMock<MockMakeFunctor> make_functor;
    test_cycle_detector_t detector{globalize, find, make_functor};

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(make_functor, make_functor(_, _)).WillByDefault([this](uint32_t id, const std::vector<const expr*>& args) {
            return functor(id, args);
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
    test_cycle_detector_t::visitation visitation;
    bool found_cycle = false;
    for (uint32_t class_root : std::vector<uint32_t>{}) {
        if (detector.find_cycle_from_class(class_root, visitation)) {
            found_cycle = true;
            break;
        }
    }
    EXPECT_FALSE(found_cycle);
}

TEST_F(PudCycleDetectorTest, AnUnboundClassHasNoCycle) {
    test_cycle_detector_t::visitation visitation;
    EXPECT_FALSE(detector.find_cycle_from_class(4, visitation));
}

TEST_F(PudCycleDetectorTest, AClassBoundToATermWithoutItselfHasNoCycle) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{
            .root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(1), functor(2, {})}))->content), 0}}}));
    test_cycle_detector_t::visitation visitation;
    EXPECT_FALSE(detector.find_cycle_from_class(0, visitation));
}

TEST_F(PudCycleDetectorTest, AClassBoundToATermContainingItselfIsACycle) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(0)}))->content), 0}}}));
    test_cycle_detector_t::visitation visitation;
    EXPECT_TRUE(detector.find_cycle_from_class(0, visitation));
}

TEST_F(PudCycleDetectorTest, AValueIsReadInItsOwnFrame) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(0)}))->content), 5}}}));
    test_cycle_detector_t::visitation visitation_first;
    EXPECT_FALSE(detector.find_cycle_from_class(0, visitation_first));
    ON_CALL(find, find(5)).WillByDefault(
        Return(ubs_class{.root = 5, .size = 1, .rep = 5, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(0)}))->content), 5}}}));
    test_cycle_detector_t::visitation visitation_second;
    EXPECT_TRUE(detector.find_cycle_from_class(0, visitation_second));
}

TEST_F(PudCycleDetectorTest, ACycleThroughTwoClassesIsFound) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(1)}))->content), 0}}}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = framed_functor{std::get_if<expr::functor>(&(functor(2, {var(0)}))->content), 0}}}));
    test_cycle_detector_t::visitation visitation;
    EXPECT_TRUE(detector.find_cycle_from_class(1, visitation));
}

TEST_F(PudCycleDetectorTest, TwoRoutesToOneClassAreNoCycle) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(1), var(1)}))->content), 0}}}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = framed_functor{std::get_if<expr::functor>(&(functor(2, {}))->content), 0}}}));
    test_cycle_detector_t::visitation visitation;
    EXPECT_FALSE(detector.find_cycle_from_class(0, visitation));
}

TEST_F(PudCycleDetectorTest, ASharedSubtermIsSearchedOnce) {
    const expr* shared = functor(2, {var(1)});
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {shared, shared}))->content), 0}}}));
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(1)).Times(1);
    test_cycle_detector_t::visitation visitation;
    EXPECT_FALSE(detector.find_cycle_from_class(0, visitation));
}

TEST_F(PudCycleDetectorTest, ASharedSubtermReachedFromInsideItselfIsACycle) {
    const expr* shared = functor(2, {var(1)});
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {shared}))->content), 0}}}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = framed_functor{std::get_if<expr::functor>(&(shared)->content), 0}}}));
    test_cycle_detector_t::visitation visitation;
    EXPECT_TRUE(detector.find_cycle_from_class(0, visitation));
}

TEST_F(PudCycleDetectorTest, AClassSearchedFromAnEarlierRootIsNotSearchedAgain) {
    ON_CALL(find, find(0)).WillByDefault(
        Return(ubs_class{.root = 0, .size = 1, .rep = 0, .value = framed_functor{std::get_if<expr::functor>(&(functor(1, {var(1)}))->content), 0}}}));
    ON_CALL(find, find(1)).WillByDefault(
        Return(ubs_class{.root = 1, .size = 1, .rep = 1, .value = framed_functor{std::get_if<expr::functor>(&(functor(2, {var(2)}))->content), 0}}}));
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(2)).Times(1);
    test_cycle_detector_t::visitation visitation;
    EXPECT_FALSE(detector.find_cycle_from_class(0, visitation));
    EXPECT_FALSE(detector.find_cycle_from_class(1, visitation));
}
