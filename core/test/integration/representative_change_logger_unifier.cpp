#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "representative_change_logger_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

uint32_t gkey(globalizer& globalize, uint32_t frame, uint32_t local) {
    return globalize.globalize(frame, local);
}

struct RepChangeLoggerUnifierIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_logged_unifier_bind_map_stack stack;

    const expr* v(uint32_t local) { return stack.bind_backing.pool.make_var(local); }
    const expr* atom(const char* name) {
        return stack.bind_backing.pool.make_functor(functors.id(name), {});
    }
};

} // namespace

TEST_F(RepChangeLoggerUnifierIntegrationTest, TwoFreshVariablesLogLargerIndex) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(3), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({3u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, FreshVariableWithFunctorLogsVariable) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(2), 0), pud_fe(atom("a"), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({2u}));
}

// Below: empty changed_rep sinks mean the unifier never called equate/instantiate.
// That is not "the logger ignores same-class equate" — same-class equate through the
// bind map is a contract violation (see bind-map integration). These tests assert the
// unifier only mutates when a binding would actually change state.

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorOnIdenticalGroundTerms) {
    const expr* ground = atom("a");
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(ground, 0), pud_fe(ground, 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorOnTermUnifyWithItself) {
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(fx, 0), pud_fe(fx, 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorOnTopLevelFunctorMismatch) {
    EXPECT_FALSE(stack.pud_unifier.unify(pud_fe(atom("a"), 0), pud_fe(atom("b"), 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, NonRepresentativeMemberWithFunctorLogsClassRepresentative) {
    stack.bind_map.equate(4u, 6u);
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(6), 0), pud_fe(atom("a"), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({4u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, TwoNonRepresentativeMembersLogLargerRepresentative) {
    stack.bind_map.equate(2u, 5u);
    stack.bind_map.equate(7u, 9u);
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(5), 0), pud_fe(v(9), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({7u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, AfterCallee105MergedWithCaller3BindingThrough106Logs3) {
    stack.rep_logger.equate(106u, 105u);
    stack.rep_logger.equate(105u, 3u);
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(stack.bind_backing.pool.make_var(106u), 0), pud_fe(atom("a"), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({3u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorWhenMergedVarsResolveToSameSkeleton) {
    stack.bind_map.equate(1u, 2u);
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(1), 0), pud_fe(v(2), 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierMutatesOnceForDuplicateFunctorArguments) {
    const uint32_t pid = functors.id("p");
    const expr* left = stack.bind_backing.pool.make_functor(pid, {v(0), v(0)});
    const expr* right = stack.bind_backing.pool.make_functor(pid, {atom("a"), atom("a")});
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({0u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierMutatesOnceForSwappedPairAfterFirstMerge) {
    const uint32_t pid = functors.id("p");
    const expr* left = stack.bind_backing.pool.make_functor(pid, {v(0), v(1)});
    const expr* right = stack.bind_backing.pool.make_functor(pid, {v(1), v(0)});
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    EXPECT_EQ(stack.sink.changed_reps.size(), 1u);
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorWhenVariableAlreadyBoundToRhs) {
    const expr* ground = atom("a");
    stack.bind_map.instantiate(2u, functors.id("a"), {});
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(2), 0), pud_fe(ground, 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorWhenBothVariablesShareBinding) {
    const expr* ground = atom("a");
    stack.bind_map.instantiate(1u, functors.id("a"), {});
    stack.bind_map.instantiate(4u, functors.id("a"), {});
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(1), 0), pud_fe(v(4), 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, UnifierSkipsMutatorOnOccursCheckFailure) {
    const expr* loop = stack.bind_backing.pool.make_functor(functors.id("l"), {v(0)});
    EXPECT_FALSE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(loop, 0)));
    EXPECT_TRUE(stack.sink.changed_reps.empty());
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, PartialSuccessBeforeFailureKeepsEarlierLogEntries) {
    const uint32_t pid = functors.id("p");
    const expr* left = stack.bind_backing.pool.make_functor(pid, {v(0), atom("a")});
    const expr* right = stack.bind_backing.pool.make_functor(pid, {atom("b"), atom("c")});
    EXPECT_FALSE(stack.pud_unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({0u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, DeepNestedFailureLogsOnlyBindingsBeforeFailure) {
    const uint32_t nest = functors.id("nest");
    const expr* deep_left = stack.bind_backing.pool.make_functor(
        nest, {v(0), atom("a")});
    const expr* deep_right = stack.bind_backing.pool.make_functor(
        nest, {atom("b"), atom("c")});
    EXPECT_FALSE(stack.pud_unifier.unify(pud_fe(deep_left, 0), pud_fe(deep_right, 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({0u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, SameLocalDifferentOffsetsLogDifferentRepresentatives) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(0), 1)));
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 2), pud_fe(v(0), 3)));
    const uint32_t logged_first = gkey(stack.globalize, 1, 0);
    const uint32_t logged_second = gkey(stack.globalize, 3, 0);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({logged_first, logged_second}));
    EXPECT_NE(logged_first, logged_second);
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, SameSkeletonDifferentFramesLogsLargerGlobal) {
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("f"), {v(0)});
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(fx, 0), pud_fe(fx, 10)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({10u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, CallerCalleeMergeLogsCalleeRepresentative) {
    const uint32_t callee_rep = gkey(stack.globalize, 0, 2);
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 1), pud_fe(v(2), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({callee_rep}));
    pud_logged_unifier_bind_map_stack reversed;
    const uint32_t callee_rep_reversed = gkey(reversed.globalize, 0, 2);
    EXPECT_TRUE(reversed.pud_unifier.unify(pud_fe(reversed.bind_backing.pool.make_var(2u), 0),
        pud_fe(reversed.bind_backing.pool.make_var(0u), 1)));
    EXPECT_EQ(reversed.sink.changed_reps, std::vector<uint32_t>({callee_rep_reversed}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, ReplayFromLogMatchesOriginalResolve) {
    const uint32_t pid = functors.id("pair");
    const expr* left = stack.bind_backing.pool.make_functor(pid, {v(0), v(1)});
    const expr* right = stack.bind_backing.pool.make_functor(pid, {atom("a"), v(2)});
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(left, 0), pud_fe(right, 0)));
    const rep_change_trace full = trace_from_sink(stack.sink);
    logged_ubs_bind_map_stack replay;
    replay_rep_changes(replay, stack.bind_map, full);
    EXPECT_TRUE(resolve_maps_match(replay.bind_map, stack.bind_map, 8u));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, ReplayAfterMergeThenInstantiate) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(4), 0)));
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(atom("a"), 0)));
    const rep_change_trace full = trace_from_sink(stack.sink);
    logged_ubs_bind_map_stack replay;
    replay_rep_changes(replay, stack.bind_map, full);
    EXPECT_TRUE(resolve_maps_match(replay.bind_map, stack.bind_map, 8u));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, ReplayWhenRepresentativeLaterStopsBeingRepresentative) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(5), 0)));
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(2), 0)));
    const rep_change_trace full = trace_from_sink(stack.sink);
    logged_ubs_bind_map_stack replay;
    replay_rep_changes(replay, stack.bind_map, full);
    EXPECT_TRUE(resolve_maps_match(replay.bind_map, stack.bind_map, 8u));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, DedupedReplayMatchesFullReplay) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(3), 0)));
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(2), 0)));
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(atom("z"), 0)));
    const rep_change_trace full = trace_from_sink(stack.sink);
    const rep_change_trace deduped = dedupe_rep_change_trace(full);
    logged_ubs_bind_map_stack full_replay;
    logged_ubs_bind_map_stack deduped_replay;
    replay_rep_changes(full_replay, stack.bind_map, full);
    replay_rep_changes(deduped_replay, stack.bind_map, deduped);
    EXPECT_TRUE(resolve_maps_match(full_replay.bind_map, deduped_replay.bind_map, 8u));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, ReplaySegmentReproducesOnlyThatSegmentsChanges) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(1), 0)));
    const rep_change_trace first = trace_from_sink(stack.sink);
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(2), 0), pud_fe(atom("a"), 0)));
    logged_ubs_bind_map_stack baseline;
    baseline.rep_logger.equate(0u, 1u);
    logged_ubs_bind_map_stack segment_replay;
    replay_rep_changes(segment_replay, stack.bind_map, first);
    EXPECT_TRUE(resolve_maps_match(segment_replay.bind_map, baseline.bind_map, 8u));
    EXPECT_NE(segment_replay.bind_map.resolve(2u), baseline.bind_map.resolve(2u));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, ClearingSinkBetweenUnifiesKeepsOnlySecondCall) {
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(1), 0)));
    stack.sink.clear();
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(2), 0), pud_fe(atom("a"), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({2u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, IndependentSinksDoNotMix) {
    pud_logged_unifier_bind_map_stack other;
    EXPECT_TRUE(stack.pud_unifier.unify(pud_fe(v(0), 0), pud_fe(v(1), 0)));
    EXPECT_TRUE(other.pud_unifier.unify(pud_fe(other.bind_backing.pool.make_var(2u), 0),
        pud_fe(other.bind_backing.pool.make_functor(functors.id("a"), {}), 0)));
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({1u}));
    EXPECT_EQ(other.sink.changed_reps, std::vector<uint32_t>({2u}));
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, RandomUnifyLogsPreCallRepresentativeForEachChangedClass) {
    std::mt19937 rng(42u);
    for (int trial = 0; trial < 25; ++trial) {
        pud_logged_unifier_bind_map_stack trial_stack;
        const uint32_t pid = functors.id("rnd");
        const uint32_t left_var = static_cast<uint32_t>(rng() % 4u);
        const uint32_t right_kind = rng() % 3u;
        const expr* left = trial_stack.bind_backing.pool.make_functor(pid, {trial_stack.bind_backing.pool.make_var(left_var)});
        const expr* right = right_kind == 0
            ? trial_stack.bind_backing.pool.make_functor(pid, {trial_stack.bind_backing.pool.make_var(static_cast<uint32_t>(rng() % 4u))})
            : trial_stack.bind_backing.pool.make_functor(functors.id(right_kind == 1 ? "a" : "b"), {});
        std::vector<const expr*> resolve_before;
        for (uint32_t var = 0; var < 8u; ++var)
            resolve_before.push_back(trial_stack.bind_map.resolve(var));
        const size_t log_start = trial_stack.sink.changed_reps.size();
        trial_stack.pud_unifier.unify(pud_fe(left, 0), pud_fe(right, 0));
        for (size_t entry = log_start; entry < trial_stack.sink.changed_reps.size(); ++entry) {
            const uint32_t logged = trial_stack.sink.changed_reps[entry];
            EXPECT_NE(resolve_before[logged], trial_stack.bind_map.resolve(logged));
        }
        for (uint32_t var = 0; var < 8u; ++var) {
            if (resolve_before[var] == trial_stack.bind_map.resolve(var))
                continue;
            const uint32_t expected = ubs_rep_index(resolve_before[var]);
            const bool found = std::find(
                trial_stack.sink.changed_reps.begin() + static_cast<std::ptrdiff_t>(log_start),
                trial_stack.sink.changed_reps.end(),
                expected)
                != trial_stack.sink.changed_reps.end();
            EXPECT_TRUE(found);
        }
    }
}

TEST_F(RepChangeLoggerUnifierIntegrationTest, RandomSequencesReplayMatchOriginalResolve) {
    std::mt19937 rng(7u);
    for (int trial = 0; trial < 20; ++trial) {
        pud_logged_unifier_bind_map_stack trial_stack;
        const uint32_t pid = functors.id("replay");
        for (int step = 0; step < 5; ++step) {
            const uint32_t left_var = static_cast<uint32_t>(rng() % 4u);
            const uint32_t right_var = static_cast<uint32_t>(rng() % 4u);
            const expr* left = trial_stack.bind_backing.pool.make_functor(pid, {trial_stack.bind_backing.pool.make_var(left_var)});
            const expr* right = trial_stack.bind_backing.pool.make_functor(
                pid, {trial_stack.bind_backing.pool.make_var(right_var)});
            trial_stack.pud_unifier.unify(pud_fe(left, 0), pud_fe(right, 0));
        }
        const rep_change_trace full = trace_from_sink(trial_stack.sink);
        logged_ubs_bind_map_stack replay;
        replay_rep_changes(replay, trial_stack.bind_map, full);
        EXPECT_TRUE(resolve_maps_match(replay.bind_map, trial_stack.bind_map, 12u));
    }
}
