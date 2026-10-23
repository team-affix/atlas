#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_replayer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/rule.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id));
};
struct MockGetInference {
    MOCK_METHOD(const pud_node_delta&, get_inference, (pud_global_node_id));
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};
struct MockBindMap {
    MOCK_METHOD(pud_ubs_state, persist, (), (ref(&&)));
};
using edited_t = std::optional<std::vector<ubs_class>>;

const edited_t k_unified{std::vector<ubs_class>{}};
const edited_t k_not_unified{std::nullopt};

struct MockCycleDetector {};
struct MockUnifier {
    MOCK_METHOD(edited_t, unify, (framed_expr, framed_expr));
};
struct MockMakeBindMap {
    MOCK_METHOD(MockBindMap&, make_bind_map, (const pud_ubs_state&));
};
struct MockMakeCycleDetector {
    MOCK_METHOD(MockCycleDetector&, make_cycle_detector, (MockBindMap&));
};
struct MockMakeUnifier {
    MOCK_METHOD(MockUnifier&, make_unifier, (MockBindMap&, MockCycleDetector&));
};

using test_replayer_t = pud_replayer<
    NiceMock<MockGetAxiom>, NiceMock<MockGetInference>, NiceMock<MockMakeVar>,
    NiceMock<MockMakeBindMap>, NiceMock<MockMakeCycleDetector>, NiceMock<MockMakeUnifier>>;

struct PudReplayerTest : public ::testing::Test {
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockBindMap> bind_map;
    MockCycleDetector cycle_detector;
    NiceMock<MockUnifier> unifier;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockMakeCycleDetector> make_cycle_detector;
    NiceMock<MockMakeUnifier> make_unifier;
    test_replayer_t replayer{axioms, inferences, make_var, make_bind_map, make_cycle_detector, make_unifier};
    expr goal{expr::functor{1, {}}};
    expr head{expr::functor{2, {}}};
    expr var_three{expr::var{3}};
    expr var_four{expr::var{4}};
    expr value{expr::functor{5, {}}};
    expr other_value{expr::functor{6, {}}};
    rule axiom{&head, {}, 0};
    pud_node_delta child_inference{
        .parent = 8,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 3, .value = &value}, pud_substitution{.var_idx = 4, .value = &other_value}},
        .body_goals = {},
        .var_count = 0,
    };
    pud_ubs_state caller_state;
    pud_ubs_state persisted_state;

    void SetUp() override {
        distinguish_persisted_state();
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(bind_map));
        ON_CALL(make_cycle_detector, make_cycle_detector(_)).WillByDefault(ReturnRef(cycle_detector));
        ON_CALL(make_unifier, make_unifier(_, _)).WillByDefault(ReturnRef(unifier));
        ON_CALL(std::move(bind_map), persist()).WillByDefault(Return(persisted_state));
        ON_CALL(axioms, get_axiom(pud_test_node(8))).WillByDefault(ReturnRef(axiom));
        ON_CALL(inferences, get_inference(pud_test_node(9))).WillByDefault(ReturnRef(child_inference));
        ON_CALL(make_var, make_var(3)).WillByDefault(Return(&var_three));
        ON_CALL(make_var, make_var(4)).WillByDefault(Return(&var_four));
        ON_CALL(unifier, unify(_, _)).WillByDefault(Return(k_unified));
    }

    // Distinguishes the persisted state from the caller's, so a test sees which one a replay carries.
    void distinguish_persisted_state() {
        persisted_state.parents = persisted_state.parents.set(1, 0);
    }
};

}

TEST_F(PudReplayerTest, EnteringAnAxiomUnifiesTheGoalWithTheHeadFramedAtTheOffset) {
    EXPECT_CALL(make_bind_map, make_bind_map(caller_state));
    EXPECT_CALL(unifier, unify((framed_expr{&goal, 0}), (framed_expr{&head, 7}))).WillOnce(Return(k_unified));
    EXPECT_EQ(
        replayer.enter(caller_state, &goal, 7, pud_test_node(8)),
        (pud_replay{.node = pud_test_node(8), .frame_offset = 7, .ubs_state = persisted_state}));
}

TEST_F(PudReplayerTest, EnteringFailsWhenTheHeadDoesNotUnify) {
    ON_CALL(unifier, unify(_, _)).WillByDefault(Return(k_not_unified));
    EXPECT_FALSE(replayer.enter(caller_state, &goal, 7, pud_test_node(8)).has_value());
}

TEST_F(PudReplayerTest, DescendingReplaysEverySubstitutionAtTheFrameOffsetInOrder) {
    const pud_replay parent{.node = pud_test_node(8), .frame_offset = 7, .ubs_state = caller_state};
    InSequence in_order;
    EXPECT_CALL(make_bind_map, make_bind_map(caller_state));
    EXPECT_CALL(unifier, unify((framed_expr{&var_three, 7}), (framed_expr{&value, 7}))).WillOnce(Return(k_unified));
    EXPECT_CALL(unifier, unify((framed_expr{&var_four, 7}), (framed_expr{&other_value, 7}))).WillOnce(Return(k_unified));
    EXPECT_EQ(
        replayer.descend(parent, pud_test_node(9)),
        (pud_replay{.node = pud_test_node(9), .frame_offset = 7, .ubs_state = persisted_state}));
}

TEST_F(PudReplayerTest, DescendingStopsAtTheFirstSubstitutionThatFails) {
    const pud_replay parent{.node = pud_test_node(8), .frame_offset = 7, .ubs_state = caller_state};
    EXPECT_CALL(unifier, unify(_, _)).WillOnce(Return(k_not_unified));
    EXPECT_CALL(std::move(bind_map), persist()).Times(0);
    EXPECT_FALSE(replayer.descend(parent, pud_test_node(9)).has_value());
}
