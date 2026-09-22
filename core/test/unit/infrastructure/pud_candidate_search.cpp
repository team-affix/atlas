// pud_candidate_search: accept-first; choice-point, self-witness, unary advance, axiom_refuted.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include "infrastructure/pud_candidate_search.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_pair.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

using children_set_t = std::set<const pud_rule_id*>;
using children_opt_t = std::optional<children_set_t>;

struct MockResumeWitnessSearch {
    MOCK_METHOD(void, resume, (pud_witness_search_context&), ());
};

struct MockTryEnter {
    MOCK_METHOD(bool, try_enter, (pud_witness_search_context&, const pud_rule_id*), ());
};

struct MockGetChildren {
    MOCK_METHOD(children_opt_t, get, (const pud_rule_id*), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_rule_id*, get, (const pud_rule_id*), ());
};

using test_search_t = pud_candidate_search<NiceMock<MockResumeWitnessSearch>,
                                           NiceMock<MockTryEnter>,
                                           NiceMock<MockGetChildren>,
                                           NiceMock<MockGetParent>>;

struct PudCandidateSearchTest : public ::testing::Test {
    PudCandidateSearchTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , c1_{pud_rule_id::inference{&a0_, 1, &a0_}}
        , search_(witness_, try_enter_, children_, get_parent_) {
        ON_CALL(try_enter_, try_enter(_, _)).WillByDefault(Return(true));
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;

    pud_witness_search_context make_edge(const pud_rule_id* search_root,
                                         const pud_rule_id* current) {
        return pud_witness_search_context{&a0_, 0, 1, search_root, current};
    }

    pud_witness_pair make_pair(pud_witness_search_context a,
                               pud_witness_search_context b) {
        return pud_witness_pair{std::move(a), std::move(b)};
    }

    pud_candidate_search_context make_ctx(
            const pud_rule_id* cursor,
            std::optional<pud_witness_pair> witnesses) {
        return pud_candidate_search_context{
            &a0_, 0, 1, cursor, std::move(witnesses)};
    }

    void expect_choice(const pud_candidate_search_context& ctx) {
        ASSERT_TRUE(ctx.witnesses.has_value());
        EXPECT_NE(ctx.witnesses->a.current, nullptr);
        EXPECT_NE(ctx.witnesses->b.current, nullptr);
        EXPECT_NE(ctx.witnesses->a.search_root, nullptr);
        EXPECT_NE(ctx.witnesses->b.search_root, nullptr);
        EXPECT_NE(ctx.cursor, nullptr);
    }

    void expect_self(const pud_candidate_search_context& ctx) {
        EXPECT_FALSE(ctx.witnesses.has_value());
        EXPECT_NE(ctx.cursor, nullptr);
    }

    void expect_refuted(const pud_candidate_search_context& ctx) {
        EXPECT_EQ(ctx.cursor, nullptr);
        EXPECT_FALSE(ctx.witnesses.has_value());
    }

    NiceMock<MockResumeWitnessSearch> witness_;
    NiceMock<MockTryEnter> try_enter_;
    NiceMock<MockGetChildren> children_;
    NiceMock<MockGetParent> get_parent_;
    test_search_t search_;
};

TEST_F(PudCandidateSearchTest, AcceptsExistingChoicePoint) {
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(&c0_, &c0_), make_edge(&c1_, &c1_)));
    search_.resume(ctx);
    expect_choice(ctx);
}

TEST_F(PudCandidateSearchTest, AcceptsSelfWitnessingLeafCursor) {
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = edge.search_root;
    });
    search_.resume(ctx);
    expect_self(ctx);
}

TEST_F(PudCandidateSearchTest, CursorUnifyFailureRefutes) {
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    EXPECT_CALL(try_enter_, try_enter(_, &a0_)).WillOnce(Return(false));
    EXPECT_CALL(witness_, resume(_)).Times(0);
    search_.resume(ctx);
    expect_refuted(ctx);
}

TEST_F(PudCandidateSearchTest, TwoLiveOutgoingEdgesAreAChoicePoint) {
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    EXPECT_CALL(witness_, resume(_))
        .WillOnce([](pud_witness_search_context& edge) {
            edge.current = edge.search_root;
        })
        .WillOnce([](pud_witness_search_context& edge) {
            edge.current = edge.search_root;
        });
    search_.resume(ctx);
    expect_choice(ctx);
}

TEST_F(PudCandidateSearchTest, OneLiveEdgeQueryAdvancesThenSelfWitnesses) {
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(witness_, resume(_))
        .WillOnce([](pud_witness_search_context& edge) {
            edge.current = edge.search_root;
        })
        .WillOnce([](pud_witness_search_context& edge) {
            edge.current = edge.search_root;
        });
    search_.resume(ctx);
    expect_self(ctx);
    EXPECT_EQ(ctx.cursor, &c0_);
}

TEST_F(PudCandidateSearchTest, NoLiveEdgesMeansAxiomRefuted) {
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = nullptr;
    });
    search_.resume(ctx);
    expect_refuted(ctx);
}

TEST_F(PudCandidateSearchTest, AfterOneLiveEdgeFailsScansRemainingOutgoingEdges) {
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(&c0_, &c0_), make_edge(nullptr, nullptr)));
    const children_set_t kids{&c0_, &c1_};
    auto it = kids.begin();
    const pud_rule_id* left = *it;
    ++it;
    const pud_rule_id* right = *it;
    ctx.witnesses = make_pair(make_edge(left, left), make_edge(nullptr, nullptr));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(kids));
    EXPECT_CALL(witness_, resume(_)).WillOnce([right](pud_witness_search_context& edge) {
        EXPECT_EQ(edge.search_root, right);
        edge.current = right;
    });
    search_.resume(ctx);
    expect_choice(ctx);
}

TEST_F(PudCandidateSearchTest, LeftDiesFillsFromSuccessorOfRightmostRoot) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    const children_set_t kids{&c0_, &c1_, &c2};
    auto it = kids.begin();
    const pud_rule_id* left = *it;
    ++it;
    const pud_rule_id* mid = *it;
    ++it;
    const pud_rule_id* right = *it;
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(left, nullptr), make_edge(mid, mid)));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(kids));
    EXPECT_CALL(witness_, resume(_)).WillOnce([right](pud_witness_search_context& edge) {
        EXPECT_EQ(edge.search_root, right);
        edge.current = right;
    });
    search_.resume(ctx);
    expect_choice(ctx);
    EXPECT_EQ(ctx.witnesses->a.search_root, right);
    EXPECT_EQ(ctx.witnesses->b.search_root, mid);
}

TEST_F(PudCandidateSearchTest, RightDiesFillsFromSuccessorOfDeadRoot) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    const children_set_t kids{&c0_, &c1_, &c2};
    auto it = kids.begin();
    const pud_rule_id* left = *it;
    ++it;
    const pud_rule_id* mid = *it;
    ++it;
    const pud_rule_id* right = *it;
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(left, left), make_edge(mid, nullptr)));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(kids));
    EXPECT_CALL(witness_, resume(_)).WillOnce([right](pud_witness_search_context& edge) {
        EXPECT_EQ(edge.search_root, right);
        edge.current = right;
    });
    search_.resume(ctx);
    expect_choice(ctx);
    EXPECT_EQ(ctx.witnesses->a.search_root, left);
    EXPECT_EQ(ctx.witnesses->b.search_root, right);
}

TEST_F(PudCandidateSearchTest, AfterAdvanceDeadRootIsNullAndFillStartsRightOfSurvivor) {
    pud_rule_id g0{pud_rule_id::inference{&c0_, 0, &a0_}};
    pud_rule_id g1{pud_rule_id::inference{&c0_, 1, &a0_}};
    const children_set_t grands{&g0, &g1};
    auto git = grands.begin();
    const pud_rule_id* left_g = *git;
    ++git;
    const pud_rule_id* right_g = *git;
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(&c0_, left_g), make_edge(&c1_, nullptr)));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(grands));
    EXPECT_CALL(get_parent_, get(left_g)).WillRepeatedly(Return(&c0_));
    EXPECT_CALL(witness_, resume(_)).WillOnce([right_g](pud_witness_search_context& edge) {
        EXPECT_EQ(edge.search_root, right_g);
        edge.current = right_g;
    });
    search_.resume(ctx);
    expect_choice(ctx);
    EXPECT_EQ(ctx.cursor, &c0_);
    const bool a_is_survivor = ctx.witnesses->a.search_root == left_g;
    const pud_witness_search_context& survivor =
        a_is_survivor ? ctx.witnesses->a : ctx.witnesses->b;
    const pud_witness_search_context& filled =
        a_is_survivor ? ctx.witnesses->b : ctx.witnesses->a;
    EXPECT_EQ(survivor.search_root, left_g);
    EXPECT_EQ(filled.search_root, right_g);
}

TEST_F(PudCandidateSearchTest, CurrentEqualsCursorAfterRebaseClearsThePair) {
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(&c0_, &c0_), make_edge(&c1_, nullptr)));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = edge.search_root;
    });
    search_.resume(ctx);
    expect_self(ctx);
    EXPECT_EQ(ctx.cursor, &c0_);
}

TEST_F(PudCandidateSearchTest, FillLiveEdgesStopsAtTwoOfThreeChildren) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    const children_set_t kids{&c0_, &c1_, &c2};
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(kids));
    EXPECT_CALL(witness_, resume(_))
        .Times(2)
        .WillRepeatedly([](pud_witness_search_context& edge) {
            edge.current = edge.search_root;
        });
    search_.resume(ctx);
    expect_choice(ctx);
    auto it = kids.begin();
    EXPECT_EQ(ctx.witnesses->a.search_root, *it);
    ++it;
    EXPECT_EQ(ctx.witnesses->b.search_root, *it);
}

TEST_F(PudCandidateSearchTest, LeafThatFailsUnifyIsAxiomRefuted) {
    pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(witness_, resume(_)).WillOnce([](pud_witness_search_context& edge) {
        edge.current = nullptr;
    });
    search_.resume(ctx);
    expect_refuted(ctx);
}

TEST_F(PudCandidateSearchTest, ResumeTwiceOnChoicePointStaysChoicePoint) {
    pud_candidate_search_context ctx = make_ctx(
        &a0_,
        make_pair(make_edge(&c0_, &c0_), make_edge(&c1_, &c1_)));
    for (int step = 0; step < 8; ++step) {
        search_.resume(ctx);
        expect_choice(ctx);
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
            edge.current = edge.search_root;
        });

    pud_candidate_search_context ctx = make_ctx(&nodes.front(), std::nullopt);
    search_.resume(ctx);
    expect_self(ctx);
    EXPECT_EQ(ctx.cursor, &nodes.back());
}

TEST_F(PudCandidateSearchTest, FuzzResumeOnFixedMockDag) {
    pud_rule_id c2{pud_rule_id::inference{&a0_, 2, &a0_}};
    pud_rule_id g0{pud_rule_id::inference{&c0_, 0, &a0_}};
    pud_rule_id g1{pud_rule_id::inference{&c1_, 0, &a0_}};
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
            edge.current = edge.search_root;
        });

    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> seed_dist(0, 2);
    std::ostringstream log;
    for (int step = 0; step < 80; ++step) {
        pud_candidate_search_context ctx = make_ctx(&a0_, std::nullopt);
        const int preset = seed_dist(rng);
        log << step << ':' << preset << ' ';
        if (preset == 1)
            ctx.witnesses = make_pair(make_edge(&c0_, &c0_), make_edge(nullptr, nullptr));
        if (preset == 2)
            ctx.witnesses = make_pair(make_edge(&c0_, &c0_), make_edge(&c1_, &c1_));
        search_.resume(ctx);
        const bool choice = ctx.witnesses.has_value()
            && ctx.witnesses->a.current != nullptr
            && ctx.witnesses->b.current != nullptr;
        const bool self = !ctx.witnesses.has_value() && ctx.cursor != nullptr;
        const bool refuted = ctx.cursor == nullptr;
        EXPECT_TRUE(choice || self || refuted) << "seed " << k_seed << " log " << log.str();
        if (self) {
            EXPECT_TRUE(ctx.cursor == &g0 || ctx.cursor == &g1 || ctx.cursor == &c2)
                << "seed " << k_seed << " log " << log.str();
        }
    }
}
