// pud_unfolder: materialize + link; yields whatever queries report.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::_;

struct MockGetNode {
    MOCK_METHOD(const pud_db_node&, get_node, (const pud_rule_id*), ());
};

struct MockUnifyCallee {
    MOCK_METHOD(bool, unify_callee,
                (pud_query&, const pud_rule_id*, (std::vector<uint32_t>&), om_interval&), ());
};

struct MockNormalize {
    MOCK_METHOD(const expr*, normalize,
                (om_interval, framed_expr, uint32_t,
                 (std::unordered_map<uint32_t, uint32_t>&)), ());
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t), ());
};

struct MockAddInference {
    MOCK_METHOD(const pud_rule_id*, add_inference,
                (const pud_rule_id*, size_t, const pud_rule_id*,
                 (std::vector<pud_added_unification>),
                 (std::vector<const expr*>),
                 uint32_t), ());
};

struct MockLinkChildren {
    MOCK_METHOD(void, link_children,
                (const pud_rule_id*, (const std::vector<const pud_rule_id*>&)), ());
};

struct MockEffectiveBody {
    MOCK_METHOD((std::vector<const expr*>), effective_body, (const pud_rule_id*), ());
};

struct MockLiveCallees {
    MOCK_METHOD((std::vector<const pud_rule_id*>), live_callees,
                (const pud_rule_id*, size_t), ());
};

struct MockGetLeafQueries {
    MOCK_METHOD(const std::vector<pud_query*>&, get, (const pud_rule_id*), ());
};

struct MockReplaceUnfolded {
    MOCK_METHOD(void, replace_unfolded,
                (const pud_rule_id*, size_t, (const std::vector<const pud_rule_id*>&)), ());
};

struct MockTakeForcedUnfolds {
    MOCK_METHOD((std::vector<pud_forced_unfold>), take_forced_unfolds, (), ());
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockGetNode>,
    NiceMock<MockUnifyCallee>,
    NiceMock<MockNormalize>,
    NiceMock<MockMakeVar>,
    NiceMock<MockAddInference>,
    NiceMock<MockLinkChildren>,
    NiceMock<MockEffectiveBody>,
    NiceMock<MockLiveCallees>,
    NiceMock<MockGetLeafQueries>,
    NiceMock<MockReplaceUnfolded>,
    NiceMock<MockTakeForcedUnfolds>>;

struct PudUnfolderTest : public ::testing::Test {
    PudUnfolderTest()
        : open_(10)
        , close_(40)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::functor{1, {}}}
        , var0_{expr::var{0}}
        , leaf_{pud_rule_id::axiom{0}}
        , callee_a_{pud_rule_id::axiom{1}}
        , callee_b_{pud_rule_id::axiom{2}}
        , child_{pud_rule_id::inference{&leaf_, 0, &leaf_}}
        , child_a_{pud_rule_id::inference{&leaf_, 0, &callee_a_}}
        , child_b_{pud_rule_id::inference{&leaf_, 0, &callee_b_}}
        , parent_node_{interval_, {}, {&body_}, 1}
        , parent_query_{interval_, &body_, {}, 1}
        , parent_query_ptrs_{&parent_query_}
        , unfolder_(get_node_, unify_callee_, normalize_, make_var_,
                    add_inference_, link_children_, effective_body_,
                    live_callees_, get_queries_, replace_unfolded_,
                    take_forced_) {
        ON_CALL(get_node_, get_node(&leaf_)).WillByDefault(ReturnRef(parent_node_));
        ON_CALL(get_queries_, get(&leaf_)).WillByDefault(ReturnRef(parent_query_ptrs_));
        ON_CALL(live_callees_, live_callees(&leaf_, 0)).WillByDefault(
            Return(std::vector<const pud_rule_id*>{&leaf_}));
        ON_CALL(unify_callee_, unify_callee(_, _, _, _)).WillByDefault(Return(true));
        ON_CALL(make_var_, make_var(_)).WillByDefault(Return(&var0_));
        ON_CALL(normalize_, normalize(_, _, _, _)).WillByDefault(Return(&body_));
        ON_CALL(effective_body_, effective_body(_)).WillByDefault(
            Return(std::vector<const expr*>{&body_}));
        ON_CALL(add_inference_, add_inference(_, _, _, _, _, _)).WillByDefault(Return(&child_));
        ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(
            Return(std::vector<pud_forced_unfold>{
                pud_forced_unfold{pud_forced_unfold::unit{&child_, 0}}}));
    }

    struct unfold_out {
        std::vector<pud_forced_unfold> yields;
        std::vector<const pud_rule_id*> children;
    };

    unfold_out drain(
            coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>> task) {
        unfold_out out;
        while (!task.done()) {
            task.resume();
            if (task.has_yield())
                out.yields.push_back(task.consume_yield());
        }
        out.children = task.result();
        return out;
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    expr var0_;
    pud_rule_id leaf_;
    pud_rule_id callee_a_;
    pud_rule_id callee_b_;
    pud_rule_id child_;
    pud_rule_id child_a_;
    pud_rule_id child_b_;
    pud_db_node parent_node_;
    pud_query parent_query_;
    std::vector<pud_query*> parent_query_ptrs_;
    NiceMock<MockGetNode> get_node_;
    NiceMock<MockUnifyCallee> unify_callee_;
    NiceMock<MockNormalize> normalize_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockAddInference> add_inference_;
    NiceMock<MockLinkChildren> link_children_;
    NiceMock<MockEffectiveBody> effective_body_;
    NiceMock<MockLiveCallees> live_callees_;
    NiceMock<MockGetLeafQueries> get_queries_;
    NiceMock<MockReplaceUnfolded> replace_unfolded_;
    NiceMock<MockTakeForcedUnfolds> take_forced_;
    test_unfolder_t unfolder_;
};

TEST_F(PudUnfolderTest, UnfoldLinksChildRewritesQueriesAndYieldsUnit) {
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _, _, _))
        .WillOnce(Return(&child_));
    EXPECT_CALL(link_children_, link_children(&leaf_, ElementsAre(&child_)));
    EXPECT_CALL(replace_unfolded_, replace_unfolded(&leaf_, 0, ElementsAre(&child_)));

    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(out.yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(out.yields[0].content));
    const auto unit = std::get<pud_forced_unfold::unit>(out.yields[0].content);
    EXPECT_EQ(unit.leaf, &child_);
    EXPECT_EQ(unit.body_goal_idx, 0u);
    EXPECT_THAT(out.children, ElementsAre(&child_));
}

TEST_F(PudUnfolderTest, UnfoldYieldsWhatQueriesReport) {
    ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(
        Return(std::vector<pud_forced_unfold>{
            pud_forced_unfold{pud_forced_unfold::refuted{&child_}}}));
    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(out.yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::refuted>(out.yields[0].content));
    EXPECT_EQ(std::get<pud_forced_unfold::refuted>(out.yields[0].content).leaf, &child_);
}

TEST_F(PudUnfolderTest, LiveCursorIsTheCalleePassedToInference) {
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _, _, _))
        .WillOnce(Return(&child_));
    drain(unfolder_.unfold(&leaf_, 0));
}

TEST_F(PudUnfolderTest, UnfoldCreatesOneChildPerLiveCandidate) {
    ON_CALL(live_callees_, live_callees(&leaf_, 0)).WillByDefault(
        Return(std::vector<const pud_rule_id*>{&callee_a_, &callee_b_}));
    ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));

    {
        InSequence seq;
        EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &callee_a_, _, _, _))
            .WillOnce(Return(&child_a_));
        EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &callee_b_, _, _, _))
            .WillOnce(Return(&child_b_));
        EXPECT_CALL(link_children_,
                    link_children(&leaf_, ElementsAre(&child_a_, &child_b_)));
        EXPECT_CALL(replace_unfolded_,
                    replace_unfolded(&leaf_, 0, ElementsAre(&child_a_, &child_b_)));
    }

    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_THAT(out.children, ElementsAre(&child_a_, &child_b_));
}

TEST_F(PudUnfolderTest, MaterializePassesTouchedRepsAsAddedUnificationsAndLiftsLvc) {
    ON_CALL(unify_callee_, unify_callee(_, _, _, _)).WillByDefault(
        [](pud_query&, const pud_rule_id*, std::vector<uint32_t>& reps, om_interval&) {
            reps = {3, 5};
            return true;
        });
    ON_CALL(normalize_, normalize(_, _, _, _)).WillByDefault(
        [](om_interval, framed_expr, uint32_t,
           std::unordered_map<uint32_t, uint32_t>& translation) {
            const uint32_t key = static_cast<uint32_t>(translation.size());
            translation.emplace(key, key);
            return nullptr;
        });
    std::vector<pud_added_unification> unifs;
    uint32_t child_lvc = 0;
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _, _, _))
        .WillOnce(DoAll(SaveArg<3>(&unifs), SaveArg<5>(&child_lvc), Return(&child_)));
    drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(unifs.size(), 2u);
    EXPECT_EQ(unifs[0].var_idx, 3u);
    EXPECT_EQ(unifs[1].var_idx, 5u);
    EXPECT_EQ(child_lvc, parent_node_.lvc + 3u);
}

TEST_F(PudUnfolderTest, MaterializeEmptyEffectiveBodyPassesEmptyGoals) {
    ON_CALL(effective_body_, effective_body(_)).WillByDefault(
        Return(std::vector<const expr*>{}));
    std::vector<const expr*> goals;
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _, _, _))
        .WillOnce(DoAll(SaveArg<4>(&goals), Return(&child_)));
    drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_TRUE(goals.empty());
}

TEST_F(PudUnfolderTest, UnfoldUsesNonzeroBodyGoalIdx) {
    pud_query extra{interval_, &body_, {}, 1};
    std::vector<pud_query*> two_queries{&parent_query_, &extra};
    ON_CALL(get_queries_, get(&leaf_)).WillByDefault(ReturnRef(two_queries));
    ON_CALL(live_callees_, live_callees(&leaf_, 1)).WillByDefault(
        Return(std::vector<const pud_rule_id*>{&leaf_}));
    EXPECT_CALL(add_inference_, add_inference(&leaf_, 1, &leaf_, _, _, _))
        .WillOnce(Return(&child_));
    EXPECT_CALL(replace_unfolded_, replace_unfolded(&leaf_, 1, ElementsAre(&child_)));
    drain(unfolder_.unfold(&leaf_, 1));
}

TEST_F(PudUnfolderTest, UnfoldYieldsNothingWhenQueriesReportEmpty) {
    ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));
    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_TRUE(out.yields.empty());
}

TEST_F(PudUnfolderTest, UnfoldYieldsMultipleForcedUnfoldsInOrder) {
    ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(
        Return(std::vector<pud_forced_unfold>{
            pud_forced_unfold{pud_forced_unfold::unit{&child_a_, 0}},
            pud_forced_unfold{pud_forced_unfold::refuted{&child_b_}}}));
    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(out.yields.size(), 2u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::unit>(out.yields[0].content));
    EXPECT_EQ(std::get<pud_forced_unfold::unit>(out.yields[0].content).leaf, &child_a_);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::refuted>(out.yields[1].content));
    EXPECT_EQ(std::get<pud_forced_unfold::refuted>(out.yields[1].content).leaf, &child_b_);
}

TEST_F(PudUnfolderTest, UnfoldSequenceIsMaterializeThenLinkThenReplaceThenTake) {
    for (int step = 0; step < 3; ++step) {
        InSequence seq;
        EXPECT_CALL(add_inference_, add_inference(&leaf_, 0, &leaf_, _, _, _))
            .WillOnce(Return(&child_));
        EXPECT_CALL(link_children_, link_children(&leaf_, ElementsAre(&child_)));
        EXPECT_CALL(replace_unfolded_, replace_unfolded(&leaf_, 0, ElementsAre(&child_)));
        EXPECT_CALL(take_forced_, take_forced_unfolds())
            .WillOnce(Return(std::vector<pud_forced_unfold>{}));
        drain(unfolder_.unfold(&leaf_, 0));
    }
}

TEST_F(PudUnfolderTest, StressManyCallees) {
    std::vector<pud_rule_id> callees;
    std::vector<pud_rule_id> children;
    callees.reserve(24);
    children.reserve(24);
    for (int idx = 0; idx < 24; ++idx) {
        callees.push_back(pud_rule_id{pud_rule_id::axiom{static_cast<size_t>(idx + 10)}});
        children.push_back(pud_rule_id{pud_rule_id::inference{
            &leaf_, 0, &callees.back()}});
    }
    std::vector<const pud_rule_id*> callee_ptrs;
    std::vector<const pud_rule_id*> child_ptrs;
    for (int idx = 0; idx < 24; ++idx) {
        callee_ptrs.push_back(&callees[static_cast<size_t>(idx)]);
        child_ptrs.push_back(&children[static_cast<size_t>(idx)]);
    }
    ON_CALL(live_callees_, live_callees(&leaf_, 0)).WillByDefault(Return(callee_ptrs));
    ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));
    {
        InSequence seq;
        for (int idx = 0; idx < 24; ++idx) {
            EXPECT_CALL(add_inference_,
                        add_inference(&leaf_, 0, callee_ptrs[static_cast<size_t>(idx)], _, _, _))
                .WillOnce(Return(child_ptrs[static_cast<size_t>(idx)]));
        }
        EXPECT_CALL(link_children_, link_children(&leaf_, child_ptrs));
        EXPECT_CALL(replace_unfolded_, replace_unfolded(&leaf_, 0, child_ptrs));
    }
    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_EQ(out.children, child_ptrs);
}

TEST_F(PudUnfolderTest, FuzzUnfold) {
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> callee_count(1, 4);
    std::uniform_int_distribution<int> yield_kind(0, 2);
    std::ostringstream log;
    std::vector<pud_rule_id> callees;
    std::vector<pud_rule_id> children;
    callees.reserve(16);
    children.reserve(16);
    for (int idx = 0; idx < 16; ++idx) {
        callees.push_back(pud_rule_id{pud_rule_id::axiom{static_cast<size_t>(idx + 20)}});
        children.push_back(pud_rule_id{pud_rule_id::inference{&leaf_, 0, &callees.back()}});
    }
    for (int step = 0; step < 40; ++step) {
        const int count = callee_count(rng);
        log << step << ':' << count << ' ';
        std::vector<const pud_rule_id*> callee_ptrs;
        std::vector<const pud_rule_id*> child_ptrs;
        for (int idx = 0; idx < count; ++idx) {
            callee_ptrs.push_back(&callees[static_cast<size_t>(idx)]);
            child_ptrs.push_back(&children[static_cast<size_t>(idx)]);
        }
        ON_CALL(live_callees_, live_callees(&leaf_, 0)).WillByDefault(Return(callee_ptrs));
        const int kind = yield_kind(rng);
        std::vector<pud_forced_unfold> yields;
        if (kind == 1)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::unit{&child_, 0}});
        if (kind == 2)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::refuted{&child_}});
        ON_CALL(take_forced_, take_forced_unfolds()).WillByDefault(Return(yields));
        int infer_idx = 0;
        ON_CALL(add_inference_, add_inference(_, _, _, _, _, _)).WillByDefault(
            [&](const pud_rule_id*, size_t, const pud_rule_id*,
                std::vector<pud_added_unification>,
                std::vector<const expr*>,
                uint32_t) {
                const pud_rule_id* out = child_ptrs[static_cast<size_t>(infer_idx)];
                ++infer_idx;
                return out;
            });
        std::vector<const pud_rule_id*> linked;
        std::vector<const pud_rule_id*> replaced;
        ON_CALL(link_children_, link_children(_, _)).WillByDefault(
            [&](const pud_rule_id*, const std::vector<const pud_rule_id*>& kids) {
                linked = kids;
            });
        ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
            [&](const pud_rule_id*, size_t, const std::vector<const pud_rule_id*>& kids) {
                replaced = kids;
            });
        const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
        EXPECT_EQ(out.children.size(), callee_ptrs.size())
            << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(linked, out.children) << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(replaced, out.children) << "seed " << k_seed << " log " << log.str();
    }
}
