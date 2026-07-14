// Integration slice: a real dbuct_cdcl_elimination_generator wired to a real
// dbuct_avoidance_unit_boundary. In production the boundary object is exactly what
// supplies three of the generator's five collaborators -- IGetPenultimateMctsFrameDepth,
// IGetUltimateDecision, IGetPenultimateDecision -- so this proves the two cooperate
// end to end: real decision bookkeeping (ultimate / penultimate / the one-decision
// lagging penultimate mcts frame depth) flows into learn(), and the resulting raised unit
// avoidance is emitted-vs-armed at pop according to the boundary the REAL component
// computed.
//
// Everything outside the slice is mocked: the mcts frame depth the boundary
// depends on, the nearest-decision oracle that drives its rotate/overwrite branch,
// and the generator's chosen-candidate and derive-lemma collaborators. Assertions
// are made only against the eliminations yielded by constrain()/pop_frame().

#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct fake_mcts_frame_depth {
    size_t depth_value;
    size_t mcts_frame_depth() const { return depth_value; }
};

struct MockGetNearestDecision {
    MOCK_METHOD(const resolution_lineage*, get_nearest_decision, (const resolution_lineage*), (const));
};
struct MockTryGetChosenGoalCandidate {
    MOCK_METHOD(std::optional<rule_id>, try_get, (const goal_lineage*), (const));
};
struct MockDeriveDecisionLemma {
    MOCK_METHOD(lemma, derive_decision_lemma, ());
};

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const resolution_lineage* v = sm.consume_yield();
            if (v != nullptr)
                out.push_back(v);
        }
    }
    return out;
}

lemma make_lemma(std::initializer_list<const resolution_lineage*> rs) {
    return lemma{{rs.begin(), rs.end()}};
}

using boundary_t = dbuct_avoidance_unit_boundary<NiceMock<MockGetNearestDecision>,
                                                 fake_mcts_frame_depth>;
using sut_t = dbuct_cdcl_elimination_generator<NiceMock<MockTryGetChosenGoalCandidate>,
                                               boundary_t,
                                               NiceMock<MockDeriveDecisionLemma>,
                                               boundary_t,
                                               boundary_t>;

struct DbuctCdclEliminationGeneratorIntegrationTest : public ::testing::Test {
    fake_mcts_frame_depth fc{1};
    NiceMock<MockGetNearestDecision> nd;
    NiceMock<MockTryGetChosenGoalCandidate> tgcc;
    NiceMock<MockDeriveDecisionLemma> dl;

    boundary_t aub{nd, fc};
    sut_t sut{tgcc, aub, dl, aub, aub};

    resolution_lineage gpr1{nullptr, 10};
    goal_lineage gg1{&gpr1, 0};
    resolution_lineage D1{&gg1, 0};

    resolution_lineage gpr2{nullptr, 20};
    goal_lineage gg2{&gpr2, 0};
    resolution_lineage D2{&gg2, 0};

    resolution_lineage gpr3{nullptr, 30};
    goal_lineage gg3{&gpr3, 0};
    resolution_lineage D3{&gg3, 0};

    resolution_lineage sentinel{nullptr, 99};

    void SetUp() override {
        ON_CALL(nd, get_nearest_decision(_)).WillByDefault(Return(&sentinel));
        ON_CALL(tgcc, try_get(_)).WillByDefault(Return(std::optional<rule_id>{}));
    }
};

TEST_F(DbuctCdclEliminationGeneratorIntegrationTest, RotatedBoundaryKeepsConflictUnitOnPop) {
    fc.depth_value = 1;
    aub.log_decision(&D1);
    sut.push_frame();
    fc.depth_value = 2;
    aub.log_decision(&D2);

    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({&D2, &D1})));
    sut.learn();

    EXPECT_THAT(collect_elims(sut.pop_frame()), ElementsAre(&D2));
}

TEST_F(DbuctCdclEliminationGeneratorIntegrationTest, DeepBoundaryEmitsThenArmsThenPropagates) {
    fc.depth_value = 1;
    aub.log_decision(&D1);
    sut.push_frame();
    fc.depth_value = 2;
    aub.log_decision(&D2);
    sut.push_frame();
    fc.depth_value = 3;
    aub.log_decision(&D3);

    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({&D3, &D2})));
    sut.learn();

    EXPECT_THAT(collect_elims(sut.pop_frame()), ElementsAre(&D3));
    EXPECT_THAT(collect_elims(sut.pop_frame()), IsEmpty());
    EXPECT_THAT(collect_elims(sut.constrain(&D3)), ElementsAre(&D2));
}

}  // namespace
