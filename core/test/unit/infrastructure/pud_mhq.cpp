#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhq.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockEnterReplay {
    MOCK_METHOD(std::optional<pud_replay>, enter, (const pud_ubs_state&, const expr*, uint32_t, pud_node_id));
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
struct MockInvalidateCandidateLeaves {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, invalidate_leaves, (const std::vector<pud_node_id>&));
};
struct MockPushUnitQuery {
    MOCK_METHOD(void, push_back, (pud_mhq_head_id));
};

pud_replay replay_at(pud_node_id node, uint32_t frame_offset) {
    return pud_replay{.node = node, .frame_offset = frame_offset, .ubs_state = {}};
}

using test_mhq_t = pud_mhq<
    NiceMock<MockEnterReplay>, std::vector<pud_node_id>, NiceMock<MockTryAddCandidate>, NiceMock<MockTryForkCandidate>,
    NiceMock<MockRemoveCandidate>, NiceMock<MockInvalidateCandidateLeaves>, NiceMock<MockPushUnitQuery>>;

struct PudMhqTest : public ::testing::Test {
    NiceMock<MockEnterReplay> enter;
    NiceMock<MockTryAddCandidate> add_candidate;
    NiceMock<MockTryForkCandidate> fork_candidate;
    NiceMock<MockRemoveCandidate> remove_candidate;
    NiceMock<MockInvalidateCandidateLeaves> invalidate_candidates;
    NiceMock<MockPushUnitQuery> unit_queries;
    std::vector<pud_node_id> axioms{0, 1, 2};
    test_mhq_t mhq{enter, axioms, add_candidate, fork_candidate, remove_candidate, invalidate_candidates, unit_queries};
    expr goal{expr::functor{3, {}}};
    expr specialized_goal{expr::functor{4, {}}};

    // The goal enters every axiom at frame 7 and the specialized goal at frame 9.
    void enterable(pud_node_id axiom) {
        ON_CALL(enter, enter(_, &goal, 7u, axiom)).WillByDefault(Return(replay_at(axiom, 7)));
        ON_CALL(enter, enter(_, &specialized_goal, 9u, axiom)).WillByDefault(Return(replay_at(axiom, 9)));
    }

    void candidate_under(pud_node_id axiom, pud_mhcs_head_id candidate) {
        enterable(axiom);
        ON_CALL(add_candidate, try_add(Field(&pud_replay::node, axiom))).WillByDefault(Return(candidate));
    }

    std::optional<pud_mhq_head_id> open() {
        return mhq.try_open(pud_ubs_state{}, &goal, 7);
    }

    std::optional<pud_mhq_head_id> fork(pud_mhq_head_id head) {
        return mhq.try_fork(head, pud_ubs_state{}, &specialized_goal, 9);
    }

    void refute_candidates(const std::vector<pud_mhcs_head_id>& candidates) {
        ON_CALL(invalidate_candidates, invalidate_leaves(_)).WillByDefault(Return(candidates));
    }
};

}

TEST_F(PudMhqTest, AQueryIsOpenedWithACandidateUnderEveryAxiomItEntersInAxiomOrder) {
    candidate_under(0, 30);
    candidate_under(2, 20);
    const pud_mhq_head_id head = open().value();
    EXPECT_THAT(mhq.candidates(head), ElementsAre(30u, 20u));
}

TEST_F(PudMhqTest, AnAxiomWhoseHeadDoesNotUnifyIsNotSearched) {
    candidate_under(0, 30);
    candidate_under(2, 20);
    EXPECT_CALL(add_candidate, try_add(_)).Times(AnyNumber());
    EXPECT_CALL(add_candidate, try_add(Field(&pud_replay::node, 1u))).Times(0);
    open();
}

TEST_F(PudMhqTest, AnAxiomWithoutACandidateIsLeftOut) {
    candidate_under(0, 30);
    enterable(1);
    candidate_under(2, 20);
    EXPECT_THAT(mhq.candidates(open().value()), ElementsAre(30u, 20u));
}

TEST_F(PudMhqTest, AQueryWithoutCandidatesIsNotOpened) {
    enterable(0);
    EXPECT_CALL(unit_queries, push_back(_)).Times(0);
    EXPECT_FALSE(open().has_value());
}

TEST_F(PudMhqTest, AQueryOpenedWithOneCandidateIsUnit) {
    candidate_under(1, 30);
    EXPECT_CALL(unit_queries, push_back(_));
    open();
}

TEST_F(PudMhqTest, AQueryOpenedWithTwoCandidatesIsNotUnit) {
    candidate_under(0, 30);
    candidate_under(1, 31);
    EXPECT_CALL(unit_queries, push_back(_)).Times(0);
    open();
}

TEST_F(PudMhqTest, EveryLeafIsPassedOnToTheCandidates) {
    EXPECT_CALL(invalidate_candidates, invalidate_leaves(ElementsAre(5u, 6u)));
    mhq.invalidate_leaves({5, 6});
}

TEST_F(PudMhqTest, LosingAllButOneCandidateMakesTheQueryUnit) {
    candidate_under(0, 30);
    candidate_under(1, 31);
    const pud_mhq_head_id head = open().value();
    refute_candidates({30});
    EXPECT_CALL(unit_queries, push_back(head));
    EXPECT_THAT(mhq.invalidate_leaves({5}), IsEmpty());
    EXPECT_THAT(mhq.candidates(head), ElementsAre(31u));
}

TEST_F(PudMhqTest, LosingTheLastCandidateRefutesTheQueryUntilItIsRemoved) {
    candidate_under(0, 30);
    const pud_mhq_head_id head = open().value();
    refute_candidates({30});
    EXPECT_THAT(mhq.invalidate_leaves({5}), ElementsAre(head));
    EXPECT_THAT(mhq.candidates(head), IsEmpty());
    EXPECT_CALL(remove_candidate, remove(_)).Times(0);
    mhq.remove(head);
}

TEST_F(PudMhqTest, RefutedCandidatesReachOnlyTheirOwnQuery) {
    candidate_under(0, 30);
    const pud_mhq_head_id first = open().value();
    candidate_under(0, 40);
    candidate_under(1, 41);
    const pud_mhq_head_id second = open().value();
    refute_candidates({30, 40});
    EXPECT_THAT(mhq.invalidate_leaves({5}), ElementsAre(first));
    EXPECT_THAT(mhq.candidates(second), ElementsAre(41u));
}

TEST_F(PudMhqTest, RemovingAQueryRemovesItsCandidates) {
    candidate_under(0, 30);
    candidate_under(2, 32);
    const pud_mhq_head_id head = open().value();
    EXPECT_CALL(remove_candidate, remove(30u));
    EXPECT_CALL(remove_candidate, remove(32u));
    mhq.remove(head);
}

TEST_F(PudMhqTest, AForkForksEachCandidateOntoItsAxiomInTheNewEnvironment) {
    candidate_under(0, 30);
    candidate_under(2, 32);
    const pud_mhq_head_id head = open().value();
    EXPECT_CALL(fork_candidate, try_fork(30u, AllOf(Field(&pud_replay::node, 0u), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(50u));
    EXPECT_CALL(fork_candidate, try_fork(32u, AllOf(Field(&pud_replay::node, 2u), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(52u));
    EXPECT_THAT(mhq.candidates(fork(head).value()), ElementsAre(50u, 52u));
}

TEST_F(PudMhqTest, AForkOnlyEntersTheAxiomsTheForkedQueryStillHasCandidatesUnder) {
    candidate_under(0, 30);
    candidate_under(2, 32);
    const pud_mhq_head_id head = open().value();
    EXPECT_CALL(enter, enter(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(enter, enter(_, &specialized_goal, 9u, 1u)).Times(0);
    fork(head);
}

TEST_F(PudMhqTest, AForkWithoutCandidatesIsNotOpenedAndLeavesTheForkedQuery) {
    candidate_under(0, 30);
    candidate_under(2, 32);
    const pud_mhq_head_id head = open().value();
    EXPECT_FALSE(fork(head).has_value());
    EXPECT_THAT(mhq.candidates(head), ElementsAre(30u, 32u));
}

TEST_F(PudMhqTest, AForkKeepingOneCandidateIsUnit) {
    candidate_under(0, 30);
    candidate_under(2, 32);
    const pud_mhq_head_id head = open().value();
    ON_CALL(fork_candidate, try_fork(32u, _)).WillByDefault(Return(52u));
    EXPECT_CALL(unit_queries, push_back(_));
    fork(head);
}
