#include <cstdint>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "representative_change_logger_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

struct RepChangeLoggerBindMapIntegrationTest : public ::testing::Test {
    logged_ubs_bind_map_stack stack;

    const expr* var(uint32_t index) {
        return stack.bind_backing.pool.make_var(index);
    }

    const expr* atom() {
        return stack.bind_backing.pool.make_functor(100u, {});
    }
};

} // namespace

TEST_F(RepChangeLoggerBindMapIntegrationTest, EquateLogsLargerRepBeforeMerge) {
    const uint32_t rep_before = logged_global_rep_index(stack.bind_map, 7u);
    EXPECT_EQ(rep_before, 7u);
    stack.rep_logger.equate(3u, 7u);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({7u}));
    EXPECT_EQ(logged_global_rep_index(stack.bind_map, 7u), 3u);
    EXPECT_EQ(logged_global_rep_index(stack.bind_map, 3u), 3u);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, EquateMergesLargerClassOntoSmallerRepresentative) {
    stack.rep_logger.equate(5u, 9u);
    EXPECT_EQ(ubs_rep_index(stack.bind_map.resolve(9u)), 5u);
    EXPECT_EQ(ubs_rep_index(stack.bind_map.resolve(5u)), 5u);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, UnchangedClassMembersStillResolveAsBefore) {
    const expr* before_two = stack.bind_map.resolve(2u);
    stack.rep_logger.equate(8u, 9u);
    EXPECT_EQ(stack.bind_map.resolve(2u), before_two);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, EveryMemberOfLoggedClassResolvesToNewRepresentative) {
    stack.rep_logger.equate(4u, 6u);
    stack.rep_logger.equate(4u, 5u);
    const expr* rep_node = stack.bind_map.resolve(4u);
    EXPECT_EQ(stack.bind_map.resolve(6u), rep_node);
    EXPECT_EQ(stack.bind_map.resolve(5u), rep_node);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, LoggedRepresentativeWasRealClassMemberBeforeCall) {
    const uint32_t logged = 11u;
    stack.rep_logger.equate(3u, logged);
    EXPECT_EQ(stack.sink.changed_reps.back(), logged);
    EXPECT_EQ(ubs_rep_index(stack.bind_map.resolve(logged)), 3u);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, DecreasingIndexMergeSequenceLogsEachPreviousMinimum) {
    stack.rep_logger.equate(0u, 4u);
    stack.rep_logger.equate(0u, 3u);
    stack.rep_logger.equate(0u, 2u);
    stack.rep_logger.equate(0u, 1u);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({4u, 3u, 2u, 1u}));
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, IncreasingIndexMergeSequenceLogsEachNewLargerIndex) {
    stack.rep_logger.equate(1u, 2u);
    stack.rep_logger.equate(1u, 3u);
    stack.rep_logger.equate(1u, 4u);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({2u, 3u, 4u}));
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, SurvivorRootLargerRepClassStillLogsLargerRepresentative) {
    stack.bind_map.equate(1u, 2u);
    stack.bind_map.equate(1u, 3u);
    stack.sink.clear();
    stack.rep_logger.equate(4u, 9u);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({9u}));
    EXPECT_EQ(ubs_rep_index(stack.bind_map.resolve(9u)), 4u);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, LoserRootWithSmallerRepresentativeStillLogsCorrectly) {
    stack.bind_map.equate(10u, 11u);
    stack.sink.clear();
    stack.rep_logger.equate(9u, 11u);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({11u}));
    EXPECT_EQ(ubs_rep_index(stack.bind_map.resolve(11u)), 9u);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, InstantiateThroughNonRepresentativeLogsCallerRep) {
    stack.rep_logger.equate(6u, 8u);
    stack.sink.clear();
    const uint32_t rep_before = logged_global_rep_index(stack.bind_map, 8u);
    const expr* ground = atom();
    stack.rep_logger.instantiate(8u, ground);
    EXPECT_EQ(rep_before, 6u);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({8u}));
    EXPECT_EQ(stack.bind_map.resolve(8u), ground);
    EXPECT_EQ(stack.bind_map.resolve(6u), ground);
}

TEST_F(RepChangeLoggerBindMapIntegrationTest, InstantiateLogsRepresentativeAndBindsFunctor) {
    const expr* ground = atom();
    stack.rep_logger.instantiate(5u, ground);
    EXPECT_EQ(stack.sink.changed_reps, std::vector<uint32_t>({5u}));
    EXPECT_EQ(stack.bind_map.resolve(5u), ground);
}

#ifdef DEBUG
// Same-class equate is not a supported "no log" case: callers must not equate two
// variables already in one class. The bind map rejects it; the logger does not swallow it.
TEST_F(RepChangeLoggerBindMapIntegrationTest, SameRootEquateTripsBindMapAssertion) {
    stack.rep_logger.equate(2u, 5u);
    const size_t before = stack.sink.changed_reps.size();
    EXPECT_THROW(stack.rep_logger.equate(2u, 5u), std::logic_error);
    EXPECT_EQ(stack.sink.changed_reps.size(), before);
}
#endif
