#include <cstdint>
#include <deque>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/representative_change_logger.hpp"
#include "value_objects/expr.hpp"

namespace {

struct MockEquate {
    MOCK_METHOD(void, equate, (uint32_t, uint32_t), ());
};

struct MockInstantiate {
    MOCK_METHOD(void, instantiate, (uint32_t, uint32_t, (const std::vector<const expr*>&)), ());
};

struct MockPushBackChangedRep {
    MOCK_METHOD(void, push_back, (uint32_t), ());
};

using test_rep_change_logger_t = representative_change_logger<
    MockEquate,
    MockInstantiate,
    MockPushBackChangedRep>;

struct RepresentativeChangeLoggerTest : public ::testing::Test {
    ::testing::NiceMock<MockEquate> equate;
    ::testing::NiceMock<MockInstantiate> instantiate;
    ::testing::NiceMock<MockPushBackChangedRep> push_back;
    test_rep_change_logger_t logger{equate, instantiate, push_back};

    std::deque<expr> owned;
    const expr* functor_node() {
        owned.push_back(expr{expr::functor{1u, {}}});
        return &owned.back();
    }
};

struct three_phase_recorder {
    std::vector<int> phases;

    void on_equate() { phases.push_back(0); }
    void on_instantiate() { phases.push_back(1); }
    void on_push() { phases.push_back(2); }
};

struct rep_collector {
    std::vector<uint32_t> values;

    void collect(uint32_t rep) { values.push_back(rep); }
};

TEST_F(RepresentativeChangeLoggerTest, EquateForwardsArgumentsInOrder) {
    EXPECT_CALL(equate, equate(3u, 7u)).Times(1);
    EXPECT_CALL(push_back, push_back(7u)).Times(1);
    logger.equate(3u, 7u);
}

TEST_F(RepresentativeChangeLoggerTest, InstantiateForwardsRepFunctorIdAndArgs) {
    std::vector<const expr*> args;
    EXPECT_CALL(instantiate, instantiate(5u, 1u, testing::Ref(args))).Times(1);
    EXPECT_CALL(push_back, push_back(5u)).Times(1);
    logger.instantiate(5u, 1u, args);
}

TEST_F(RepresentativeChangeLoggerTest, EquateDoesNotCallInstantiate) {
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    test_rep_change_logger_t strict_logger{equate, strict_instantiate, push_back};
    EXPECT_CALL(equate, equate(1u, 2u));
    EXPECT_CALL(push_back, push_back(2u));
    strict_logger.equate(1u, 2u);
}

TEST_F(RepresentativeChangeLoggerTest, InstantiateDoesNotCallEquate) {
    ::testing::StrictMock<MockEquate> strict_equate;
    test_rep_change_logger_t strict_logger{strict_equate, instantiate, push_back};
    std::vector<const expr*> args;
    EXPECT_CALL(instantiate, instantiate(4u, 1u, testing::Ref(args)));
    EXPECT_CALL(push_back, push_back(4u));
    strict_logger.instantiate(4u, 1u, args);
}

TEST_F(RepresentativeChangeLoggerTest, ForwardingHappensBeforePushBack) {
    three_phase_recorder recorder;
    EXPECT_CALL(equate, equate(1u, 2u))
        .WillOnce(testing::Invoke(&recorder, &three_phase_recorder::on_equate));
    EXPECT_CALL(push_back, push_back(2u))
        .WillOnce(testing::Invoke(&recorder, &three_phase_recorder::on_push));
    logger.equate(1u, 2u);
    EXPECT_EQ(recorder.phases.size(), 2u);
    EXPECT_EQ(recorder.phases[0], 0);
    EXPECT_EQ(recorder.phases[1], 2);
}

TEST_F(RepresentativeChangeLoggerTest, InstantiateForwardingHappensBeforePushBack) {
    three_phase_recorder recorder;
    std::vector<const expr*> args;
    EXPECT_CALL(instantiate, instantiate(9u, 1u, testing::Ref(args)))
        .WillOnce(testing::Invoke(&recorder, &three_phase_recorder::on_instantiate));
    EXPECT_CALL(push_back, push_back(9u))
        .WillOnce(testing::Invoke(&recorder, &three_phase_recorder::on_push));
    logger.instantiate(9u, 1u, args);
    EXPECT_EQ(recorder.phases.size(), 2u);
    EXPECT_EQ(recorder.phases[0], 1);
    EXPECT_EQ(recorder.phases[1], 2);
}

TEST_F(RepresentativeChangeLoggerTest, EquateReversedArgumentOrderStillPushesLarger) {
    EXPECT_CALL(equate, equate(7u, 3u));
    EXPECT_CALL(push_back, push_back(7u));
    logger.equate(7u, 3u);
}

TEST_F(RepresentativeChangeLoggerTest, EquateIndexZeroPushesOne) {
    EXPECT_CALL(equate, equate(0u, 1u));
    EXPECT_CALL(push_back, push_back(1u));
    logger.equate(0u, 1u);
}

TEST_F(RepresentativeChangeLoggerTest, EquateNearUint32MaxPushesLargerWithoutOverflow) {
    const uint32_t high = UINT32_MAX;
    const uint32_t lower_high = UINT32_MAX - 1u;
    EXPECT_CALL(equate, equate(lower_high, high));
    EXPECT_CALL(push_back, push_back(high));
    logger.equate(lower_high, high);
}

TEST_F(RepresentativeChangeLoggerTest, InstantiateZeroPushesZero) {
    std::vector<const expr*> args;
    EXPECT_CALL(instantiate, instantiate(0u, 1u, testing::Ref(args)));
    EXPECT_CALL(push_back, push_back(0u));
    logger.instantiate(0u, 1u, args);
}

TEST_F(RepresentativeChangeLoggerTest, EachCallPushesExactlyOnce) {
    std::vector<const expr*> args;
    EXPECT_CALL(equate, equate(testing::_, testing::_)).Times(2);
    EXPECT_CALL(instantiate, instantiate(testing::_, 1u, testing::Ref(args))).Times(1);
    EXPECT_CALL(push_back, push_back(testing::_)).Times(3);
    logger.equate(1u, 2u);
    logger.instantiate(5u, 1u, args);
    logger.equate(4u, 8u);
}

TEST_F(RepresentativeChangeLoggerTest, MixedSequencePushesInCallOrder) {
    rep_collector pushed;
    ON_CALL(push_back, push_back(testing::_))
        .WillByDefault(testing::Invoke(&pushed, &rep_collector::collect));
    EXPECT_CALL(equate, equate(testing::_, testing::_)).Times(2);
    EXPECT_CALL(instantiate, instantiate(testing::_, testing::_, testing::_)).Times(1);
    std::vector<const expr*> args;
    logger.equate(1u, 5u);
    logger.instantiate(3u, 1u, args);
    logger.equate(2u, 4u);
    EXPECT_EQ(pushed.values, std::vector<uint32_t>({5u, 3u, 4u}));
}

TEST_F(RepresentativeChangeLoggerTest, RepeatedSameRepresentativePushesEachTime) {
    EXPECT_CALL(equate, equate(2u, 7u)).Times(2);
    EXPECT_CALL(push_back, push_back(7u)).Times(2);
    logger.equate(2u, 7u);
    logger.equate(2u, 7u);
}

TEST_F(RepresentativeChangeLoggerTest, NoCallsMeansNoPush) {
    EXPECT_CALL(push_back, push_back(testing::_)).Times(0);
}

TEST_F(RepresentativeChangeLoggerTest, EquateThrowSkipsPush) {
    EXPECT_CALL(equate, equate(1u, 2u)).WillOnce(testing::Throw(std::runtime_error("fail")));
    EXPECT_CALL(push_back, push_back(testing::_)).Times(0);
    EXPECT_THROW(logger.equate(1u, 2u), std::runtime_error);
}

TEST_F(RepresentativeChangeLoggerTest, InstantiateThrowSkipsPush) {
    std::vector<const expr*> args;
    EXPECT_CALL(instantiate, instantiate(1u, 1u, testing::Ref(args)))
        .WillOnce(testing::Throw(std::runtime_error("fail")));
    EXPECT_CALL(push_back, push_back(testing::_)).Times(0);
    EXPECT_THROW(logger.instantiate(1u, 1u, args), std::runtime_error);
}

TEST_F(RepresentativeChangeLoggerTest, SeparateSinksDoNotShareEntries) {
    ::testing::NiceMock<MockPushBackChangedRep> other_push;
    test_rep_change_logger_t other_logger{equate, instantiate, other_push};
    rep_collector first_sink;
    rep_collector second_sink;
    ON_CALL(push_back, push_back(testing::_))
        .WillByDefault(testing::Invoke(&first_sink, &rep_collector::collect));
    ON_CALL(other_push, push_back(testing::_))
        .WillByDefault(testing::Invoke(&second_sink, &rep_collector::collect));
    EXPECT_CALL(equate, equate(1u, 2u));
    EXPECT_CALL(equate, equate(3u, 4u));
    logger.equate(1u, 2u);
    other_logger.equate(3u, 4u);
    EXPECT_EQ(first_sink.values, std::vector<uint32_t>({2u}));
    EXPECT_EQ(second_sink.values, std::vector<uint32_t>({4u}));
}

TEST_F(RepresentativeChangeLoggerTest, SharedSinkInterleavesInCallOrder) {
    rep_collector shared;
    ::testing::NiceMock<MockPushBackChangedRep> shared_push;
    test_rep_change_logger_t first{equate, instantiate, shared_push};
    ::testing::NiceMock<MockEquate> other_equate;
    test_rep_change_logger_t second{other_equate, instantiate, shared_push};
    ON_CALL(shared_push, push_back(testing::_))
        .WillByDefault(testing::Invoke(&shared, &rep_collector::collect));
    EXPECT_CALL(equate, equate(1u, 9u));
    EXPECT_CALL(other_equate, equate(2u, 8u));
    first.equate(1u, 9u);
    second.equate(2u, 8u);
    EXPECT_EQ(shared.values, std::vector<uint32_t>({9u, 8u}));
}

struct push_only_sink {
    void push_back(uint32_t) {}
};

struct equate_only {
    void equate(uint32_t, uint32_t) {}
};

struct instantiate_only {
    void instantiate(uint32_t, uint32_t, const std::vector<const expr*>&) {}
};

TEST(RepresentativeChangeLoggerCompileTest, LoggerDoesNotRequireReadCapabilitiesOnDependencies) {
    equate_only equate_dep;
    instantiate_only instantiate_dep;
    push_only_sink sink;
    representative_change_logger<equate_only, instantiate_only, push_only_sink> sut{
        equate_dep, instantiate_dep, sink};
    (void)sut;
}

} // namespace
