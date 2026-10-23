#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unifier.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/ubs_class.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};
struct MockFind {
    MOCK_METHOD(ubs_class, find, (uint32_t));
};
struct MockEquate {
    MOCK_METHOD(std::optional<framed_expr>, equate, (const ubs_class&, const ubs_class&));
};
struct MockInstantiate {
    MOCK_METHOD(void, instantiate, (const ubs_class&, framed_expr));
};
struct MockDetectCycle {
    struct visitation {};
    MOCK_METHOD(bool, find_cycle_from_class, (uint32_t, visitation&));
};

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

ubs_class bound(uint32_t root, framed_expr value) {
    return ubs_class{.root = root, .size = 1, .rep = root, .value = value};
}

using test_unifier_t = pud_unifier<
    NiceMock<MockGlobalize>, NiceMock<MockFind>, NiceMock<MockEquate>,
    NiceMock<MockInstantiate>, NiceMock<MockDetectCycle>>;

struct PudUnifierTest : public ::testing::Test {
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockFind> find;
    NiceMock<MockEquate> equate;
    NiceMock<MockInstantiate> instantiate;
    NiceMock<MockDetectCycle> detect_cycle;
    test_unifier_t unifier{globalize, find, equate, instantiate, detect_cycle};
    std::deque<expr> owned;

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(equate, equate(_, _)).WillByDefault(Return(std::nullopt));
        ON_CALL(detect_cycle, find_cycle_from_class(_, _)).WillByDefault(Return(false));
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

TEST_F(PudUnifierTest, EqualGroundTermsSucceedWithoutBinding) {
    EXPECT_CALL(equate, equate(_, _)).Times(0);
    EXPECT_CALL(instantiate, instantiate(_, _)).Times(0);
    EXPECT_CALL(detect_cycle, find_cycle_from_class(_, _)).Times(0);
    const expr* term = functor(1, {functor(2, {})});
    EXPECT_TRUE(unifier.unify({term, 0}, {term, 5}));
}

TEST_F(PudUnifierTest, IdenticalFramedTermsAreNotLookedInto) {
    EXPECT_CALL(find, find(_)).Times(0);
    const expr* term = functor(1, {var(0), var(1)});
    EXPECT_TRUE(unifier.unify({term, 3}, {term, 3}));
}

TEST_F(PudUnifierTest, VarsInTheSameClassAreNotEquated) {
    ON_CALL(find, find(7)).WillByDefault(Return(ubs_class{.root = 3, .size = 2, .rep = 3, .value = std::nullopt}));
    EXPECT_CALL(equate, equate(_, _)).Times(0);
    EXPECT_TRUE(unifier.unify({var(3), 0}, {var(2), 5}));
}

TEST_F(PudUnifierTest, TwoUnboundVarsEquateTheirClasses) {
    EXPECT_CALL(equate, equate(singleton(4), singleton(9)));
    EXPECT_TRUE(unifier.unify({var(4), 0}, {var(6), 3}));
}

TEST_F(PudUnifierTest, AnUnboundVarIsBoundToTheOtherSideAsIsWithItsFrame) {
    const expr* term = functor(1, {var(0)});
    EXPECT_CALL(instantiate, instantiate(singleton(2), (framed_expr{term, 8})));
    EXPECT_TRUE(unifier.unify({var(2), 0}, {term, 8}));
}

TEST_F(PudUnifierTest, VarOnEitherSideBindsTheSameWay) {
    const expr* term = functor(1, {});
    EXPECT_CALL(instantiate, instantiate(singleton(2), (framed_expr{term, 4})));
    EXPECT_TRUE(unifier.unify({term, 4}, {var(2), 0}));
}

TEST_F(PudUnifierTest, ABoundVarUnifiesItsValueInsteadOfBinding) {
    ON_CALL(find, find(2)).WillByDefault(Return(bound(2, framed_expr{functor(1, {var(0)}), 10})));
    const expr* five = functor(5, {});
    EXPECT_CALL(instantiate, instantiate(singleton(10), (framed_expr{five, 0})));
    EXPECT_CALL(instantiate, instantiate(Field(&ubs_class::root, 2u), _)).Times(0);
    EXPECT_TRUE(unifier.unify({var(2), 0}, {functor(1, {five}), 0}));
}

TEST_F(PudUnifierTest, ADisplacedValueIsUnifiedWithTheMergedClass) {
    const framed_expr kept{functor(3, {var(0)}), 20};
    const framed_expr displaced{functor(3, {functor(9, {})}), 0};
    const ubs_class merged = bound(1, kept);
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(1)).WillOnce(Return(bound(1, kept))).WillOnce(Return(merged));
    EXPECT_CALL(find, find(2)).WillOnce(Return(bound(2, displaced)));
    InSequence in_order;
    EXPECT_CALL(equate, equate(bound(1, kept), bound(2, displaced))).WillOnce(Return(displaced));
    EXPECT_CALL(instantiate, instantiate(singleton(20), _));
    EXPECT_TRUE(unifier.unify({var(1), 0}, {var(2), 0}));
}

TEST_F(PudUnifierTest, ArgumentsAreUnifiedFirstToLast) {
    InSequence in_order;
    EXPECT_CALL(instantiate, instantiate(singleton(0), _));
    EXPECT_CALL(instantiate, instantiate(singleton(1), _));
    EXPECT_TRUE(unifier.unify(
        {functor(1, {var(0), var(1)}), 0},
        {functor(1, {functor(2, {}), functor(3, {})}), 0}));
}

TEST_F(PudUnifierTest, ASharedPairOfSubtermsIsUnifiedOnce) {
    const expr* left_shared = functor(2, {var(0)});
    const expr* right_shared = functor(2, {var(1)});
    EXPECT_CALL(find, find(_)).Times(AnyNumber());
    EXPECT_CALL(find, find(0)).Times(1);
    EXPECT_CALL(equate, equate(_, _)).Times(1);
    EXPECT_TRUE(unifier.unify(
        {functor(1, {left_shared, left_shared}), 0},
        {functor(1, {right_shared, right_shared}), 0}));
}

TEST_F(PudUnifierTest, MismatchedFunctorIdsFail) {
    EXPECT_FALSE(unifier.unify({functor(1, {}), 0}, {functor(2, {}), 0}));
}

TEST_F(PudUnifierTest, MismatchedArityFails) {
    EXPECT_FALSE(unifier.unify({functor(1, {functor(2, {})}), 0}, {functor(1, {}), 0}));
}

TEST_F(PudUnifierTest, DeepMismatchFails) {
    EXPECT_FALSE(unifier.unify(
        {functor(1, {functor(2, {functor(3, {})})}), 0},
        {functor(1, {functor(2, {functor(4, {})})}), 0}));
}

TEST_F(PudUnifierTest, EveryBoundOrMergedClassIsCheckedForCyclesOnce) {
    EXPECT_CALL(detect_cycle, find_cycle_from_class(0u, _)).WillOnce(Return(false));
    EXPECT_CALL(detect_cycle, find_cycle_from_class(1u, _)).WillOnce(Return(false));
    EXPECT_TRUE(unifier.unify(
        {functor(1, {var(0), var(1)}), 0},
        {functor(1, {functor(2, {}), var(5)}), 0}));
}

TEST_F(PudUnifierTest, ACycleFailsTheUnification) {
    EXPECT_CALL(detect_cycle, find_cycle_from_class(0u, _)).WillOnce(Return(true));
    EXPECT_FALSE(unifier.unify({var(0), 0}, {functor(1, {var(0)}), 0}));
}

TEST_F(PudUnifierTest, EqualTermsEditNothing) {
    const expr* term = functor(1, {functor(2, {})});
    EXPECT_EQ(unifier.unify({term, 0}, {term, 5}), std::optional<std::vector<ubs_class>>{std::vector<ubs_class>{}});
}

TEST_F(PudUnifierTest, AnInstantiatedClassIsReportedAsItWasBeforeTheEdit) {
    EXPECT_THAT(unifier.unify({var(2), 0}, {functor(1, {}), 8}).value(), ElementsAre(singleton(2)));
}

TEST_F(PudUnifierTest, BothEquatedClassesAreReportedAsTheyWereBeforeTheEdit) {
    EXPECT_THAT(unifier.unify({var(4), 0}, {var(6), 3}).value(), ElementsAre(singleton(4), singleton(9)));
}

TEST_F(PudUnifierTest, EveryEditIsReportedInTheOrderItWasMade) {
    EXPECT_THAT(
        unifier.unify({functor(1, {var(0), var(1)}), 0}, {functor(1, {functor(2, {}), var(5)}), 0}).value(),
        ElementsAre(singleton(0), singleton(1), singleton(5)));
}

TEST_F(PudUnifierTest, AFailedUnificationReportsNoEdits) {
    EXPECT_FALSE(unifier.unify({functor(1, {}), 0}, {functor(2, {}), 0}).has_value());
}

TEST_F(PudUnifierTest, AFailedUnificationSkipsTheCycleCheck) {
    EXPECT_CALL(detect_cycle, find_cycle_from_class(_, _)).Times(0);
    EXPECT_FALSE(unifier.unify({functor(1, {}), 0}, {functor(2, {}), 0}));
}
