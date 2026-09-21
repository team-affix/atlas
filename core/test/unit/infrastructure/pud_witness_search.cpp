// pud_witness_search: accept-first resume; DFS descend then next sibling; stop at edge_root.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

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

using test_search_t = pud_witness_search<NiceMock<MockIsLeaf>,
                                         NiceMock<MockOrderedChildren>,
                                         NiceMock<MockTryParent>,
                                         NiceMock<MockUnifyHead>>;

struct PudWitnessSearchTest : public ::testing::Test {
    PudWitnessSearchTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , a0_{pud_rule_id::axiom{0}}
        , c0_{pud_rule_id::inference{&a0_, 0, &a0_}}
        , c1_{pud_rule_id::inference{&a0_, 1, &a0_}}
        , g0_{pud_rule_id::inference{&c0_, 0, &a0_}}
        , query_{interval_, &body_, {}, 1}
        , search_(is_leaf_, children_, try_parent_, unify_) {}

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    pud_rule_id g0_;
    pud_query query_;
    NiceMock<MockIsLeaf> is_leaf_;
    NiceMock<MockOrderedChildren> children_;
    NiceMock<MockTryParent> try_parent_;
    NiceMock<MockUnifyHead> unify_;
    test_search_t search_;
};

TEST_F(PudWitnessSearchTest, AcceptsCurrentIfItIsUnifyingLeaf) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, DescendsIntoChildrenWhenCurrentIsNoLongerALeaf) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &c0_);
}

TEST_F(PudWitnessSearchTest, PrunesSubtreeWhenUnifyFails) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(children_, ordered_children(_)).Times(0);
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::failed>(result.content));
}

TEST_F(PudWitnessSearchTest, TriesNextSiblingInIdOrderAfterFailedChild) {
    pud_witness_search_context ctx{&a0_, &c0_};
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(is_leaf_, is_leaf(&c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(try_parent_, try_parent(&c0_)).WillRepeatedly(Return(&a0_));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, StopsAtEdgeRootAndFailsWhenNoSiblingWorks) {
    pud_witness_search_context ctx{&c0_, &g0_};
    EXPECT_CALL(is_leaf_, is_leaf(&g0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(try_parent_, try_parent(&g0_)).WillRepeatedly(Return(&c0_));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&g0_}));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::failed>(result.content));
}

TEST_F(PudWitnessSearchTest, SelfNodeMayBeAWitness) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
}

TEST_F(PudWitnessSearchTest, DescendsTwoLevelsToGrandchild) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&g0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_}));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&g0_}));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &g0_);
}

TEST_F(PudWitnessSearchTest, InternalUnifyWithNoUnifyingChildFailsThenTriesSibling) {
    pud_witness_search_context ctx{&a0_, &c0_};
    EXPECT_CALL(is_leaf_, is_leaf(&c0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(is_leaf_, is_leaf(&g0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(is_leaf_, is_leaf(&c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(try_parent_, try_parent(&c0_)).WillRepeatedly(Return(&a0_));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&g0_}));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ClimbsTwoParentsToReachUncle) {
    pud_witness_search_context ctx{&a0_, &g0_};
    EXPECT_CALL(is_leaf_, is_leaf(&g0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(is_leaf_, is_leaf(&c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(try_parent_, try_parent(&g0_)).WillRepeatedly(Return(&c0_));
    EXPECT_CALL(try_parent_, try_parent(&c0_)).WillRepeatedly(Return(&a0_));
    EXPECT_CALL(children_, ordered_children(&c0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&g0_}));
    EXPECT_CALL(children_, ordered_children(&a0_))
        .WillRepeatedly(Return(std::vector<const pud_rule_id*>{&c0_, &c1_}));
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ResumeFoundKeepsAcceptableCurrent) {
    pud_witness_search_context ctx{&a0_, &a0_};
    EXPECT_CALL(is_leaf_, is_leaf(&a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    for (int step = 0; step < 8; ++step) {
        const pud_witness_search_result result = search_.resume(query_, ctx);
        EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
        EXPECT_EQ(ctx.current, &a0_);
    }
}

TEST_F(PudWitnessSearchTest, StressWideSiblingScan) {
    std::vector<pud_rule_id> siblings;
    siblings.reserve(32);
    for (int idx = 0; idx < 32; ++idx)
        siblings.push_back(pud_rule_id{pud_rule_id::inference{&a0_, static_cast<size_t>(idx), &a0_}});
    std::vector<const pud_rule_id*> sibling_ptrs;
    sibling_ptrs.reserve(siblings.size());
    for (const pud_rule_id& sibling : siblings)
        sibling_ptrs.push_back(&sibling);

    EXPECT_CALL(is_leaf_, is_leaf(_)).WillRepeatedly(
        [this](const pud_rule_id* node) { return node != &a0_; });
    EXPECT_CALL(unify_, unify_head(_, _)).WillRepeatedly(
        [this, &siblings](pud_query&, const pud_rule_id* node) {
            return node == &a0_ || node == &siblings.back();
        });
    EXPECT_CALL(children_, ordered_children(&a0_)).WillRepeatedly(Return(sibling_ptrs));
    pud_witness_search_context ctx{&a0_, &a0_};
    const pud_witness_search_result result = search_.resume(query_, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, &siblings.back());
}

TEST_F(PudWitnessSearchTest, FuzzResumeOnFixedMockTree) {
    ON_CALL(is_leaf_, is_leaf(_)).WillByDefault([&](const pud_rule_id* node) {
        return node == &g0_ || node == &c1_;
    });
    ON_CALL(unify_, unify_head(_, _)).WillByDefault([&](pud_query&, const pud_rule_id* node) {
        return node == &g0_ || node == &c1_;
    });
    ON_CALL(children_, ordered_children(_)).WillByDefault([&](const pud_rule_id* node) {
        if (node == &a0_)
            return std::vector<const pud_rule_id*>{&c0_, &c1_};
        if (node == &c0_)
            return std::vector<const pud_rule_id*>{&g0_};
        return std::vector<const pud_rule_id*>{};
    });
    ON_CALL(try_parent_, try_parent(_)).WillByDefault([&](const pud_rule_id* node) -> const pud_rule_id* {
        if (node == &g0_)
            return &c0_;
        if (node == &c0_ || node == &c1_)
            return &a0_;
        return nullptr;
    });

    const std::vector<std::pair<const pud_rule_id*, const pud_rule_id*>> legal = {
        {&a0_, &a0_}, {&a0_, &c0_}, {&a0_, &c1_}, {&a0_, &g0_},
        {&c0_, &c0_}, {&c0_, &g0_}, {&c1_, &c1_}, {&g0_, &g0_},
    };
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<size_t> pick(0, legal.size() - 1);
    std::ostringstream log;
    for (int step = 0; step < 80; ++step) {
        const auto [edge_root, current] = legal[pick(rng)];
        log << step << ':' << edge_root << ',' << current << ' ';
        pud_witness_search_context ctx{edge_root, current};
        const pud_witness_search_result result = search_.resume(query_, ctx);
        const bool found =
            std::holds_alternative<pud_witness_search_result::found>(result.content);
        const bool failed =
            std::holds_alternative<pud_witness_search_result::failed>(result.content);
        EXPECT_TRUE(found || failed) << "seed " << k_seed << " log " << log.str();
        if (found) {
            EXPECT_TRUE(ctx.current == &g0_ || ctx.current == &c1_)
                << "seed " << k_seed << " log " << log.str();
        }
        if (failed)
            EXPECT_EQ(ctx.edge_root, edge_root) << "seed " << k_seed << " log " << log.str();
    }
}
