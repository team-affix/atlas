// pud_invalidation_router: dead leaf resumes witness search, then candidate search on fail.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include "infrastructure/pud_invalidation_router.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockGetWatchers {
    MOCK_METHOD(const std::unordered_set<pud_query*>&, get, (const pud_rule_id*), ());
};

struct MockResumeWitnessSearch {
    MOCK_METHOD(pud_witness_search_result, resume, (pud_witness_search_context&), ());
};

struct MockResumeCandidateSearch {
    MOCK_METHOD(pud_candidate_search_result, resume, (pud_candidate_search_context&), ());
};

struct MockBindQuery {
    MOCK_METHOD(void, bind_query, (pud_query&, uint32_t), ());
};

using test_router_t = pud_invalidation_router<
    NiceMock<MockGetWatchers>,
    NiceMock<MockResumeWitnessSearch>,
    NiceMock<MockResumeCandidateSearch>,
    NiceMock<MockBindQuery>>;

struct PudInvalidationRouterTest : public ::testing::Test {
    PudInvalidationRouterTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , axiom_{pud_rule_id::axiom{0}}
        , dead_{pud_rule_id::inference{&axiom_, 0, &axiom_}}
        , sibling_{pud_rule_id::inference{&axiom_, 1, &axiom_}}
        , router_(watchers_, witness_, candidate_, bind_query_) {}

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id axiom_;
    pud_rule_id dead_;
    pud_rule_id sibling_;
    NiceMock<MockGetWatchers> watchers_;
    NiceMock<MockResumeWitnessSearch> witness_;
    NiceMock<MockResumeCandidateSearch> candidate_;
    NiceMock<MockBindQuery> bind_query_;
    test_router_t router_;
};

TEST_F(PudInvalidationRouterTest, SuccessfulWitnessResumeDoesNotResumeCandidate) {
    pud_query query{interval_, &body_, {
        pud_candidate_search_context{&axiom_, {{&dead_, &dead_}}}}};
    std::unordered_set<pud_query*> watching{&query};
    EXPECT_CALL(watchers_, get(&dead_)).WillOnce(ReturnRef(watching));
    EXPECT_CALL(witness_, resume(_)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::found{}}));
    EXPECT_CALL(candidate_, resume(_)).Times(0);
    router_.invalidate_leaf(&dead_);
}

TEST_F(PudInvalidationRouterTest, FailedWitnessResumeResumesCandidateContext) {
    pud_query query{interval_, &body_, {
        pud_candidate_search_context{&axiom_, {{&dead_, &dead_}, {&sibling_, &sibling_}}}}};
    std::unordered_set<pud_query*> watching{&query};
    EXPECT_CALL(watchers_, get(&dead_)).WillOnce(ReturnRef(watching));
    EXPECT_CALL(witness_, resume(_)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::failed{}}));
    EXPECT_CALL(candidate_, resume(_)).WillOnce(Return(
        pud_candidate_search_result{pud_candidate_search_result::choice_point{}}));
    router_.invalidate_leaf(&dead_);
}

TEST_F(PudInvalidationRouterTest, SelfWitnessCursorResumesCandidateSearch) {
    pud_query query{interval_, &body_, {
        pud_candidate_search_context{&dead_, {}}}};
    std::unordered_set<pud_query*> watching{&query};
    EXPECT_CALL(watchers_, get(&dead_)).WillOnce(ReturnRef(watching));
    EXPECT_CALL(candidate_, resume(_)).WillOnce(Return(
        pud_candidate_search_result{pud_candidate_search_result::axiom_refuted{}}));
    router_.invalidate_leaf(&dead_);
}
