#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_replayer.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetRootHead {
    MOCK_METHOD(const expr*, get, (pud_node_id));
};
struct MockGetNodeAddedSubstitutions {
    MOCK_METHOD(const std::vector<pud_substitution>&, get, (pud_node_id));
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};
struct MockBindMap {
    MOCK_METHOD(pud_ubs_state, persist, (), (ref(&&)));
};
struct MockCycleDetector {};
struct MockUnifier {
    MOCK_METHOD(bool, unify, (framed_expr, framed_expr));
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
    NiceMock<MockGetRootHead>, NiceMock<MockGetNodeAddedSubstitutions>, NiceMock<MockMakeVar>,
    NiceMock<MockMakeBindMap>, NiceMock<MockMakeCycleDetector>, NiceMock<MockMakeUnifier>>;

struct PudReplayerTest : public ::testing::Test {
    NiceMock<MockGetRootHead> root_heads;
    NiceMock<MockGetNodeAddedSubstitutions> substitutions;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockBindMap> bind_map;
    MockCycleDetector cycle_detector;
    NiceMock<MockUnifier> unifier;
    NiceMock<MockMakeBindMap> make_bind_map;
    NiceMock<MockMakeCycleDetector> make_cycle_detector;
    NiceMock<MockMakeUnifier> make_unifier;
    test_replayer_t replayer{root_heads, substitutions, make_var, make_bind_map, make_cycle_detector, make_unifier};
    expr goal{expr::functor{1, {}}};
    expr head{expr::functor{2, {}}};
    expr var_three{expr::var{3}};
    expr var_four{expr::var{4}};
    expr value{expr::functor{5, {}}};
    expr other_value{expr::functor{6, {}}};
    std::vector<pud_substitution> child_substitutions{
        pud_substitution{.var_idx = 3, .value = &value},
        pud_substitution{.var_idx = 4, .value = &other_value},
    };
    pud_ubs_state caller_state;
    pud_ubs_state persisted_state;

    void SetUp() override {
        distinguish_persisted_state();
        ON_CALL(make_bind_map, make_bind_map(_)).WillByDefault(ReturnRef(bind_map));
        ON_CALL(make_cycle_detector, make_cycle_detector(_)).WillByDefault(ReturnRef(cycle_detector));
        ON_CALL(make_unifier, make_unifier(_, _)).WillByDefault(ReturnRef(unifier));
        ON_CALL(std::move(bind_map), persist()).WillByDefault(Return(persisted_state));
        ON_CALL(root_heads, get(8)).WillByDefault(Return(&head));
        ON_CALL(substitutions, get(9)).WillByDefault(ReturnRef(child_substitutions));
        ON_CALL(make_var, make_var(3)).WillByDefault(Return(&var_three));
        ON_CALL(make_var, make_var(4)).WillByDefault(Return(&var_four));
        ON_CALL(unifier, unify(_, _)).WillByDefault(Return(true));
    }

    // Distinguishes the persisted state from the caller's, so a test sees which one a replay carries.
    void distinguish_persisted_state() {
        persisted_state.parents = persisted_state.parents.set(1, 0);
    }
};

}

TEST_F(PudReplayerTest, EnteringAnAxiomUnifiesTheGoalWithTheHeadFramedAtTheOffset) {
    EXPECT_CALL(make_bind_map, make_bind_map(caller_state));
    EXPECT_CALL(unifier, unify((framed_expr{&goal, 0}), (framed_expr{&head, 7}))).WillOnce(Return(true));
    EXPECT_EQ(
        replayer.enter(caller_state, &goal, 7, 8),
        (pud_replay{.node = 8, .frame_offset = 7, .ubs_state = persisted_state}));
}

TEST_F(PudReplayerTest, EnteringFailsWhenTheHeadDoesNotUnify) {
    ON_CALL(unifier, unify(_, _)).WillByDefault(Return(false));
    EXPECT_FALSE(replayer.enter(caller_state, &goal, 7, 8).has_value());
}

TEST_F(PudReplayerTest, DescendingReplaysEverySubstitutionAtTheFrameOffsetInOrder) {
    const pud_replay parent{.node = 8, .frame_offset = 7, .ubs_state = caller_state};
    InSequence in_order;
    EXPECT_CALL(make_bind_map, make_bind_map(caller_state));
    EXPECT_CALL(unifier, unify((framed_expr{&var_three, 7}), (framed_expr{&value, 7}))).WillOnce(Return(true));
    EXPECT_CALL(unifier, unify((framed_expr{&var_four, 7}), (framed_expr{&other_value, 7}))).WillOnce(Return(true));
    EXPECT_EQ(
        replayer.descend(parent, 9),
        (pud_replay{.node = 9, .frame_offset = 7, .ubs_state = persisted_state}));
}

TEST_F(PudReplayerTest, DescendingStopsAtTheFirstSubstitutionThatFails) {
    const pud_replay parent{.node = 8, .frame_offset = 7, .ubs_state = caller_state};
    EXPECT_CALL(unifier, unify(_, _)).WillOnce(Return(false));
    EXPECT_CALL(std::move(bind_map), persist()).Times(0);
    EXPECT_FALSE(replayer.descend(parent, 9).has_value());
}
