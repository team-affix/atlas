// pud_candidate_search: accept-first; choice-point, self-witness, unary query-advance, axiom_refuted.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <sstream>
#include <variant>
#include <vector>
#include "infrastructure/pud_candidate_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

struct MockResumeWitnessSearch {
    MOCK_METHOD(pud_witness_search_result, resume, (pud_query&, pud_witness_search_context&), ());
};

struct MockIsLeaf {
    MOCK_METHOD(bool, is_leaf, (const pud_rule_id*), ());
};

struct MockOrderedChildren {
    MOCK_METHOD(std::vector<const pud_rule_id*>, ordered_children, (const pud_rule_id*), ());
};

struct MockTryParent {
    MOCK_METHOD(const pud_rule_id*, try_parent, (const pud_rule_id*), ());
};

struct MockUnifyHead {
    MOCK_METHOD(bool, unify_head, (pud_query&, const pud_rule_id*), ());
};

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

using test_search_t = pud_candidate_search<NiceMock<MockResumeWitnessSearch>,
                                           NiceMock<MockIsLeaf>,
                                           NiceMock<MockOrderedChildren>,
                                           NiceMock<MockTryParent>,
                                           NiceMock<MockUnifyHead>,
                                           NiceMock<MockGetNode>>;

struct PudCandidateSearchTest : public ::testing::Test {
    PudCandidateSearchTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , c1_{pud_rule_id::inference{&a0_, 1, &a0_}}
        , dummy_{expr::var{9}}
        , advance_node_{{}, {&dummy_}, 1}
        , query_{interval_, &body_, {}, 1}
        , search_(witness_, is_leaf_, children_, try_parent_, unify_, get_node_) {
        ON_CALL(get_node_, get_node(_)).WillByDefault(ReturnRef(advance_node_));
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    expr dummy_;
    pud_db_node advance_node_;
    pud_query query_;
    NiceMock<MockResumeWitnessSearch> witness_;
    NiceMock<MockIsLeaf> is_leaf_;
    NiceMock<MockOrderedChildren> children_;
    NiceMock<MockTryParent> try_parent_;
    NiceMock<MockUnifyHead> unify_;
    NiceMock<MockGetNode> get_node_;
    test_search_t search_;
};

TEST_F(PudCandidateSearchTest, AcceptsExistingChoicePoint) {
    pud_candidate_search_context ctx{
        &a0_,
        {{&c0_, &c0_}, {&c1_, &c1_}}};
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
}

TEST_F(PudCandidateSearchTest, AcceptsSelfWitnessingLeafCursor) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
}

TEST_F(PudCandidateSearchTest, TwoLiveOutgoingEdgesAreAChoicePoint) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    EXPECT_CALL(witness_, resume(_, _))
        .WillOnce([](pud_query&, pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        })
        .WillOnce([](pud_query&, pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}

TEST_F(PudCandidateSearchTest, OneLiveEdgeQueryAdvancesThenSelfWitnesses) {
    pud_candidate_search_context ctx{&a0_, {}, {&body_}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{}));
    EXPECT_CALL(witness_, resume(_, _)).WillOnce([](pud_query&, pud_witness_search_context& edge) {
        edge.current = edge.edge_root;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
}

TEST_F(PudCandidateSearchTest, QueryAdvanceRewritesAddedBodyGoalsViaCallSite) {
    expr leftover{expr::var{2}};
    expr added{expr::var{3}};
    pud_db_node child_node{{}, {&added}, 1};
    ON_CALL(get_node_, get_node(&c0_)).WillByDefault(ReturnRef(child_node));
    pud_candidate_search_context ctx{&a0_, {}, {&body_, &leftover}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{}));
    EXPECT_CALL(witness_, resume(_, _)).WillOnce([](pud_query&, pud_witness_search_context& edge) {
        edge.current = edge.edge_root;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
    EXPECT_EQ(ctx.added_body_goals, (std::vector<const expr*>{&leftover, &added}));
}

TEST_F(PudCandidateSearchTest, NoLiveEdgesMeansAxiomRefuted) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(witness_, resume(_, _)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::failed{}}));
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudCandidateSearchTest, AfterOneLiveEdgeFailsScansRemainingOutgoingEdges) {
    pud_candidate_search_context ctx{&a0_, {{&c0_, &c0_}}};
    const pud_rule_id* expected = &c1_;
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    EXPECT_CALL(witness_, resume(_, _)).WillOnce([expected](pud_query&, pud_witness_search_context& edge) {
        EXPECT_EQ(edge.edge_root, expected);
        edge.current = expected;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}

TEST_F(PudCandidateSearchTest, QueryAdvanceRewritesEdgeRootWhenCurrentIsDeeper) {
    pud_rule_id g0{pud_rule_id::inference{&c0_, 0, &a0_}};
    pud_candidate_search_context ctx{&a0_, {{&c0_, &g0}}, {&body_}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(try_parent_, try_parent(&g0)).WillRepeatedly(Return(&c0_));
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
    ASSERT_EQ(ctx.live_edges.size(), 1u);
    EXPECT_EQ(ctx.live_edges[0].edge_root, &g0);
}

TEST_F(PudCandidateSearchTest, FillLiveEdgesStopsAtTwoOfThreeChildren) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_, &c2}));
    EXPECT_CALL(witness_, resume(_, _))
        .Times(2)
        .WillRepeatedly([](pud_query&, pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
    EXPECT_EQ(ctx.live_edges[0].edge_root, &c0_);
    EXPECT_EQ(ctx.live_edges[1].edge_root, &c1_);
}

TEST_F(PudCandidateSearchTest, LeafThatFailsUnifyIsAxiomRefuted) {
    pud_candidate_search_context ctx{&a0_, {}};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{}));
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudCandidateSearchTest, ResumeTwiceOnChoicePointStaysChoicePoint) {
    pud_candidate_search_context ctx{
        &a0_,
        {{&c0_, &c0_}, {&c1_, &c1_}}};
    for (int step = 0; step < 8; ++step) {
        const pud_candidate_search_result result = search_.resume(query_, ctx);
        EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(
            result.content));
        EXPECT_EQ(ctx.live_edges.size(), 2u);
        EXPECT_EQ(ctx.cursor, &a0_);
    }
}

TEST_F(PudCandidateSearchTest, StressUnaryChainAdvance) {
    // Unary descent must walk a spine of live edges to the leaf. After each
    // advance onto the witness itself, that edge is consumed so the next
    // fill_live_edges sees an empty set, not a stale self-edge plus the child.
    constexpr int k_depth = 16;
    std::vector<pud_rule_id> nodes;
    nodes.reserve(static_cast<size_t>(k_depth));
    nodes.push_back(pud_rule_id{pud_rule_id::axiom{0}});
    for (int idx = 1; idx < k_depth; ++idx)
        nodes.push_back(pud_rule_id{
            pud_rule_id::inference{&nodes[0], 0, &nodes[0]}});

    ON_CALL(is_leaf_, is_leaf(_)).WillByDefault([&nodes](const pud_rule_id* node) {
        return node == &nodes.back();
    });
    ON_CALL(unify_, unify_head(_, _)).WillByDefault([&nodes](pud_query&, const pud_rule_id* node) {
        return node == &nodes.back();
    });
    ON_CALL(children_, ordered_children(_)).WillByDefault(
        [&nodes](const pud_rule_id* node) {
            for (int idx = 0; idx + 1 < k_depth; ++idx) {
                if (node != &nodes[static_cast<size_t>(idx)])
                    continue;
                return std::vector<const pud_rule_id*>{&nodes[static_cast<size_t>(idx + 1)]};
            }
            return std::vector<const pud_rule_id*>{};
        });
    ON_CALL(witness_, resume(_, _)).WillByDefault(
        [](pud_query&, pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });

    pud_candidate_search_context ctx{&nodes.front(), {}, {&body_}};
    const pud_candidate_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &nodes.back());
    EXPECT_TRUE(ctx.live_edges.empty());
}

TEST_F(PudCandidateSearchTest, FuzzResumeOnFixedMockDag) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    pud_rule_id g0{pud_rule_id::inference{&c0_, 0, &a0_}};
    pud_rule_id g1{pud_rule_id::inference{&c1_, 0, &a0_}};
    const std::vector<const pud_rule_id*> children_of_root{&c0_, &c1_, &c2};
    ON_CALL(is_leaf_, is_leaf(_)).WillByDefault([&](const pud_rule_id* node) {
        return node == &g0 || node == &g1 || node == &c2;
    });
    ON_CALL(unify_, unify_head(_, _)).WillByDefault([&](pud_query&, const pud_rule_id* node) {
        return node == &g0 || node == &g1 || node == &c2;
    });
    ON_CALL(children_, ordered_children(_)).WillByDefault([&](const pud_rule_id* node) {
        if (node == &a0_)
            return children_of_root;
        if (node == &c0_)
            return std::vector<const pud_rule_id*>{&g0};
        if (node == &c1_)
            return std::vector<const pud_rule_id*>{&g1};
        return std::vector<const pud_rule_id*>{};
    });
    ON_CALL(try_parent_, try_parent(_)).WillByDefault([&](const pud_rule_id* node) -> const pud_rule_id* {
        if (node == &g0)
            return &c0_;
        if (node == &g1)
            return &c1_;
        if (node == &c0_ || node == &c1_ || node == &c2)
            return &a0_;
        return nullptr;
    });
    ON_CALL(witness_, resume(_, _)).WillByDefault(
        [](pud_query&, pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });

    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> seed_dist(0, 2);
    std::ostringstream log;
    for (int step = 0; step < 80; ++step) {
        pud_candidate_search_context ctx{&a0_, {}, {&body_, &body_, &body_}};
        const int preset = seed_dist(rng);
        log << step << ':' << preset << ' ';
        if (preset == 1)
            ctx.live_edges.push_back({&c0_, &c0_});
        if (preset == 2) {
            ctx.live_edges.push_back({&c0_, &c0_});
            ctx.live_edges.push_back({&c1_, &c1_});
        }
        const pud_candidate_search_result result = search_.resume(query_, ctx);
        const bool choice =
            std::holds_alternative<pud_candidate_search_result::choice_point>(result.content);
        const bool self =
            std::holds_alternative<pud_candidate_search_result::self_witness>(result.content);
        const bool refuted =
            std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content);
        EXPECT_TRUE(choice || self || refuted) << "seed " << k_seed << " log " << log.str();
        if (choice)
            EXPECT_GE(ctx.live_edges.size(), 2u) << "seed " << k_seed << " log " << log.str();
        if (self) {
            EXPECT_TRUE(ctx.cursor == &g0 || ctx.cursor == &g1 || ctx.cursor == &c2)
                << "seed " << k_seed << " log " << log.str();
        }
    }
}
