#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhq.hpp"
#include "value_objects/expr.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockEnterReplay {
    MOCK_METHOD(std::optional<pud_replay>, enter, (const pud_ubs_state&, const expr*, uint32_t, pud_global_node_id, pud_module_id));
};
struct MockListVisibleAxioms {
    MOCK_METHOD(const std::vector<pud_global_node_id>&, list_visible_axioms, (pud_module_id), (const));
};
struct MockTryAddCandidate {
    MOCK_METHOD(std::optional<pud_mhcs_head_id>, try_add, (pud_replay));
};
struct MockTryForkCandidate {
    MOCK_METHOD(std::optional<pud_mhcs_head_id>, try_fork, (pud_mhcs_head_id, pud_replay));
};
struct MockRemoveCandidate {
    MOCK_METHOD(void, remove, (pud_mhcs_head_id));
};
struct MockInvalidateCandidateLeaf {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, invalidate_leaf, (pud_global_node_id));
};
struct MockPushUnitQuery {
    MOCK_METHOD(void, push_back, (pud_mhq_head_id));
};

using test_mhq_t = pud_mhq<
    NiceMock<MockEnterReplay>, NiceMock<MockListVisibleAxioms>, NiceMock<MockTryAddCandidate>,
    NiceMock<MockTryForkCandidate>, NiceMock<MockRemoveCandidate>, NiceMock<MockInvalidateCandidateLeaf>,
    NiceMock<MockPushUnitQuery>>;

struct PudMhqTest : public ::testing::Test {
    NiceMock<MockEnterReplay> enter;
    NiceMock<MockListVisibleAxioms> axioms;
    NiceMock<MockTryAddCandidate> add_candidate;
    NiceMock<MockTryForkCandidate> fork_candidate;
    NiceMock<MockRemoveCandidate> remove_candidate;
    NiceMock<MockInvalidateCandidateLeaf> invalidate_candidates;
    NiceMock<MockPushUnitQuery> unit_queries;
    std::vector<pud_global_node_id> visible_axioms{pud_test_node(0), pud_test_node(1), pud_test_node(2)};
    test_mhq_t mhq{enter, axioms, add_candidate, fork_candidate, remove_candidate, invalidate_candidates, unit_queries};
    expr goal{expr::functor{3, {}}};
    expr specialized_goal{expr::functor{4, {}}};

    void SetUp() override {
        ON_CALL(axioms, list_visible_axioms(k_test_module)).WillByDefault(ReturnRef(visible_axioms));
    }
};

}

TEST_F(PudMhqTest, AQueryIsOpenedWithACandidateUnderEveryAxiomItEntersInAxiomOrder) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(20u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    EXPECT_THAT(mhq.candidates(head), ElementsAre(30u, 20u));
}

TEST_F(PudMhqTest, AnAxiomWhoseHeadDoesNotUnifyIsNotSearched) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(20u));
    EXPECT_CALL(add_candidate, try_add(_)).Times(AnyNumber());
    EXPECT_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(1)))).Times(0);
    mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7);
}

TEST_F(PudMhqTest, AnAxiomWithoutACandidateIsLeftOut) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(20u));
    EXPECT_THAT(mhq.candidates(mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value()), ElementsAre(30u, 20u));
}

TEST_F(PudMhqTest, AQueryWithoutCandidatesIsNotOpened) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    EXPECT_CALL(unit_queries, push_back(_)).Times(0);
    EXPECT_FALSE(mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).has_value());
}

TEST_F(PudMhqTest, AQueryOpenedWithOneCandidateIsUnit) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(30u));
    EXPECT_CALL(unit_queries, push_back(_));
    mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7);
}

TEST_F(PudMhqTest, AQueryOpenedWithTwoCandidatesIsNotUnit) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(31u));
    EXPECT_CALL(unit_queries, push_back(_)).Times(0);
    mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7);
}

TEST_F(PudMhqTest, InvalidatingALeafIsPassedOnToTheCandidates) {
    EXPECT_CALL(invalidate_candidates, invalidate_leaf(pud_test_node(5)));
    mhq.invalidate_leaf(pud_test_node(5));
}

TEST_F(PudMhqTest, LosingAllButOneCandidateMakesTheQueryUnit) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(31u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    ON_CALL(invalidate_candidates, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30}));
    EXPECT_CALL(unit_queries, push_back(head));
    EXPECT_THAT(mhq.invalidate_leaf(pud_test_node(5)), IsEmpty());
    EXPECT_THAT(mhq.candidates(head), ElementsAre(31u));
}

TEST_F(PudMhqTest, LosingTheLastCandidateRefutesTheQueryUntilItIsRemoved) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    ON_CALL(invalidate_candidates, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30}));
    EXPECT_THAT(mhq.invalidate_leaf(pud_test_node(5)), ElementsAre(head));
    EXPECT_THAT(mhq.candidates(head), IsEmpty());
    EXPECT_CALL(remove_candidate, remove(_)).Times(0);
    mhq.remove(head);
}

TEST_F(PudMhqTest, RefutedCandidatesReachOnlyTheirOwnQuery) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    const pud_mhq_head_id first = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(40u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(41u));
    const pud_mhq_head_id second = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    ON_CALL(invalidate_candidates, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 40}));
    EXPECT_THAT(mhq.invalidate_leaf(pud_test_node(5)), ElementsAre(first));
    EXPECT_THAT(mhq.candidates(second), ElementsAre(41u));
}

TEST_F(PudMhqTest, AQueryIsExhaustedOnlyOnceItsLastCandidateIsRefuted) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(1), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(31u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    EXPECT_FALSE(mhq.check_exhausted(head));
    ON_CALL(invalidate_candidates, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30}));
    mhq.invalidate_leaf(pud_test_node(5));
    EXPECT_FALSE(mhq.check_exhausted(head));
    ON_CALL(invalidate_candidates, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{31}));
    mhq.invalidate_leaf(pud_test_node(6));
    EXPECT_TRUE(mhq.check_exhausted(head));
}

TEST_F(PudMhqTest, RemovingAQueryRemovesItsCandidates) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(32u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    EXPECT_CALL(remove_candidate, remove(30u));
    EXPECT_CALL(remove_candidate, remove(32u));
    mhq.remove(head);
}

TEST_F(PudMhqTest, AForkForksEachCandidateOntoItsAxiomInTheNewEnvironment) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(32u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    EXPECT_CALL(
        fork_candidate,
        try_fork(30u, AllOf(Field(&pud_replay::node, pud_test_node(0)), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(50u));
    EXPECT_CALL(
        fork_candidate,
        try_fork(32u, AllOf(Field(&pud_replay::node, pud_test_node(2)), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(52u));
    EXPECT_THAT(mhq.candidates(mhq.try_fork(head, pud_ubs_state{}, &specialized_goal, 9).value()), ElementsAre(50u, 52u));
}

TEST_F(PudMhqTest, AForkOnlyEntersTheAxiomsTheForkedQueryStillHasCandidatesUnder) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(32u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    EXPECT_CALL(enter, enter(_, _, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(1), _)).Times(0);
    mhq.try_fork(head, pud_ubs_state{}, &specialized_goal, 9);
}

TEST_F(PudMhqTest, AForkWithoutCandidatesIsNotOpenedAndLeavesTheForkedQuery) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(32u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    EXPECT_FALSE(mhq.try_fork(head, pud_ubs_state{}, &specialized_goal, 9).has_value());
    EXPECT_THAT(mhq.candidates(head), ElementsAre(30u, 32u));
}

TEST_F(PudMhqTest, AForkKeepingOneCandidateIsUnit) {
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(32u));
    const pud_mhq_head_id head = mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value();
    ON_CALL(fork_candidate, try_fork(32u, _)).WillByDefault(Return(52u));
    EXPECT_CALL(unit_queries, push_back(_));
    mhq.try_fork(head, pud_ubs_state{}, &specialized_goal, 9);
}

TEST_F(PudMhqTest, AxiomsFromAnotherModuleAreNotVisible) {
    const pud_module_id other_module = k_test_module + 1;
    const pud_global_node_id foreign{.module = other_module, .node = 0};
    const std::vector<pud_global_node_id> other_visible_axioms{foreign};
    ON_CALL(axioms, list_visible_axioms(other_module)).WillByDefault(ReturnRef(other_visible_axioms));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(0), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(0)))).WillByDefault(Return(30u));
    ON_CALL(enter, enter(_, &goal, 7u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, pud_test_node(2), _))
        .WillByDefault(Return(pud_replay{.node = pud_test_node(2), .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(20u));
    ON_CALL(enter, enter(_, &goal, 7u, foreign, _))
        .WillByDefault(Return(pud_replay{.node = foreign, .frame_offset = 7u, .ubs_state = {}}));
    ON_CALL(enter, enter(_, &specialized_goal, 9u, foreign, _))
        .WillByDefault(Return(pud_replay{.node = foreign, .frame_offset = 9u, .ubs_state = {}}));
    ON_CALL(add_candidate, try_add(Field(&pud_replay::node, foreign))).WillByDefault(Return(40u));
    EXPECT_CALL(enter, enter(_, _, _, _, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(enter, enter(_, _, _, foreign, _)).Times(0);
    EXPECT_THAT(mhq.candidates(mhq.try_open(k_test_module, pud_ubs_state{}, &goal, 7).value()), ElementsAre(30u, 20u));
}
