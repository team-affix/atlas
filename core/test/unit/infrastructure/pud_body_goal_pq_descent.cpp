#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_body_goal_pq_descent.hpp"
#include "value_objects/pud_mhq_head_admission.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;

namespace {

struct MockOpenQuery {
    MOCK_METHOD(std::vector<int>, open, (const pud_ubs_state&, const expr*, uint32_t));
};

struct MockTryAddQueryHead {
    MOCK_METHOD(pud_mhq_head_admission, try_add_head, (const std::vector<int>&));
};

struct MockTryForkQueryHead {
    MOCK_METHOD(pud_mhq_head_admission, try_fork_head, (pud_mhq_head_id, const std::vector<int>&));
};

struct MockRemoveQueryHead {
    MOCK_METHOD(void, remove_head, (pud_mhq_head_id));
};

struct MockGetCallSite {
    MOCK_METHOD(body_goal_id, get, (pud_node_id));
};

using test_descent_t = pud_body_goal_pq_descent<
    NiceMock<MockOpenQuery>,
    NiceMock<MockTryAddQueryHead>,
    NiceMock<MockTryForkQueryHead>,
    NiceMock<MockRemoveQueryHead>,
    NiceMock<MockGetCallSite>>;

pud_mhq_head_admission live(pud_mhq_head_id head_id) {
    return {head_id, std::nullopt};
}

pud_mhq_head_admission unit(pud_mhq_head_id head_id) {
    return {head_id, pud_unit_query{head_id}};
}

pud_mhq_head_admission refuted(pud_mhq_head_id head_id) {
    return {head_id, pud_refuted_query{head_id}};
}

struct PudBodyGoalPqDescentTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    NiceMock<MockOpenQuery> open_query;
    NiceMock<MockTryAddQueryHead> try_add_query_head;
    NiceMock<MockTryForkQueryHead> try_fork_query_head;
    NiceMock<MockRemoveQueryHead> remove_query_head;
    NiceMock<MockGetCallSite> get_call_site;
    pud_node_id root = 0;
    pud_node_id child = 1;
    uint32_t root_frame_offset = 3;
    uint32_t child_frame_offset = 5;
    pud_ubs_state root_ubs_state;
    pud_ubs_state child_ubs_state;

    const expr* goal(const char* name) {
        return exprs.make_functor(functors.id(name), {});
    }

    const expr* g0 = goal("g0");
    const expr* g1 = goal("g1");
    const expr* g2 = goal("g2");
    const expr* c3 = goal("c3");
    const expr* c4 = goal("c4");
    immer::map<body_goal_id, const expr*> root_exprs =
        immer::map<body_goal_id, const expr*>{}.set(0, g0).set(1, g1).set(2, g2);
    immer::map<body_goal_id, const expr*> child_exprs =
        immer::map<body_goal_id, const expr*>{}.set(1, g1).set(2, g2).set(3, c3).set(4, c4);

    void SetUp() override {
        child_ubs_state.parents = child_ubs_state.parents.set(0, 0);
        ON_CALL(get_call_site, get(root)).WillByDefault(Return(0));
        ON_CALL(get_call_site, get(child)).WillByDefault(Return(3));
    }

    test_descent_t make_root(pud_mhq_head_admission first, pud_mhq_head_admission second, pud_mhq_head_admission third) {
        EXPECT_CALL(try_add_query_head, try_add_head(_))
            .WillOnce(Return(first))
            .WillOnce(Return(second))
            .WillOnce(Return(third));
        test_descent_t descent{
            open_query,
            try_add_query_head,
            try_fork_query_head,
            remove_query_head,
            get_call_site,
            root_exprs,
            root_ubs_state,
            root_frame_offset,
            {10, 11, 12},
        };
        Mock::VerifyAndClearExpectations(&try_add_query_head);
        return descent;
    }

    test_descent_t make_live_root() {
        return make_root(live(100), live(101), live(102));
    }

    test_descent_t descend_to_child(
        const test_descent_t& at_root,
        const std::vector<pud_mhsw_head_id>& structure_changes,
        const std::vector<pud_mhsw_head_id>& added_watch_head_ids) {
        return at_root.descend(
            3,
            root,
            child_ubs_state,
            child_frame_offset,
            child_exprs,
            structure_changes,
            added_watch_head_ids);
    }

    test_descent_t descend_below_child(
        const test_descent_t& at_child,
        const std::vector<pud_mhsw_head_id>& structure_changes) {
        return at_child.descend(5, child, child_ubs_state, child_frame_offset, child_exprs, structure_changes, {});
    }
};

// ---------- root ----------

TEST_F(PudBodyGoalPqDescentTest, RootOpensEveryRootGoalInTheRootEnvironment) {
    EXPECT_CALL(open_query, open(Ref(root_ubs_state), g0, root_frame_offset)).WillOnce(Return(std::vector<int>{0}));
    EXPECT_CALL(open_query, open(Ref(root_ubs_state), g1, root_frame_offset)).WillOnce(Return(std::vector<int>{1}));
    EXPECT_CALL(open_query, open(Ref(root_ubs_state), g2, root_frame_offset)).WillOnce(Return(std::vector<int>{2}));
    {
        InSequence in_order;
        EXPECT_CALL(try_add_query_head, try_add_head(ElementsAre(0))).WillOnce(Return(live(100)));
        EXPECT_CALL(try_add_query_head, try_add_head(ElementsAre(1))).WillOnce(Return(live(101)));
        EXPECT_CALL(try_add_query_head, try_add_head(ElementsAre(2))).WillOnce(Return(live(102)));
    }
    test_descent_t at_root{
        open_query,
        try_add_query_head,
        try_fork_query_head,
        remove_query_head,
        get_call_site,
        root_exprs,
        root_ubs_state,
        root_frame_offset,
        {10, 11, 12},
    };
}

TEST_F(PudBodyGoalPqDescentTest, RootWithNoGoalsOpensNothing) {
    EXPECT_CALL(open_query, open(_, _, _)).Times(0);
    EXPECT_CALL(try_add_query_head, try_add_head(_)).Times(0);
    test_descent_t at_root{
        open_query,
        try_add_query_head,
        try_fork_query_head,
        remove_query_head,
        get_call_site,
        {},
        root_ubs_state,
        root_frame_offset,
        {},
    };
    EXPECT_TRUE(at_root.unit_goals().empty());
    EXPECT_TRUE(at_root.refuted_goals().empty());
}

TEST_F(PudBodyGoalPqDescentTest, RootLiveGoalsAreNotForced) {
    test_descent_t at_root = make_live_root();
    EXPECT_TRUE(at_root.unit_goals().empty());
    EXPECT_TRUE(at_root.refuted_goals().empty());
}

TEST_F(PudBodyGoalPqDescentTest, RootReportsUnitAndRefutedGoals) {
    test_descent_t at_root = make_root(live(100), unit(101), refuted(102));
    EXPECT_EQ(at_root.unit_goals(), (std::vector<body_goal_id>{1}));
    EXPECT_EQ(at_root.refuted_goals(), (std::vector<body_goal_id>{2}));
}

TEST_F(PudBodyGoalPqDescentTest, RootHeadsAreRemovedWhenTheDescentDies) {
    {
        test_descent_t at_root = make_live_root();
        EXPECT_CALL(remove_query_head, remove_head(100u));
        EXPECT_CALL(remove_query_head, remove_head(101u));
        EXPECT_CALL(remove_query_head, remove_head(102u));
    }
}

TEST_F(PudBodyGoalPqDescentTest, RootUnitHeadIsStillOwned) {
    {
        test_descent_t at_root = make_root(live(100), unit(101), live(102));
        EXPECT_CALL(remove_query_head, remove_head(100u));
        EXPECT_CALL(remove_query_head, remove_head(101u));
        EXPECT_CALL(remove_query_head, remove_head(102u));
    }
}

TEST_F(PudBodyGoalPqDescentTest, RootRefutedHeadIsNotOwned) {
    {
        test_descent_t at_root = make_root(live(100), live(101), refuted(102));
        EXPECT_CALL(remove_query_head, remove_head(100u));
        EXPECT_CALL(remove_query_head, remove_head(101u));
    }
}

TEST_F(PudBodyGoalPqDescentTest, CopiesShareTheirHeads) {
    std::optional<test_descent_t> copy;
    {
        test_descent_t at_root = make_live_root();
        copy.emplace(at_root);
        EXPECT_CALL(remove_query_head, remove_head(_)).Times(0);
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(100u));
    EXPECT_CALL(remove_query_head, remove_head(101u));
    EXPECT_CALL(remove_query_head, remove_head(102u));
    copy.reset();
}

TEST_F(PudBodyGoalPqDescentTest, CallSiteWatchHeadIdComesFromTheRootGoal) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(1));
    EXPECT_EQ(at_root.find_call_site_watch_head_id(root), 11u);
}

// ---------- closing the call site ----------

TEST_F(PudBodyGoalPqDescentTest, ClosedCallSiteHeadDiesWithTheParent) {
    std::optional<test_descent_t> at_child;
    {
        test_descent_t at_root = make_live_root();
        at_child.emplace(descend_to_child(at_root, {}, {}));
        EXPECT_CALL(remove_query_head, remove_head(100u));
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(101u));
    EXPECT_CALL(remove_query_head, remove_head(102u));
    at_child.reset();
}

TEST_F(PudBodyGoalPqDescentTest, ClosedCallSiteNoLongerMapsItsWatchHead) {
    test_descent_t at_root = make_live_root();
    test_descent_t at_child = descend_to_child(at_root, {}, {});
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    descend_below_child(at_child, {10});
}

TEST_F(PudBodyGoalPqDescentTest, ClosingTheCallSiteDoesNotForkOrOpen) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(open_query, open(_, _, _)).Times(0);
    EXPECT_CALL(try_add_query_head, try_add_head(_)).Times(0);
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    descend_to_child(at_root, {}, {});
}

// ---------- unchanged goals ----------

TEST_F(PudBodyGoalPqDescentTest, UnchangedGoalsShareTheParentsHeads) {
    test_descent_t at_root = make_live_root();
    {
        test_descent_t at_child = descend_to_child(at_root, {}, {});
        EXPECT_CALL(remove_query_head, remove_head(_)).Times(0);
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
}

// ---------- changed goals ----------

TEST_F(PudBodyGoalPqDescentTest, ChangedGoalIsForkedInTheChildEnvironment) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(open_query, open(Ref(child_ubs_state), g1, child_frame_offset)).WillOnce(Return(std::vector<int>{7}));
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, ElementsAre(7))).WillOnce(Return(live(200)));
    descend_to_child(at_root, {11}, {});
}

TEST_F(PudBodyGoalPqDescentTest, OnlyChangedGoalsAreForked) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(102u, _)).WillOnce(Return(live(200)));
    descend_to_child(at_root, {12}, {});
}

TEST_F(PudBodyGoalPqDescentTest, ChangesAreForkedInReportedOrder) {
    test_descent_t at_root = make_live_root();
    InSequence in_order;
    EXPECT_CALL(try_fork_query_head, try_fork_head(102u, _)).WillOnce(Return(live(200)));
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(201)));
    descend_to_child(at_root, {12, 11}, {});
}

TEST_F(PudBodyGoalPqDescentTest, UnknownWatchHeadChangeIsIgnored) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    descend_to_child(at_root, {99}, {});
}

TEST_F(PudBodyGoalPqDescentTest, ForkedHeadShadowsTheParentsHead) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(200)));
    test_descent_t at_child = descend_to_child(at_root, {11}, {13});
    EXPECT_CALL(try_fork_query_head, try_fork_head(200u, _)).WillOnce(Return(live(300)));
    descend_below_child(at_child, {11});
}

TEST_F(PudBodyGoalPqDescentTest, ForkedHeadDiesWithTheChildWhileTheParentKeepsItsOwn) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(200)));
    {
        test_descent_t at_child = descend_to_child(at_root, {11}, {});
        EXPECT_CALL(remove_query_head, remove_head(200u));
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(201)));
    at_root.descend(3, root, child_ubs_state, child_frame_offset, child_exprs, {11}, {});
}

TEST_F(PudBodyGoalPqDescentTest, SiblingsForkFromTheSameParentHead) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(102u, _))
        .WillOnce(Return(live(200)))
        .WillOnce(Return(live(201)));
    test_descent_t at_child = descend_to_child(at_root, {12}, {});
    test_descent_t at_sibling = at_root.descend(3, root, child_ubs_state, child_frame_offset, child_exprs, {12}, {});
}

TEST_F(PudBodyGoalPqDescentTest, ChangedGoalKeepsWatchingThroughTheSameWatchHead) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(102u, _)).WillOnce(Return(live(200)));
    test_descent_t at_child = descend_to_child(at_root, {12}, {13});
    EXPECT_CALL(get_call_site, get(child)).WillOnce(Return(2));
    EXPECT_EQ(at_child.find_call_site_watch_head_id(child), 12u);
}

TEST_F(PudBodyGoalPqDescentTest, UnitForkIsReported) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(unit(200)));
    test_descent_t at_child = descend_to_child(at_root, {11}, {});
    EXPECT_EQ(at_child.unit_goals(), (std::vector<body_goal_id>{1}));
    EXPECT_TRUE(at_child.refuted_goals().empty());
}

TEST_F(PudBodyGoalPqDescentTest, UnitForkIsStillOwned) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(unit(200)));
    {
        test_descent_t at_child = descend_to_child(at_root, {11}, {});
        EXPECT_CALL(remove_query_head, remove_head(200u));
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
}

TEST_F(PudBodyGoalPqDescentTest, RefutedForkIsReported) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(refuted(200)));
    test_descent_t at_child = descend_to_child(at_root, {11}, {});
    EXPECT_EQ(at_child.refuted_goals(), (std::vector<body_goal_id>{1}));
    EXPECT_TRUE(at_child.unit_goals().empty());
}

TEST_F(PudBodyGoalPqDescentTest, RefutedForkDropsTheGoalsHeadFromTheChildOnly) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(refuted(200)));
    {
        test_descent_t at_child = descend_to_child(at_root, {11}, {});
        EXPECT_CALL(remove_query_head, remove_head(_)).Times(0);
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(201)));
    at_root.descend(3, root, child_ubs_state, child_frame_offset, child_exprs, {11}, {});
}

TEST_F(PudBodyGoalPqDescentTest, RefutedGoalIsNotForkedAgain) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(refuted(200)));
    test_descent_t at_child = descend_to_child(at_root, {11}, {});
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    descend_below_child(at_child, {11});
}

TEST_F(PudBodyGoalPqDescentTest, ForcedGoalsAreNotInherited) {
    test_descent_t at_root = make_root(live(100), unit(101), refuted(102));
    test_descent_t at_child = descend_to_child(at_root, {}, {});
    EXPECT_TRUE(at_child.unit_goals().empty());
    EXPECT_TRUE(at_child.refuted_goals().empty());
}

TEST_F(PudBodyGoalPqDescentTest, DescendLeavesTheParentsForcedGoalsUntouched) {
    test_descent_t at_root = make_root(live(100), unit(101), live(102));
    EXPECT_CALL(try_fork_query_head, try_fork_head(102u, _)).WillOnce(Return(refuted(200)));
    descend_to_child(at_root, {12}, {});
    EXPECT_EQ(at_root.unit_goals(), (std::vector<body_goal_id>{1}));
    EXPECT_TRUE(at_root.refuted_goals().empty());
}

// ---------- new goals ----------

TEST_F(PudBodyGoalPqDescentTest, NewGoalsAreOpenedInTheChildEnvironment) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(open_query, open(Ref(child_ubs_state), c3, child_frame_offset)).WillOnce(Return(std::vector<int>{3}));
    EXPECT_CALL(open_query, open(Ref(child_ubs_state), c4, child_frame_offset)).WillOnce(Return(std::vector<int>{4}));
    InSequence in_order;
    EXPECT_CALL(try_add_query_head, try_add_head(ElementsAre(3))).WillOnce(Return(live(300)));
    EXPECT_CALL(try_add_query_head, try_add_head(ElementsAre(4))).WillOnce(Return(live(301)));
    descend_to_child(at_root, {}, {13, 14});
}

TEST_F(PudBodyGoalPqDescentTest, NewGoalWatchHeadIsFindableAsACallSite) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(live(300))).WillOnce(Return(live(301)));
    test_descent_t at_child = descend_to_child(at_root, {}, {13, 14});
    EXPECT_CALL(get_call_site, get(child)).WillOnce(Return(4));
    EXPECT_EQ(at_child.find_call_site_watch_head_id(child), 14u);
}

TEST_F(PudBodyGoalPqDescentTest, NewGoalChangeBelowForksItsHead) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(live(300))).WillOnce(Return(live(301)));
    test_descent_t at_child = descend_to_child(at_root, {}, {13, 14});
    EXPECT_CALL(try_fork_query_head, try_fork_head(301u, _)).WillOnce(Return(live(400)));
    descend_below_child(at_child, {14});
}

TEST_F(PudBodyGoalPqDescentTest, NewGoalHeadsDieWithTheChild) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(live(300))).WillOnce(Return(live(301)));
    {
        test_descent_t at_child = descend_to_child(at_root, {}, {13, 14});
        EXPECT_CALL(remove_query_head, remove_head(300u));
        EXPECT_CALL(remove_query_head, remove_head(301u));
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
}

TEST_F(PudBodyGoalPqDescentTest, UnitAndRefutedNewGoalsAreReported) {
    test_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(refuted(300))).WillOnce(Return(unit(301)));
    test_descent_t at_child = descend_to_child(at_root, {}, {13, 14});
    EXPECT_EQ(at_child.unit_goals(), (std::vector<body_goal_id>{4}));
    EXPECT_EQ(at_child.refuted_goals(), (std::vector<body_goal_id>{3}));
}

TEST_F(PudBodyGoalPqDescentTest, ChangedGoalsAreForkedBeforeNewGoalsAreOpened) {
    test_descent_t at_root = make_live_root();
    InSequence in_order;
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(200)));
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(live(300)));
    descend_to_child(at_root, {11}, {13});
}

// ---------- assignability ----------

TEST_F(PudBodyGoalPqDescentTest, DescentIsAssignable) {
    test_descent_t current = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(200)));
    current = descend_to_child(current, {11}, {13});
    EXPECT_CALL(try_fork_query_head, try_fork_head(200u, _)).WillOnce(Return(live(300)));
    descend_below_child(current, {11});
}

}  // namespace
