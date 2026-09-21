// pud_unfolder: unfold_site, materialize stores, link, bind_child, replace_unfolded.

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
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"

using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

struct MockUnfoldSite {
    MOCK_METHOD(pud_unfold_site, unfold_site, (const pud_rule_id*, size_t), ());
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

struct MockGetLvc {
    MOCK_METHOD(uint32_t, get, (const pud_rule_id*), ());
};

struct MockMakeInference {
    MOCK_METHOD(const pud_rule_id*, make_inference,
                (const pud_rule_id*, size_t, const pud_rule_id*), ());
};

struct MockStoreAddedUnifications {
    MOCK_METHOD(void, store, (const pud_rule_id*, (std::vector<pud_added_unification>)), ());
};

struct MockStoreAddedBodyGoals {
    MOCK_METHOD(void, store, (const pud_rule_id*, (std::vector<const expr*>)), ());
};

struct MockStoreLvc {
    MOCK_METHOD(void, store, (const pud_rule_id*, uint32_t), ());
};

struct MockLinkChildren {
    MOCK_METHOD(void, link_children,
                (const pud_rule_id*, (const std::vector<const pud_rule_id*>&)), ());
};

struct MockBindChildInterval {
    MOCK_METHOD(void, bind_child, (const pud_rule_id*, const pud_rule_id*), ());
};

struct MockReplaceUnfolded {
    MOCK_METHOD((std::vector<pud_forced_unfold>), replace_unfolded,
                (const pud_rule_id*, size_t, (const std::vector<const pud_rule_id*>&)), ());
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockUnfoldSite>,
    NiceMock<MockUnifyCallee>,
    NiceMock<MockNormalize>,
    NiceMock<MockMakeVar>,
    NiceMock<MockGetLvc>,
    NiceMock<MockMakeInference>,
    NiceMock<MockStoreAddedUnifications>,
    NiceMock<MockStoreAddedBodyGoals>,
    NiceMock<MockStoreLvc>,
    NiceMock<MockLinkChildren>,
    NiceMock<MockBindChildInterval>,
    NiceMock<MockReplaceUnfolded>>;

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
        , parent_query_{interval_, &body_, {pud_candidate_search_context{&leaf_, {}, {&body_}}}, 1}
        , site_{&parent_query_, {&leaf_}}
        , unfolder_(unfold_site_, unify_callee_, normalize_, make_var_,
                    get_lvc_, make_inference_,
                    store_unifs_, store_goals_, store_lvc_,
                    link_children_, bind_child_, replace_unfolded_) {
        ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
        ON_CALL(get_lvc_, get(&leaf_)).WillByDefault(Return(1u));
        ON_CALL(unify_callee_, unify_callee(_, _, _, _)).WillByDefault(Return(true));
        ON_CALL(make_var_, make_var(_)).WillByDefault(Return(&var0_));
        ON_CALL(normalize_, normalize(_, _, _, _)).WillByDefault(Return(&body_));
        ON_CALL(make_inference_, make_inference(_, _, _)).WillByDefault(Return(&child_));
        ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
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
    pud_query parent_query_;
    pud_unfold_site site_;
    NiceMock<MockUnfoldSite> unfold_site_;
    NiceMock<MockUnifyCallee> unify_callee_;
    NiceMock<MockNormalize> normalize_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockGetLvc> get_lvc_;
    NiceMock<MockMakeInference> make_inference_;
    NiceMock<MockStoreAddedUnifications> store_unifs_;
    NiceMock<MockStoreAddedBodyGoals> store_goals_;
    NiceMock<MockStoreLvc> store_lvc_;
    NiceMock<MockLinkChildren> link_children_;
    NiceMock<MockBindChildInterval> bind_child_;
    NiceMock<MockReplaceUnfolded> replace_unfolded_;
    test_unfolder_t unfolder_;
};

TEST_F(PudUnfolderTest, UnfoldLinksChildRewritesQueriesAndYieldsUnit) {
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
        .WillOnce(Return(&child_));
    EXPECT_CALL(link_children_, link_children(&leaf_, ElementsAre(&child_)));
    EXPECT_CALL(bind_child_, bind_child(&child_, &leaf_));
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
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
        Return(std::vector<pud_forced_unfold>{
            pud_forced_unfold{pud_forced_unfold::refuted{&child_}}}));
    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(out.yields.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<pud_forced_unfold::refuted>(out.yields[0].content));
    EXPECT_EQ(std::get<pud_forced_unfold::refuted>(out.yields[0].content).leaf, &child_);
}

TEST_F(PudUnfolderTest, LiveCursorIsTheCalleePassedToInference) {
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
        .WillOnce(Return(&child_));
    drain(unfolder_.unfold(&leaf_, 0));
}

TEST_F(PudUnfolderTest, UnfoldCreatesOneChildPerLiveCandidate) {
    parent_query_.axiom_contexts = {
        pud_candidate_search_context{&callee_a_, {}, {&body_}},
        pud_candidate_search_context{&callee_b_, {}, {&body_}}};
    site_.callees = {&callee_a_, &callee_b_};
    ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));

    {
        InSequence seq;
        EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &callee_a_))
            .WillOnce(Return(&child_a_));
        EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &callee_b_))
            .WillOnce(Return(&child_b_));
        EXPECT_CALL(link_children_,
                    link_children(&leaf_, ElementsAre(&child_a_, &child_b_)));
        EXPECT_CALL(bind_child_, bind_child(&child_a_, &leaf_));
        EXPECT_CALL(bind_child_, bind_child(&child_b_, &leaf_));
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
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
        .WillOnce(Return(&child_));
    EXPECT_CALL(store_unifs_, store(&child_, _))
        .WillOnce(SaveArg<1>(&unifs));
    EXPECT_CALL(store_lvc_, store(&child_, _))
        .WillOnce(SaveArg<1>(&child_lvc));
    drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(unifs.size(), 2u);
    EXPECT_EQ(unifs[0].var_idx, 3u);
    EXPECT_EQ(unifs[1].var_idx, 5u);
    EXPECT_EQ(child_lvc, 1u + 3u);
}

TEST_F(PudUnfolderTest, MaterializeEmptyCandidateGoalsPassesEmptyGoals) {
    parent_query_.axiom_contexts = {
        pud_candidate_search_context{&leaf_, {}, {}}};
    std::vector<const expr*> goals;
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
        .WillOnce(Return(&child_));
    EXPECT_CALL(store_goals_, store(&child_, _))
        .WillOnce(SaveArg<1>(&goals));
    drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_TRUE(goals.empty());
}

TEST_F(PudUnfolderTest, UnfoldUsesNonzeroBodyGoalIdx) {
    pud_unfold_site site1{&parent_query_, {&leaf_}};
    ON_CALL(unfold_site_, unfold_site(&leaf_, 1)).WillByDefault(Return(site1));
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 1, &leaf_))
        .WillOnce(Return(&child_));
    EXPECT_CALL(replace_unfolded_, replace_unfolded(&leaf_, 1, ElementsAre(&child_)));
    drain(unfolder_.unfold(&leaf_, 1));
}

TEST_F(PudUnfolderTest, UnfoldYieldsNothingWhenQueriesReportEmpty) {
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));
    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_TRUE(out.yields.empty());
}

TEST_F(PudUnfolderTest, UnfoldYieldsMultipleForcedUnfoldsInOrder) {
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
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

TEST_F(PudUnfolderTest, UnfoldSequenceIsMaterializeThenLinkThenBindThenReplace) {
    for (int step = 0; step < 3; ++step) {
        InSequence seq;
        EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
            .WillOnce(Return(&child_));
        EXPECT_CALL(link_children_, link_children(&leaf_, ElementsAre(&child_)));
        EXPECT_CALL(bind_child_, bind_child(&child_, &leaf_));
        EXPECT_CALL(replace_unfolded_, replace_unfolded(&leaf_, 0, ElementsAre(&child_)))
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
    site_.callees = callee_ptrs;
    ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
    parent_query_.axiom_contexts.clear();
    for (int idx = 0; idx < 24; ++idx) {
        parent_query_.axiom_contexts.push_back(
            pud_candidate_search_context{callee_ptrs[static_cast<size_t>(idx)], {}, {&body_}});
    }
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));
    {
        InSequence seq;
        for (int idx = 0; idx < 24; ++idx) {
            EXPECT_CALL(make_inference_,
                        make_inference(&leaf_, 0, callee_ptrs[static_cast<size_t>(idx)]))
                .WillOnce(Return(child_ptrs[static_cast<size_t>(idx)]));
        }
        EXPECT_CALL(link_children_, link_children(&leaf_, child_ptrs));
        for (int idx = 0; idx < 24; ++idx) {
            EXPECT_CALL(bind_child_,
                        bind_child(child_ptrs[static_cast<size_t>(idx)], &leaf_));
        }
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
        site_.callees = callee_ptrs;
        ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
        parent_query_.axiom_contexts.clear();
        for (int idx = 0; idx < count; ++idx) {
            parent_query_.axiom_contexts.push_back(
                pud_candidate_search_context{callee_ptrs[static_cast<size_t>(idx)], {}, {&body_}});
        }
        const int kind = yield_kind(rng);
        std::vector<pud_forced_unfold> yields;
        if (kind == 1)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::unit{&child_, 0}});
        if (kind == 2)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::refuted{&child_}});
        ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(Return(yields));
        int infer_idx = 0;
        ON_CALL(make_inference_, make_inference(_, _, _)).WillByDefault(
            [&](const pud_rule_id*, size_t, const pud_rule_id*) {
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
                return yields;
            });
        const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
        EXPECT_EQ(out.children.size(), callee_ptrs.size())
            << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(linked, out.children) << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(replaced, out.children) << "seed " << k_seed << " log " << log.str();
    }
}
