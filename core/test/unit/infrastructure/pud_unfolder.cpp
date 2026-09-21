// pud_unfolder: unfold_site, materialize stores, store children/parent, store child interval, replace_unfolded.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"

using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::_;

struct MockUnfoldSite {
    MOCK_METHOD(pud_unfold_site, unfold_site, (const pud_rule_id*, size_t), ());
};

struct MockSetNormEnv {
    MOCK_METHOD(void, set_normalization_environment, (om_interval, uint32_t), ());
};

struct MockNormalize {
    MOCK_METHOD(const expr*, normalize,
                (framed_expr, (std::unordered_map<uint32_t, uint32_t>&)), ());
};

struct MockWhnf {
    MOCK_METHOD(framed_expr, whnf, (framed_expr), ());
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

struct MockGetAddedUnifications {
    MOCK_METHOD(const std::vector<pud_added_unification>&, get, (const pud_rule_id*), ());
};

struct MockStoreAddedBodyGoals {
    MOCK_METHOD(void, store, (const pud_rule_id*, (std::vector<const expr*>)), ());
};

struct MockStoreLvc {
    MOCK_METHOD(void, store, (const pud_rule_id*, uint32_t), ());
};

struct MockStoreChildren {
    MOCK_METHOD(void, store, (const pud_rule_id*, (std::set<const pud_rule_id*>)), ());
};

struct MockStoreParent {
    MOCK_METHOD(void, store, (const pud_rule_id*, const pud_rule_id*), ());
};

struct MockGetParent {
    MOCK_METHOD(const pud_rule_id*, get, (const pud_rule_id*), ());
};

struct MockGetInterval {
    MOCK_METHOD(const om_interval&, get, (const pud_rule_id*), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

struct MockStoreInterval {
    MOCK_METHOD(void, store, (const pud_rule_id*, om_interval), ());
};

struct MockRecordBinding {
    MOCK_METHOD(void, record, (om_interval, uint32_t, framed_expr), ());
};

struct MockGetAddedCallerReps {
    MOCK_METHOD(const std::vector<uint32_t>&, get, (const pud_rule_id*), ());
};

struct MockReplaceUnfolded {
    MOCK_METHOD((std::vector<pud_forced_unfold>), replace_unfolded,
                (const pud_rule_id*, size_t, (const std::vector<const pud_rule_id*>&)), ());
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockUnfoldSite>,
    NiceMock<MockSetNormEnv>,
    NiceMock<MockNormalize>,
    NiceMock<MockWhnf>,
    NiceMock<MockMakeVar>,
    NiceMock<MockGetLvc>,
    NiceMock<MockMakeInference>,
    NiceMock<MockStoreAddedUnifications>,
    NiceMock<MockGetAddedUnifications>,
    NiceMock<MockStoreAddedBodyGoals>,
    NiceMock<MockStoreLvc>,
    NiceMock<MockStoreChildren>,
    NiceMock<MockStoreParent>,
    NiceMock<MockGetParent>,
    NiceMock<MockGetInterval>,
    NiceMock<MockAllocateChildInterval>,
    NiceMock<MockStoreInterval>,
    NiceMock<MockRecordBinding>,
    NiceMock<MockGetAddedCallerReps>,
    NiceMock<MockReplaceUnfolded>>;

struct PudUnfolderTest : public ::testing::Test {
    PudUnfolderTest()
        : open_(10)
        , close_(40)
        , nested_open_(15)
        , nested_close_(20)
        , interval_{om_label(&open_), om_label(&close_)}
        , nested_{om_label(&nested_open_), om_label(&nested_close_)}
        , body_{expr::functor{1, {}}}
        , var0_{expr::var{0}}
        , leaf_{pud_rule_id::axiom{0}}
        , callee_a_{pud_rule_id::axiom{1}}
        , callee_b_{pud_rule_id::axiom{2}}
        , child_{pud_rule_id::inference{&leaf_, 0, &leaf_}}
        , child_a_{pud_rule_id::inference{&leaf_, 0, &callee_a_}}
        , child_b_{pud_rule_id::inference{&leaf_, 0, &callee_b_}}
        , live_ctx_{&leaf_, 0, &body_, 1, &leaf_, {}, {&body_}}
        , site_{&body_, {&live_ctx_}}
        , empty_caller_reps_{}
        , leaf_unifs_{{0, &body_}}
        , unfolder_(unfold_site_, set_norm_env_, normalize_, whnf_, make_var_,
                    get_lvc_, make_inference_,
                    store_unifs_, get_unifs_, store_goals_, store_lvc_,
                    store_children_, store_parent_, get_parent_, get_interval_,
                    allocate_child_, store_interval_, record_binding_,
                    get_caller_reps_, replace_unfolded_) {
        ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
        ON_CALL(get_lvc_, get(&leaf_)).WillByDefault(Return(1u));
        ON_CALL(get_interval_, get(_)).WillByDefault(ReturnRef(interval_));
        ON_CALL(allocate_child_, allocate_child_of(_)).WillByDefault(Return(nested_));
        ON_CALL(get_caller_reps_, get(_)).WillByDefault(ReturnRef(empty_caller_reps_));
        ON_CALL(get_unifs_, get(_)).WillByDefault(ReturnRef(leaf_unifs_));
        ON_CALL(get_parent_, get(_)).WillByDefault(Return(nullptr));
        ON_CALL(whnf_, whnf(_)).WillByDefault([](framed_expr fe) { return fe; });
        ON_CALL(make_var_, make_var(_)).WillByDefault(Return(&var0_));
        ON_CALL(normalize_, normalize(_, _)).WillByDefault(Return(&body_));
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
    uint64_t nested_open_;
    uint64_t nested_close_;
    om_interval interval_;
    om_interval nested_;
    expr body_;
    expr var0_;
    pud_rule_id leaf_;
    pud_rule_id callee_a_;
    pud_rule_id callee_b_;
    pud_rule_id child_;
    pud_rule_id child_a_;
    pud_rule_id child_b_;
    pud_candidate_search_context live_ctx_;
    pud_unfold_site site_;
    std::vector<uint32_t> empty_caller_reps_;
    std::vector<pud_added_unification> leaf_unifs_;
    NiceMock<MockUnfoldSite> unfold_site_;
    NiceMock<MockSetNormEnv> set_norm_env_;
    NiceMock<MockNormalize> normalize_;
    NiceMock<MockWhnf> whnf_;
    NiceMock<MockMakeVar> make_var_;
    NiceMock<MockGetLvc> get_lvc_;
    NiceMock<MockMakeInference> make_inference_;
    NiceMock<MockStoreAddedUnifications> store_unifs_;
    NiceMock<MockGetAddedUnifications> get_unifs_;
    NiceMock<MockStoreAddedBodyGoals> store_goals_;
    NiceMock<MockStoreLvc> store_lvc_;
    NiceMock<MockStoreChildren> store_children_;
    NiceMock<MockStoreParent> store_parent_;
    NiceMock<MockGetParent> get_parent_;
    NiceMock<MockGetInterval> get_interval_;
    NiceMock<MockAllocateChildInterval> allocate_child_;
    NiceMock<MockStoreInterval> store_interval_;
    NiceMock<MockRecordBinding> record_binding_;
    NiceMock<MockGetAddedCallerReps> get_caller_reps_;
    NiceMock<MockReplaceUnfolded> replace_unfolded_;
    test_unfolder_t unfolder_;
};

TEST_F(PudUnfolderTest, UnfoldLinksChildRewritesQueriesAndYieldsUnit) {
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
        .WillOnce(Return(&child_));
    EXPECT_CALL(store_interval_, store(&child_, _));
    EXPECT_CALL(store_children_, store(&leaf_, std::set<const pud_rule_id*>{&child_}));
    EXPECT_CALL(store_parent_, store(&child_, &leaf_));
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
    pud_candidate_search_context ctx_a{&leaf_, 0, &body_, 1, &callee_a_, {}, {&body_}};
    pud_candidate_search_context ctx_b{&leaf_, 0, &body_, 1, &callee_b_, {}, {&body_}};
    site_.live = {&ctx_a, &ctx_b};
    ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));

    {
        InSequence seq;
        EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &callee_a_))
            .WillOnce(Return(&child_a_));
        EXPECT_CALL(store_interval_, store(&child_a_, _));
        EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &callee_b_))
            .WillOnce(Return(&child_b_));
        EXPECT_CALL(store_interval_, store(&child_b_, _));
        EXPECT_CALL(store_children_,
                    store(&leaf_, std::set<const pud_rule_id*>{&child_a_, &child_b_}));
        EXPECT_CALL(store_parent_, store(&child_a_, &leaf_));
        EXPECT_CALL(store_parent_, store(&child_b_, &leaf_));
        EXPECT_CALL(replace_unfolded_,
                    replace_unfolded(&leaf_, 0, ElementsAre(&child_a_, &child_b_)));
    }

    const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_THAT(out.children, ElementsAre(&child_a_, &child_b_));
}

TEST_F(PudUnfolderTest, MaterializePassesTouchedRepsAsAddedUnificationsAndLiftsLvc) {
    std::vector<uint32_t> touched{3, 5};
    ON_CALL(get_caller_reps_, get(_)).WillByDefault(ReturnRef(touched));
    ON_CALL(normalize_, normalize(_, _)).WillByDefault(
        [](framed_expr, std::unordered_map<uint32_t, uint32_t>& translation) {
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
    ASSERT_EQ(unifs.size(), 3u);
    EXPECT_EQ(unifs[0].var_idx, 0u);
    EXPECT_EQ(unifs[1].var_idx, 3u);
    EXPECT_EQ(unifs[2].var_idx, 5u);
    EXPECT_EQ(child_lvc, 1u + 4u);
}

TEST_F(PudUnfolderTest, MaterializeConcatenatesAncestorThenCursorCallerReps) {
    pud_rule_id forest_parent{pud_rule_id::axiom{7}};
    pud_rule_id interned_parent{pud_rule_id::inference{&leaf_, 0, &forest_parent}};
    live_ctx_.cursor = &callee_a_;
    ON_CALL(get_parent_, get(&callee_a_)).WillByDefault(Return(&forest_parent));
    ON_CALL(get_parent_, get(&forest_parent)).WillByDefault(Return(nullptr));
    std::vector<uint32_t> parent_delta{3};
    std::vector<uint32_t> cursor_delta{5};
    ON_CALL(get_caller_reps_, get(&interned_parent))
        .WillByDefault(ReturnRef(parent_delta));
    ON_CALL(get_caller_reps_, get(&child_a_))
        .WillByDefault(ReturnRef(cursor_delta));
    ON_CALL(make_inference_, make_inference(&leaf_, 0, &callee_a_))
        .WillByDefault(Return(&child_a_));
    ON_CALL(make_inference_, make_inference(&leaf_, 0, &forest_parent))
        .WillByDefault(Return(&interned_parent));
    std::vector<pud_added_unification> unifs;
    EXPECT_CALL(store_unifs_, store(&child_a_, _))
        .WillOnce(SaveArg<1>(&unifs));
    drain(unfolder_.unfold(&leaf_, 0));
    ASSERT_EQ(unifs.size(), 3u);
    EXPECT_EQ(unifs[0].var_idx, 0u);
    EXPECT_EQ(unifs[1].var_idx, 3u);
    EXPECT_EQ(unifs[2].var_idx, 5u);
}

TEST_F(PudUnfolderTest, MaterializeEmptyCandidateGoalsPassesEmptyGoals) {
    live_ctx_.added_body_goals = {};
    std::vector<const expr*> goals;
    EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
        .WillOnce(Return(&child_));
    EXPECT_CALL(store_goals_, store(&child_, _))
        .WillOnce(SaveArg<1>(&goals));
    drain(unfolder_.unfold(&leaf_, 0));
    EXPECT_TRUE(goals.empty());
}

TEST_F(PudUnfolderTest, UnfoldUsesNonzeroBodyGoalIdx) {
    pud_unfold_site site1{&body_, {&live_ctx_}};
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

TEST_F(PudUnfolderTest, UnfoldSequenceIsMaterializeThenStoreThenReplace) {
    for (int step = 0; step < 3; ++step) {
        InSequence seq;
        EXPECT_CALL(make_inference_, make_inference(&leaf_, 0, &leaf_))
            .WillOnce(Return(&child_));
        EXPECT_CALL(store_interval_, store(&child_, _));
        EXPECT_CALL(store_children_, store(&leaf_, std::set<const pud_rule_id*>{&child_}));
        EXPECT_CALL(store_parent_, store(&child_, &leaf_));
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
    std::vector<pud_candidate_search_context> ctxs;
    ctxs.reserve(24);
    for (int idx = 0; idx < 24; ++idx) {
        ctxs.push_back(pud_candidate_search_context{
            &leaf_, 0, &body_, 1, callee_ptrs[static_cast<size_t>(idx)], {}, {&body_}});
    }
    site_.live.clear();
    for (int idx = 0; idx < 24; ++idx)
        site_.live.push_back(&ctxs[static_cast<size_t>(idx)]);
    ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
    ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
        Return(std::vector<pud_forced_unfold>{}));
    {
        InSequence seq;
        for (int idx = 0; idx < 24; ++idx) {
            EXPECT_CALL(make_inference_,
                        make_inference(&leaf_, 0, callee_ptrs[static_cast<size_t>(idx)]))
                .WillOnce(Return(child_ptrs[static_cast<size_t>(idx)]));
            EXPECT_CALL(store_interval_,
                        store(child_ptrs[static_cast<size_t>(idx)], _));
        }
        EXPECT_CALL(store_children_, store(&leaf_, std::set<const pud_rule_id*>(
            child_ptrs.begin(), child_ptrs.end())));
        for (int idx = 0; idx < 24; ++idx) {
            EXPECT_CALL(store_parent_,
                        store(child_ptrs[static_cast<size_t>(idx)], &leaf_));
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
        std::vector<pud_candidate_search_context> ctxs;
        ctxs.reserve(static_cast<size_t>(count));
        for (int idx = 0; idx < count; ++idx) {
            ctxs.push_back(pud_candidate_search_context{
                &leaf_, 0, &body_, 1, callee_ptrs[static_cast<size_t>(idx)], {}, {&body_}});
        }
        site_.live.clear();
        for (int idx = 0; idx < count; ++idx)
            site_.live.push_back(&ctxs[static_cast<size_t>(idx)]);
        ON_CALL(unfold_site_, unfold_site(&leaf_, 0)).WillByDefault(Return(site_));
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
        std::set<const pud_rule_id*> stored;
        std::vector<const pud_rule_id*> replaced;
        ON_CALL(store_children_, store(_, _)).WillByDefault(
            [&](const pud_rule_id*, std::set<const pud_rule_id*> kids) {
                stored = std::move(kids);
            });
        ON_CALL(replace_unfolded_, replace_unfolded(_, _, _)).WillByDefault(
            [&](const pud_rule_id*, size_t, const std::vector<const pud_rule_id*>& kids) {
                replaced = kids;
                return yields;
            });
        const unfold_out out = drain(unfolder_.unfold(&leaf_, 0));
        EXPECT_EQ(out.children.size(), callee_ptrs.size())
            << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(stored, std::set<const pud_rule_id*>(
            out.children.begin(), out.children.end()))
            << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(replaced, out.children) << "seed " << k_seed << " log " << log.str();
    }
}
