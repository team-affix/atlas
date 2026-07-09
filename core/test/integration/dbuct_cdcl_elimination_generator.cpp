// Integration slice: a real dbuct_cdcl_elimination_generator wired to a real
// dbuct_avoidance_unit_boundary. In production the boundary object is exactly what
// supplies three of the generator's five collaborators -- IGetUnitBoundary,
// IGetUltimateDecision, IGetPenultimateDecision -- so this proves the two cooperate
// end to end: real decision bookkeeping (ultimate / penultimate / the one-decision
// lagging unit boundary) flows into learn(), and the resulting raised unit
// avoidance is emitted-vs-armed at pop according to the boundary the REAL component
// computed.
//
// Everything outside the slice is mocked: the trail and frame-count the boundary
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
using ::testing::ReturnPointee;

namespace {

struct MockTrail {
    MOCK_METHOD(void, log, (std::unique_ptr<i_backtrackable>));
};
struct MockGetFrameCount {
    MOCK_METHOD(size_t, depth, (), (const));
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
                                                 NiceMock<MockGetFrameCount>,
                                                 NiceMock<MockTrail>>;
using sut_t = dbuct_cdcl_elimination_generator<NiceMock<MockTryGetChosenGoalCandidate>,
                                               boundary_t,
                                               NiceMock<MockDeriveDecisionLemma>,
                                               boundary_t,
                                               boundary_t>;

struct DbuctCdclEliminationGeneratorIntegrationTest : public ::testing::Test {
    NiceMock<MockTrail> trail;
    NiceMock<MockGetFrameCount> fc;
    NiceMock<MockGetNearestDecision> nd;
    NiceMock<MockTryGetChosenGoalCandidate> tgcc;
    NiceMock<MockDeriveDecisionLemma> dl;

    boundary_t aub{nd, fc, trail};
    sut_t sut{tgcc, aub, dl, aub, aub};

    // Three decisions, each on its own chain with a real grandparent resolution so
    // log_decision's rl->parent->parent lookup is well formed.
    resolution_lineage gpr1{nullptr, 10};
    goal_lineage gg1{&gpr1, 0};
    resolution_lineage D1{&gg1, 0};

    resolution_lineage gpr2{nullptr, 20};
    goal_lineage gg2{&gpr2, 0};
    resolution_lineage D2{&gg2, 0};

    resolution_lineage gpr3{nullptr, 30};
    goal_lineage gg3{&gpr3, 0};
    resolution_lineage D3{&gg3, 0};

    // Distinct from every ultimate, so the real nearest-decision map never matches
    // the current ultimate and each logged decision takes the ROTATE branch -- the
    // path that advances the lagging unit boundary.
    resolution_lineage sentinel{nullptr, 99};

    size_t depth = 1;  // mirrors the generator's frame depth (root frame == 1)

    void SetUp() override {
        ON_CALL(fc, depth()).WillByDefault(ReturnPointee(&depth));
        ON_CALL(nd, get_nearest_decision(_)).WillByDefault(Return(&sentinel));
        ON_CALL(tgcc, try_get(_)).WillByDefault(Return(std::optional<rule_id>{}));
    }
};

// Two rotated decisions: the real boundary lags one decision behind, publishing
// D1's frame index (1) as the unit boundary. A 2-member conflict learned at depth 2
// is still unit when we pop back to depth 1, so pop emits the ultimate (D2).
TEST_F(DbuctCdclEliminationGeneratorIntegrationTest, RotatedBoundaryKeepsConflictUnitOnPop) {
    depth = 1;
    aub.log_decision(&D1);
    sut.push_frame();
    depth = 2;
    aub.log_decision(&D2);  // rotate: ultimate=D2, penultimate=D1, boundary=1

    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({&D2, &D1})));
    sut.learn();  // members = [ultimate=D2, penultimate=D1]

    // depth-after-pop 1, boundary 1 -> 1 < 1 is false -> still unit -> emit D2.
    EXPECT_THAT(collect_elims(sut.pop_frame()), ElementsAre(&D2));
}

// Full end-to-end: three rotated decisions push the lagging boundary to 2. A
// conflict learned at depth 3 stays unit (emits) at depth 2, then -- once we pop
// ABOVE the boundary to depth 1 -- is armed as a watched clause instead. Committing
// the ultimate then forces the other watcher through normal propagation.
TEST_F(DbuctCdclEliminationGeneratorIntegrationTest, DeepBoundaryEmitsThenArmsThenPropagates) {
    depth = 1;
    aub.log_decision(&D1);
    sut.push_frame();
    depth = 2;
    aub.log_decision(&D2);
    sut.push_frame();
    depth = 3;
    aub.log_decision(&D3);  // rotate: ultimate=D3, penultimate=D2, boundary=2

    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({&D3, &D2})));
    sut.learn();  // members = [ultimate=D3, penultimate=D2], boundary 2

    // depth-after-pop 2, boundary 2 -> still unit -> emit D3.
    EXPECT_THAT(collect_elims(sut.pop_frame()), ElementsAre(&D3));
    // depth-after-pop 1, boundary 2 -> 1 < 2 -> no longer unit -> armed, no emit.
    EXPECT_THAT(collect_elims(sut.pop_frame()), IsEmpty());
    // Armed: committing the ultimate (D3) forces the penultimate (D2).
    EXPECT_THAT(collect_elims(sut.constrain(&D3)), ElementsAre(&D2));
}

}  // namespace
