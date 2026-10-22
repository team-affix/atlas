#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unifier.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};
struct MockFind {
    MOCK_METHOD(uint32_t, find, (uint32_t));
};
struct MockFindValue {
    MOCK_METHOD(std::optional<framed_expr>, find_value, (uint32_t));
};
struct MockEquate {
    MOCK_METHOD(void, equate, (uint32_t, uint32_t));
};
struct MockInstantiate {
    MOCK_METHOD(void, instantiate, (uint32_t, framed_expr));
};
struct MockDetectCycle {
    MOCK_METHOD(bool, detect_cycle, ((const std::vector<uint32_t>&)));
};

using test_unifier_t = pud_unifier<
    NiceMock<MockGlobalize>, NiceMock<MockFind>, NiceMock<MockFindValue>,
    NiceMock<MockEquate>, NiceMock<MockInstantiate>, NiceMock<MockDetectCycle>>;

struct PudUnifierTest : public ::testing::Test {
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockFind> find;
    NiceMock<MockFindValue> find_value;
    NiceMock<MockEquate> equate;
    NiceMock<MockInstantiate> instantiate;
    NiceMock<MockDetectCycle> detect_cycle;
    test_unifier_t unifier{globalize, find, find_value, equate, instantiate, detect_cycle};
    std::deque<expr> owned;
    std::unordered_map<uint32_t, uint32_t> roots;
    std::unordered_map<uint32_t, framed_expr> values;

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault([](uint32_t offset, uint32_t index) { return offset + index; });
        ON_CALL(find, find(_)).WillByDefault([this](uint32_t var) {
            return roots.contains(var) ? roots.at(var) : var;
        });
        ON_CALL(find_value, find_value(_)).WillByDefault([this](uint32_t root) -> std::optional<framed_expr> {
            if (!values.contains(root))
                return std::nullopt;
            return values.at(root);
        });
        ON_CALL(detect_cycle, detect_cycle(_)).WillByDefault(Return(false));
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
    EXPECT_CALL(detect_cycle, detect_cycle(IsEmpty()));
    const expr* term = functor(1, {functor(2, {})});
    EXPECT_TRUE(unifier.unify({term, 0}, {term, 5}));
}

TEST_F(PudUnifierTest, VarsInTheSameClassAreNotEquated) {
    roots[7] = 3;
    EXPECT_CALL(equate, equate(_, _)).Times(0);
    EXPECT_TRUE(unifier.unify({var(3), 0}, {var(2), 5}));
}

TEST_F(PudUnifierTest, TwoUnboundVarsEquateTheirRoots) {
    roots[4] = 1;
    EXPECT_CALL(equate, equate(1u, 9u));
    EXPECT_TRUE(unifier.unify({var(4), 0}, {var(6), 3}));
}

TEST_F(PudUnifierTest, AnUnboundVarIsBoundToTheOtherSideAsIsWithItsFrame) {
    const expr* term = functor(1, {var(0)});
    EXPECT_CALL(instantiate, instantiate(2u, (framed_expr{term, 8})));
    EXPECT_TRUE(unifier.unify({var(2), 0}, {term, 8}));
}

TEST_F(PudUnifierTest, VarOnEitherSideBindsTheSameWay) {
    const expr* term = functor(1, {});
    EXPECT_CALL(instantiate, instantiate(2u, (framed_expr{term, 4})));
    EXPECT_TRUE(unifier.unify({term, 4}, {var(2), 0}));
}

TEST_F(PudUnifierTest, ABoundVarUnifiesItsValueInsteadOfBinding) {
    values[2] = framed_expr{functor(1, {var(0)}), 10};
    const expr* five = functor(5, {});
    EXPECT_CALL(instantiate, instantiate(10u, (framed_expr{five, 0})));
    EXPECT_CALL(instantiate, instantiate(2u, _)).Times(0);
    EXPECT_TRUE(unifier.unify({var(2), 0}, {functor(1, {five}), 0}));
}

TEST_F(PudUnifierTest, TwoBoundVarsAreEquatedThenTheirValuesUnified) {
    values[1] = framed_expr{functor(3, {var(0)}), 20};
    values[2] = framed_expr{functor(3, {functor(9, {})}), 0};
    InSequence in_order;
    EXPECT_CALL(equate, equate(1u, 2u));
    EXPECT_CALL(instantiate, instantiate(20u, _));
    EXPECT_TRUE(unifier.unify({var(1), 0}, {var(2), 0}));
}

TEST_F(PudUnifierTest, ABoundAndAnUnboundVarAreOnlyEquated) {
    values[1] = framed_expr{functor(3, {}), 0};
    EXPECT_CALL(equate, equate(1u, 2u));
    EXPECT_CALL(instantiate, instantiate(_, _)).Times(0);
    EXPECT_TRUE(unifier.unify({var(1), 0}, {var(2), 0}));
}

TEST_F(PudUnifierTest, ArgumentsAreUnifiedFirstToLast) {
    InSequence in_order;
    EXPECT_CALL(instantiate, instantiate(0u, _));
    EXPECT_CALL(instantiate, instantiate(1u, _));
    EXPECT_TRUE(unifier.unify(
        {functor(1, {var(0), var(1)}), 0},
        {functor(1, {functor(2, {}), functor(3, {})}), 0}));
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
    EXPECT_CALL(detect_cycle, detect_cycle(UnorderedElementsAre(0u, 1u))).WillOnce(Return(false));
    EXPECT_TRUE(unifier.unify(
        {functor(1, {var(0), var(1)}), 0},
        {functor(1, {functor(2, {}), var(5)}), 0}));
}

TEST_F(PudUnifierTest, ACycleFailsTheUnification) {
    EXPECT_CALL(detect_cycle, detect_cycle(ElementsAre(0u))).WillOnce(Return(true));
    EXPECT_FALSE(unifier.unify({var(0), 0}, {functor(1, {var(0)}), 0}));
}

TEST_F(PudUnifierTest, AFailedUnificationSkipsTheCycleCheck) {
    EXPECT_CALL(detect_cycle, detect_cycle(_)).Times(0);
    EXPECT_FALSE(unifier.unify({functor(1, {}), 0}, {functor(2, {}), 0}));
}
