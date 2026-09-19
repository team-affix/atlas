// pud_candidate_search: accept-first; choice-point, self-witness, unary query-advance, axiom_refuted.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <variant>
#include <vector>
#include "infrastructure/pud_candidate_search.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

struct MockResumeWitnessSearch {
    MOCK_METHOD(pud_witness_search_result, resume, (pud_witness_search_context&), ());
};

struct MockIsLeaf {
    MOCK_METHOD(bool, is_leaf, (const pud_rule_id*), ());
};

struct MockOrderedChildren {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_children, (const pud_rule_id*), ());
};

struct MockParent {
    MOCK_METHOD(const pud_rule_id*, parent, (const pud_rule_id*), ());
};

struct MockUnifyHead {
    MOCK_METHOD(bool, unify_head, (const pud_rule_id*), ());
};

using test_search_t = pud_candidate_search<NiceMock<MockResumeWitnessSearch>,
                                           NiceMock<MockIsLeaf>,
                                           NiceMock<MockOrderedChildren>,
                                           NiceMock<MockParent>,
                                           NiceMock<MockUnifyHead>>;

struct PudCandidateSearchTest : public ::testing::Test {
    PudCandidateSearchTest()
        : a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , c1_{pud_rule_id::inference{&a0_, 1, &a0_}}
        , search_(witness_, is_leaf_, children_, parent_, unify_) {}

    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    NiceMock<MockResumeWitnessSearch> witness_;
    NiceMock<MockIsLeaf> is_leaf_;
    NiceMock<MockOrderedChildren> children_;
    NiceMock<MockParent> parent_;
    NiceMock<MockUnifyHead> unify_;
    test_search_t search_;
};

TEST_F(PudCandidateSearchTest, AcceptsExistingChoicePoint) {
    pud_candidate_search_context ctx{
        &a0_,
        {{&c0_, &c0_}, {&c1_, &c1_}}};
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
}

TEST_F(PudCandidateSearchTest, AcceptsSelfWitnessingLeafCursor) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&a0_)).WillRepeatedly(Return(true));
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
}

TEST_F(PudCandidateSearchTest, TwoLiveOutgoingEdgesAreAChoicePoint) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    EXPECT_CALL(witness_, resume(_))
        .WillOnce([](pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        })
        .WillOnce([](pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}

TEST_F(PudCandidateSearchTest, OneLiveEdgeQueryAdvancesThenSelfWitnesses) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{}));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = edge.edge_root;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
}

TEST_F(PudCandidateSearchTest, NoLiveEdgesMeansAxiomRefuted) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(witness_, resume(_)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::failed{}}));
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudCandidateSearchTest, AfterOneLiveEdgeFailsScansRemainingOutgoingEdges) {
    pud_candidate_search_context ctx{&a0_, {{&c0_, &c0_}}};
    const pud_rule_id* expected = &c1_;
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    EXPECT_CALL(witness_, resume(_)).WillOnce([expected](pud_witness_search_context& edge) {
        EXPECT_EQ(edge.edge_root, expected);
        edge.current = expected;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}
