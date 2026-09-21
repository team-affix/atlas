// pud_candidate_search: accept-first; choice-point, self-witness, unary query-advance, axiom_refuted.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <variant>
#include <vector>
#include "infrastructure/pud_candidate_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

using children_set_t = std::set<const pud_rule_id*>;
using children_opt_t = std::optional<children_set_t>;

struct MockResumeWitnessSearch {
    MOCK_METHOD(pud_witness_search_result, resume, (pud_witness_search_context&), ());
};

struct MockGetChildren {
    MOCK_METHOD(children_opt_t, get, (const pud_rule_id*), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_rule_id*, get, (const pud_rule_id*), ());
};

struct MockUnifyHead {
    MOCK_METHOD(bool, unify_head, (pud_candidate_search_context&, const pud_rule_id*), ());
};

struct MockGetAddedBodyGoals {
    MOCK_METHOD(const std::vector<const expr*>&, get, (const pud_rule_id*), ());
};

using test_search_t = pud_candidate_search<NiceMock<MockResumeWitnessSearch>,
                                           NiceMock<MockGetChildren>,
                                           NiceMock<MockGetParent>,
                                           NiceMock<MockUnifyHead>,
                                           NiceMock<MockGetAddedBodyGoals>>;

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
        , advance_goals_{&dummy_}
        , search_(witness_, children_, get_parent_, unify_, get_added_body_goals_) {
        ON_CALL(get_added_body_goals_, get(_)).WillByDefault(ReturnRef(advance_goals_));
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    expr dummy_;
    std::vector<const expr*> advance_goals_;

    pud_witness_search_context make_edge(const pud_rule_id* edge_root,
                                         const pud_rule_id* current) {
        return pud_witness_search_context{interval_, &body_, 1, edge_root, current};
    }

    pud_candidate_search_context make_ctx(
            const pud_rule_id* cursor,
            std::vector<pud_witness_search_context> live_edges,
            std::vector<const expr*> added_body_goals) {
        return pud_candidate_search_context{
            interval_, &body_, 1, cursor, std::move(live_edges), std::move(added_body_goals)};
    }

    NiceMock<MockResumeWitnessSearch> witness_;
    NiceMock<MockGetChildren> children_;
    NiceMock<MockGetParent> get_parent_;
    NiceMock<MockUnifyHead> unify_;
    NiceMock<MockGetAddedBodyGoals> get_added_body_goals_;
    test_search_t search_;
};

TEST_F(PudCandidateSearchTest, AcceptsExistingChoicePoint) {
    pud_candidate_search_context ctx = make_ctx(
        &a0_, {make_edge(&c0_, &c0_), make_edge(&c1_, &c1_)}, {});
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
}

TEST_F(PudCandidateSearchTest, AcceptsSelfWitnessingLeafCursor) {
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
}

TEST_F(PudCandidateSearchTest, TwoLiveOutgoingEdgesAreAChoicePoint) {
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
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
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {&body_});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = edge.edge_root;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
}

TEST_F(PudCandidateSearchTest, QueryAdvanceRewritesAddedBodyGoalsViaCallSite) {
    expr leftover{expr::var{2}};
    expr added{expr::var{3}};
    std::vector<const expr*> child_goals{&added};
    ON_CALL(get_added_body_goals_, get(&c0_)).WillByDefault(ReturnRef(child_goals));
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {&body_, &leftover});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = edge.edge_root;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
    EXPECT_EQ(ctx.added_body_goals, (std::vector<const expr*>{&leftover, &added}));
}

TEST_F(PudCandidateSearchTest, NoLiveEdgesMeansAxiomRefuted) {
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(witness_, resume(_)).WillOnce(Return(
        pud_witness_search_result{pud_witness_search_result::failed{}}));
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudCandidateSearchTest, AfterOneLiveEdgeFailsScansRemainingOutgoingEdges) {
    pud_candidate_search_context ctx = make_ctx(&a0_, {make_edge(&c0_, &c0_)}, {});
    const pud_rule_id* expected = &c1_;
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    EXPECT_CALL(witness_, resume(_)).WillOnce([expected](pud_witness_search_context& edge) {
        EXPECT_EQ(edge.edge_root, expected);
        edge.current = expected;
        return pud_witness_search_result{pud_witness_search_result::found{}};
    });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}

TEST_F(PudCandidateSearchTest, QueryAdvanceRewritesEdgeRootWhenCurrentIsDeeper) {
    pud_rule_id g0{pud_rule_id::inference{&c0_, 0, &a0_}};
    pud_candidate_search_context ctx = make_ctx(&a0_, {make_edge(&c0_, &g0)}, {&body_});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(get_parent_, get(&g0)).WillRepeatedly(Return(&c0_));
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &c0_);
    ASSERT_EQ(ctx.live_edges.size(), 1u);
    EXPECT_EQ(ctx.live_edges[0].edge_root, &g0);
}

TEST_F(PudCandidateSearchTest, FillLiveEdgesStopsAtTwoOfThreeChildren) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {});
    const children_set_t kids{&c0_, &c1_, &c2};
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(kids));
    EXPECT_CALL(witness_, resume(_))
        .Times(2)
        .WillRepeatedly([](pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
    auto it = kids.begin();
    EXPECT_EQ(ctx.live_edges[0].edge_root, *it);
    ++it;
    EXPECT_EQ(ctx.live_edges[1].edge_root, *it);
}

TEST_F(PudCandidateSearchTest, LeafThatFailsUnifyIsAxiomRefuted) {
    pud_candidate_search_context ctx = make_ctx(&a0_, {}, {});
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(false));
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudCandidateSearchTest, ResumeTwiceOnChoicePointStaysChoicePoint) {
    pud_candidate_search_context ctx = make_ctx(
        &a0_, {make_edge(&c0_, &c0_), make_edge(&c1_, &c1_)}, {});
    for (int step = 0; step < 8; ++step) {
        const pud_candidate_search_result result = search_.resume(ctx);
        EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(
            result.content));
        EXPECT_EQ(ctx.live_edges.size(), 2u);
        EXPECT_EQ(ctx.cursor, &a0_);
    }
}

TEST_F(PudCandidateSearchTest, StressUnaryChainAdvance) {
    constexpr int k_depth = 16;
    std::vector<pud_rule_id> nodes;
    nodes.reserve(static_cast<size_t>(k_depth));
    nodes.push_back(pud_rule_id{pud_rule_id::axiom{0}});
    for (int idx = 1; idx < k_depth; ++idx)
        nodes.push_back(pud_rule_id{
            pud_rule_id::inference{&nodes[0], 0, &nodes[0]}});

    ON_CALL(unify_, unify_head(_, _)).WillByDefault([&nodes](pud_candidate_search_context&, const pud_rule_id* node) {
        return node == &nodes.back();
    });
    ON_CALL(children_, get(_)).WillByDefault(
        [&nodes](const pud_rule_id* node) -> children_opt_t {
            for (int idx = 0; idx + 1 < k_depth; ++idx) {
                if (node != &nodes[static_cast<size_t>(idx)])
                    continue;
                return children_set_t{&nodes[static_cast<size_t>(idx + 1)]};
            }
            return std::nullopt;
        });
    ON_CALL(witness_, resume(_)).WillByDefault(
        [](pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });

    pud_candidate_search_context ctx = make_ctx(&nodes.front(), {}, {&body_});
    const pud_candidate_search_result result = search_.resume(ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, &nodes.back());
    EXPECT_TRUE(ctx.live_edges.empty());
}

TEST_F(PudCandidateSearchTest, FuzzResumeOnFixedMockDag) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    pud_rule_id g0{pud_rule_id::inference{&c0_, 0, &a0_}};
    pud_rule_id g1{pud_rule_id::inference{&c1_, 0, &a0_}};
    ON_CALL(unify_, unify_head(_, _)).WillByDefault([&](pud_candidate_search_context&, const pud_rule_id* node) {
        return node == &g0 || node == &g1 || node == &c2;
    });
    ON_CALL(children_, get(_)).WillByDefault([&](const pud_rule_id* node) -> children_opt_t {
        if (node == &a0_)
            return children_set_t{&c0_, &c1_, &c2};
        if (node == &c0_)
            return children_set_t{&g0};
        if (node == &c1_)
            return children_set_t{&g1};
        return std::nullopt;
    });
    ON_CALL(get_parent_, get(_)).WillByDefault([&](const pud_rule_id* node) -> const pud_rule_id* {
        if (node == &g0)
            return &c0_;
        if (node == &g1)
            return &c1_;
        if (node == &c0_ || node == &c1_ || node == &c2)
            return &a0_;
        return nullptr;
    });
    ON_CALL(witness_, resume(_)).WillByDefault(
        [](pud_witness_search_context& edge) {
            edge.current = edge.edge_root;
            return pud_witness_search_result{pud_witness_search_result::found{}};
        });

    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> seed_dist(0, 2);
    std::ostringstream log;
    for (int step = 0; step < 80; ++step) {
        pud_candidate_search_context ctx = make_ctx(&a0_, {}, {&body_, &body_, &body_});
        const int preset = seed_dist(rng);
        log << step << ':' << preset << ' ';
        if (preset == 1)
            ctx.live_edges.push_back(make_edge(&c0_, &c0_));
        if (preset == 2) {
            ctx.live_edges.push_back(make_edge(&c0_, &c0_));
            ctx.live_edges.push_back(make_edge(&c1_, &c1_));
        }
        const pud_candidate_search_result result = search_.resume(ctx);
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
