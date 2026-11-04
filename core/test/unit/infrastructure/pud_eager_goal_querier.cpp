#include <cstdint>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_eager_goal_querier.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_specialization_state.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockEnterReplay {
    MOCK_METHOD(
        std::optional<pud_replay>,
        enter,
        (const pud_ubs_state&, const expr*, uint32_t, pud_global_node_id, pud_module_id));
};
struct MockListVisibleAxioms {
    MOCK_METHOD(const std::vector<pud_global_node_id>&, list_visible_axioms, (pud_module_id), (const));
};
struct MockSpecializeCandidate {
    MOCK_METHOD(bool, specialize, (pud_specialization_state&));
};

std::optional<pud_replay> pud_enter_axiom(
    const pud_ubs_state&, const expr*, uint32_t frame_offset, pud_global_node_id axiom, pud_module_id) {
    return pud_replay{.node = axiom, .frame_offset = frame_offset, .ubs_state = {}};
}
// The candidate stays on its axiom root, which is a leaf: it is its own witness.
bool pud_justify_in_place(pud_specialization_state&) {
    return true;
}
// The candidate moves to node 5 and is justified by leaves 8 and 9 below it.
bool pud_justify_below(pud_specialization_state& state) {
    state.path.push_back(pud_test_node(5));
    state.current = pud_replay{.node = pud_test_node(5), .frame_offset = 0u, .ubs_state = {}};
    state.witnesses = pud_specialization_witnesses{
        .a = pud_specialization_witness{
            .search_root = state.current, .search = {.frames = {}}, .leaf = pud_test_node(8)},
        .b = pud_specialization_witness{
            .search_root = state.current, .search = {.frames = {}}, .leaf = pud_test_node(9)},
        .next_child = 2,
    };
    return true;
}

using test_querier_t = pud_eager_goal_querier<
    NiceMock<MockEnterReplay>, NiceMock<MockListVisibleAxioms>, NiceMock<MockSpecializeCandidate>>;

struct PudEagerGoalQuerierTest : public ::testing::Test {
    NiceMock<MockEnterReplay> enter;
    NiceMock<MockListVisibleAxioms> axioms;
    NiceMock<MockSpecializeCandidate> specialize;
    test_querier_t querier{enter, axioms, specialize};
    std::vector<pud_global_node_id> visible{pud_test_node(0), pud_test_node(1)};
    pud_ubs_state bindings{};
    expr goal{.content = expr::var{.index = 0}};

    void SetUp() override {
        ON_CALL(axioms, list_visible_axioms(k_test_module)).WillByDefault(ReturnRef(visible));
        ON_CALL(enter, enter(_, _, _, _, _)).WillByDefault(Invoke(&pud_enter_axiom));
        ON_CALL(specialize, specialize(_)).WillByDefault(Invoke(&pud_justify_in_place));
    }
};

}

TEST_F(PudEagerGoalQuerierTest, EveryJustifiedVisibleAxiomIsACandidateInAxiomOrder) {
    const std::vector<pud_eager_candidate> candidates = querier.query(k_test_module, bindings, &goal, 3);
    ASSERT_EQ(candidates.size(), 2u);
    EXPECT_THAT(candidates.at(0).path, ElementsAre(pud_test_node(0)));
    EXPECT_THAT(candidates.at(1).path, ElementsAre(pud_test_node(1)));
}

TEST_F(PudEagerGoalQuerierTest, EachAxiomIsEnteredWithTheGoalTheCallersBindingsAndItsModule) {
    EXPECT_CALL(enter, enter(_, &goal, 3u, pud_test_node(0), k_test_module));
    EXPECT_CALL(enter, enter(_, &goal, 3u, pud_test_node(1), k_test_module));
    querier.query(k_test_module, bindings, &goal, 3);
}

TEST_F(PudEagerGoalQuerierTest, AnAxiomWhoseHeadDoesNotResolveTheGoalIsNotSpecialized) {
    ON_CALL(enter, enter(_, _, _, pud_test_node(0), _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(specialize, specialize(Field(&pud_specialization_state::path, ElementsAre(pud_test_node(1)))));
    const std::vector<pud_eager_candidate> candidates = querier.query(k_test_module, bindings, &goal, 3);
    ASSERT_EQ(candidates.size(), 1u);
    EXPECT_THAT(candidates.at(0).path, ElementsAre(pud_test_node(1)));
}

TEST_F(PudEagerGoalQuerierTest, SpecializationStartsAtTheAxiomRootWithNoWitnesses) {
    EXPECT_CALL(
        specialize,
        specialize(AllOf(
            Field(&pud_specialization_state::path, ElementsAre(pud_test_node(0))),
            Field(&pud_specialization_state::current, Field(&pud_replay::node, pud_test_node(0))),
            Field(&pud_specialization_state::witnesses, std::nullopt))));
    EXPECT_CALL(specialize, specialize(Field(&pud_specialization_state::path, ElementsAre(pud_test_node(1)))));
    querier.query(k_test_module, bindings, &goal, 3);
}

TEST_F(PudEagerGoalQuerierTest, AnUnjustifiedAxiomIsNotACandidate) {
    ON_CALL(specialize, specialize(_)).WillByDefault(Return(false));
    EXPECT_THAT(querier.query(k_test_module, bindings, &goal, 3), IsEmpty());
}

TEST_F(PudEagerGoalQuerierTest, ACandidateLeftOnALeafIsItsOwnWitness) {
    const std::vector<pud_eager_candidate> candidates = querier.query(k_test_module, bindings, &goal, 3);
    EXPECT_THAT(candidates.at(0).witness_leaves, ElementsAre(pud_test_node(0)));
}

TEST_F(PudEagerGoalQuerierTest, ACandidateWithTwoWitnessesListsTheirLeavesAndItsSpecializedPath) {
    ON_CALL(specialize, specialize(_)).WillByDefault(Invoke(&pud_justify_below));
    const std::vector<pud_eager_candidate> candidates = querier.query(k_test_module, bindings, &goal, 3);
    EXPECT_THAT(candidates.at(0).path, ElementsAre(pud_test_node(0), pud_test_node(5)));
    EXPECT_THAT(candidates.at(0).witness_leaves, ElementsAre(pud_test_node(8), pud_test_node(9)));
}
