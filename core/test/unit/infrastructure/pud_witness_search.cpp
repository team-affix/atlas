// pud_witness_search: accept-first resume; DFS descend then next sibling; stop at search_root.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

using children_set_t = std::set<const pud_rule_id*>;
using children_opt_t = std::optional<children_set_t>;

struct MockGetChildren {
    MOCK_METHOD(children_opt_t, get, (const pud_rule_id*), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_rule_id*, get, (const pud_rule_id*), ());
};

struct MockUnifyHead {
    MOCK_METHOD(bool, unify_head, (pud_witness_search_context&, const pud_rule_id*), ());
};

using test_search_t = pud_witness_search<NiceMock<MockGetChildren>,
                                         NiceMock<MockGetParent>,
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
        , search_(children_, get_parent_, unify_) {}

    pud_witness_search_context make_edge(const pud_rule_id* search_root,
                                         const pud_rule_id* current) {
        return pud_witness_search_context{interval_, &body_, 1, search_root, current};
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id a0_;
    pud_rule_id c0_;
    pud_rule_id c1_;
    pud_rule_id g0_;
    NiceMock<MockGetChildren> children_;
    NiceMock<MockGetParent> get_parent_;
    NiceMock<MockUnifyHead> unify_;
    test_search_t search_;
};

TEST_F(PudWitnessSearchTest, AcceptsCurrentIfItIsUnifyingLeaf) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, DescendsIntoChildrenWhenCurrentIsNoLongerALeaf) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c0_);
}

TEST_F(PudWitnessSearchTest, PrunesSubtreeWhenUnifyFails) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(false));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_EQ(ctx.search_root, &a0_);
}

TEST_F(PudWitnessSearchTest, TriesNextSiblingInIdOrderAfterFailedChild) {
    pud_witness_search_context ctx = make_edge(&a0_, &c0_);
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(get_parent_, get(&c0_)).WillRepeatedly(Return(&a0_));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, StopsAtSearchRootAndFailsWhenNoSiblingWorks) {
    pud_witness_search_context ctx = make_edge(&c0_, &g0_);
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(get_parent_, get(&g0_)).WillRepeatedly(Return(&c0_));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_EQ(ctx.search_root, &c0_);
}

TEST_F(PudWitnessSearchTest, NullCurrentIsANoOp) {
    pud_witness_search_context ctx = make_edge(&a0_, nullptr);
    EXPECT_CALL(children_, get(_)).Times(0);
    EXPECT_CALL(unify_, unify_head(_, _)).Times(0);
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, nullptr);
    EXPECT_EQ(ctx.search_root, &a0_);
}

TEST_F(PudWitnessSearchTest, SelfNodeMayBeAWitness) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &a0_);
}

TEST_F(PudWitnessSearchTest, DescendsTwoLevelsToGrandchild) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_}));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, _)).WillRepeatedly(Return(true));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &g0_);
}

TEST_F(PudWitnessSearchTest, InternalUnifyWithNoUnifyingChildFailsThenTriesSibling) {
    pud_witness_search_context ctx = make_edge(&a0_, &c0_);
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    EXPECT_CALL(unify_, unify_head(_, &c0_)).WillRepeatedly(Return(true));
    EXPECT_CALL(unify_, unify_head(_, &g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(get_parent_, get(&c0_)).WillRepeatedly(Return(&a0_));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ClimbsTwoParentsToReachUncle) {
    pud_witness_search_context ctx = make_edge(&a0_, &g0_);
    EXPECT_CALL(children_, get(&g0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(children_, get(&c1_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &g0_)).WillRepeatedly(Return(false));
    EXPECT_CALL(unify_, unify_head(_, &c1_)).WillRepeatedly(Return(true));
    EXPECT_CALL(get_parent_, get(&g0_)).WillRepeatedly(Return(&c0_));
    EXPECT_CALL(get_parent_, get(&c0_)).WillRepeatedly(Return(&a0_));
    EXPECT_CALL(children_, get(&c0_)).WillRepeatedly(Return(children_set_t{&g0_}));
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(children_set_t{&c0_, &c1_}));
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &c1_);
}

TEST_F(PudWitnessSearchTest, ResumeFoundKeepsAcceptableCurrent) {
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    EXPECT_CALL(children_, get(&a0_)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(unify_, unify_head(_, &a0_)).WillRepeatedly(Return(true));
    for (int step = 0; step < 8; ++step) {
        search_.resume(ctx);
        EXPECT_EQ(ctx.current, &a0_);
    }
}

TEST_F(PudWitnessSearchTest, StressWideSiblingScan) {
    std::vector<pud_rule_id> siblings;
    siblings.reserve(32);
    for (int idx = 0; idx < 32; ++idx)
        siblings.push_back(pud_rule_id{pud_rule_id::inference{&a0_, static_cast<size_t>(idx), &a0_}});
    children_set_t sibling_set;
    for (const pud_rule_id& sibling : siblings)
        sibling_set.insert(&sibling);

    EXPECT_CALL(children_, get(_)).WillRepeatedly(
        [this, sibling_set](const pud_rule_id* node) -> children_opt_t {
            if (node == &a0_)
                return sibling_set;
            return std::nullopt;
        });
    EXPECT_CALL(unify_, unify_head(_, _)).WillRepeatedly(
        [this, &siblings](pud_witness_search_context&, const pud_rule_id* node) {
            return node == &a0_ || node == &siblings.back();
        });
    pud_witness_search_context ctx = make_edge(&a0_, &a0_);
    search_.resume(ctx);
    EXPECT_EQ(ctx.current, &siblings.back());
}

TEST_F(PudWitnessSearchTest, FuzzResumeOnFixedMockTree) {
    ON_CALL(unify_, unify_head(_, _)).WillByDefault([&](pud_witness_search_context&, const pud_rule_id* node) {
        return node == &g0_ || node == &c1_;
    });
    ON_CALL(children_, get(_)).WillByDefault([&](const pud_rule_id* node) -> children_opt_t {
        if (node == &a0_)
            return children_set_t{&c0_, &c1_};
        if (node == &c0_)
            return children_set_t{&g0_};
        return std::nullopt;
    });
    ON_CALL(get_parent_, get(_)).WillByDefault([&](const pud_rule_id* node) -> const pud_rule_id* {
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
        const auto [search_root, current] = legal[pick(rng)];
        log << step << ':' << search_root << ',' << current << ' ';
        pud_witness_search_context ctx = make_edge(search_root, current);
        search_.resume(ctx);
        if (ctx.current != nullptr) {
            EXPECT_TRUE(ctx.current == &g0_ || ctx.current == &c1_)
                << "seed " << k_seed << " log " << log.str();
        }
        EXPECT_EQ(ctx.search_root, search_root) << "seed " << k_seed << " log " << log.str();
    }
}
