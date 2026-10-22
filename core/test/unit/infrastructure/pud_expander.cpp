#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_expander.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {
using body_goal_exprs_t = immer::map<pud_body_goal_id, const expr*>;

struct DescentOps {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, find_query_head, (int, pud_body_goal_id));
    MOCK_METHOD(const std::vector<pud_body_goal_id>&, refuted_goals, (int));
    MOCK_METHOD(const body_goal_exprs_t&, body_goal_exprs, (int));
    MOCK_METHOD(const pud_ubs_state&, ubs_state, (int));
    MOCK_METHOD(uint32_t, lvc, (int));
    MOCK_METHOD(int, descend, (int, pud_node_id, pud_node_id));
};

DescentOps* descent_ops = nullptr;

struct MockDescent {
    struct pq_view {
        int tag;
        std::optional<pud_mhq_head_id> find_query_head(pud_body_goal_id goal) const {
            return descent_ops->find_query_head(tag, goal);
        }
        const std::vector<pud_body_goal_id>& refuted_goals() const { return descent_ops->refuted_goals(tag); }
    };
    struct expr_view {
        int tag;
        const body_goal_exprs_t& body_goal_exprs() const { return descent_ops->body_goal_exprs(tag); }
    };
    struct binding_view {
        int tag;
        const pud_ubs_state& ubs_state() const { return descent_ops->ubs_state(tag); }
    };
    struct lvc_view {
        int tag;
        uint32_t lvc() const { return descent_ops->lvc(tag); }
    };

    int tag;
    pq_view body_goal_pq() const { return {tag}; }
    expr_view body_goal_expr() const { return {tag}; }
    binding_view binding() const { return {tag}; }
    lvc_view lvc() const { return {tag}; }
    MockDescent descend(pud_node_id parent, pud_node_id child) const {
        return {descent_ops->descend(tag, parent, child)};
    }
};

struct MockGetNodeDescent {
    MOCK_METHOD(const MockDescent&, get, (pud_node_id));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(const std::unordered_set<pud_mhcs_head_id>&, get_candidates, (pud_mhq_head_id));
};
struct MockGetCandidatePath {
    MOCK_METHOD(std::vector<pud_node_id>, get_path, (pud_mhcs_head_id));
};
struct MockUnfold {
    MOCK_METHOD(std::vector<pud_node_id>, unfold, (pud_node_id, pud_body_goal_id, (const std::vector<std::vector<pud_node_id>>&)));
};
struct MockMarkLeafClosed {
    MOCK_METHOD(void, mark_leaf_closed, (pud_node_id));
};
struct MockMarkLeafOpen {
    MOCK_METHOD(void, mark_leaf_open, (pud_node_id));
};
struct MockStoreLiveChildCount {
    MOCK_METHOD(void, store, (pud_node_id, size_t));
};
struct MockPropagateRefutation {
    MOCK_METHOD(void, refute, (pud_node_id));
};
struct MockInvalidateLeaves {
    MOCK_METHOD(std::vector<pud_forced_query_propagation>, invalidate, (const std::vector<pud_node_id>&));
};
struct MockGetQueryHeadNode {
    MOCK_METHOD(pud_node_id, get, (pud_mhq_head_id));
};
struct MockRefuteNode {
    MOCK_METHOD(void, refute, (pud_node_id));
};
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};
struct MockStoreNodeDescent {
    MOCK_METHOD(void, store, (pud_node_id, MockDescent));
};
struct MockEraseNodeDescent {
    MOCK_METHOD(void, erase, (pud_node_id));
};

using test_expander_t = pud_expander<
    NiceMock<MockGetNodeDescent>, NiceMock<MockGetQueryCandidates>, NiceMock<MockGetCandidatePath>,
    NiceMock<MockUnfold>, NiceMock<MockMarkLeafClosed>,
    NiceMock<MockStoreLiveChildCount>, NiceMock<MockPropagateRefutation>,
    NiceMock<MockInvalidateLeaves>, NiceMock<MockGetQueryHeadNode>, NiceMock<MockRefuteNode>,
    NiceMock<MockCheckNodeRefuted>, NiceMock<MockStoreNodeDescent>, NiceMock<MockMarkLeafOpen>,
    NiceMock<MockEraseNodeDescent>>;

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudExpanderTest : public ::testing::Test {
    NiceMock<DescentOps> ops;
    NiceMock<MockGetNodeDescent> get_descent;
    NiceMock<MockGetQueryCandidates> query_candidates;
    NiceMock<MockGetCandidatePath> candidate_paths;
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockMarkLeafClosed> close_leaf;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    NiceMock<MockPropagateRefutation> propagator;
    NiceMock<MockInvalidateLeaves> invalidator;
    NiceMock<MockGetQueryHeadNode> head_nodes;
    NiceMock<MockRefuteNode> refuter;
    NiceMock<MockCheckNodeRefuted> refuted_nodes;
    NiceMock<MockStoreNodeDescent> store_descent;
    NiceMock<MockMarkLeafOpen> open_leaf;
    NiceMock<MockEraseNodeDescent> erase_descent;
    test_expander_t expander{
        get_descent, query_candidates, candidate_paths, unfolder, close_leaf, live_child_counts,
        propagator, invalidator, head_nodes, refuter, refuted_nodes, store_descent, open_leaf, erase_descent};

    pud_node_id leaf = 1;
    pud_body_goal_id goal = 2;
    MockDescent leaf_descent{10};
    pud_ubs_state leaf_ubs_state;
    body_goal_exprs_t leaf_goals = body_goal_exprs_t{}.set(2, expr_at(0x20)).set(3, expr_at(0x30));
    std::unordered_set<pud_mhcs_head_id> candidates{31, 30};
    std::vector<pud_node_id> path_30{50, 60};
    std::vector<pud_node_id> path_31{50, 70};
    std::vector<pud_body_goal_id> no_refuted_goals;
    std::vector<pud_body_goal_id> one_refuted_goal{4};
    body_goal_exprs_t no_goals;

    void SetUp() override {
        descent_ops = &ops;
        leaf_ubs_state.parents = leaf_ubs_state.parents.set(0, 0);
        ON_CALL(get_descent, get(leaf)).WillByDefault(ReturnRef(leaf_descent));
        ON_CALL(ops, find_query_head(10, goal)).WillByDefault(Return(40));
        ON_CALL(ops, body_goal_exprs(_)).WillByDefault(ReturnRef(leaf_goals));
        ON_CALL(ops, ubs_state(_)).WillByDefault(ReturnRef(leaf_ubs_state));
        ON_CALL(ops, lvc(_)).WillByDefault(Return(5));
        ON_CALL(ops, refuted_goals(_)).WillByDefault(ReturnRef(no_refuted_goals));
        ON_CALL(ops, descend(_, _, _)).WillByDefault([](int, pud_node_id, pud_node_id child) {
            return static_cast<int>(1000 + child);
        });
        ON_CALL(query_candidates, get_candidates(40)).WillByDefault(ReturnRef(candidates));
        ON_CALL(candidate_paths, get_path(30)).WillByDefault(Return(path_30));
        ON_CALL(candidate_paths, get_path(31)).WillByDefault(Return(path_31));
        ON_CALL(unfolder, unfold(_, _, _)).WillByDefault(
            [](pud_node_id, pud_body_goal_id, const std::vector<std::vector<pud_node_id>>& callees) {
                std::vector<pud_node_id> made;
                for (size_t index = 0; index < callees.size(); ++index)
                    made.push_back(100 + static_cast<pud_node_id>(index));
                return made;
            });
    }

    void TearDown() override { descent_ops = nullptr; }
};

TEST_F(PudExpanderTest, TheQuerysCandidatePathsAreTheCalleesInIdOrder) {
    EXPECT_CALL(unfolder, unfold(leaf, goal, ElementsAre(path_30, path_31)))
        .WillOnce(Return(std::vector<pud_node_id>{100, 101}));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ExpandReturnsTheUnfoldedChildren) {
    EXPECT_THAT(expander.expand(leaf, goal), ElementsAre(100u, 101u));
}

TEST_F(PudExpanderTest, LeafRecordsItsLiveChildCount) {
    EXPECT_CALL(live_child_counts, store(leaf, 2u));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, LeafIsUnfoldedBeforeAnyChildDescentOpensQueries) {
    InSequence in_order;
    EXPECT_CALL(unfolder, unfold(leaf, goal, _));
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf)));
    EXPECT_CALL(ops, descend(10, leaf, _)).Times(2);
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, EveryLiveChildGetsItsOwnDescent) {
    EXPECT_CALL(ops, descend(10, leaf, 100u)).WillOnce(Return(1100));
    EXPECT_CALL(ops, descend(10, leaf, 101u)).WillOnce(Return(1101));
    EXPECT_CALL(store_descent, store(100u, _));
    EXPECT_CALL(store_descent, store(101u, _));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ChildRefutedDuringInvalidationGetsNoDescent) {
    ON_CALL(refuted_nodes, check_refuted(100u)).WillByDefault(Return(true));
    EXPECT_CALL(ops, descend(_, _, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(ops, descend(_, _, 100u)).Times(0);
    EXPECT_CALL(store_descent, store(100u, _)).Times(0);
    EXPECT_CALL(store_descent, store(101u, _));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, LeafsDescentIsReleasedAfterItsChildrenAreStored) {
    InSequence in_order;
    EXPECT_CALL(store_descent, store(100u, _));
    EXPECT_CALL(store_descent, store(101u, _));
    EXPECT_CALL(erase_descent, erase(leaf));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ChildArrivingWithARefutedGoalIsRefutedAfterItIsStored) {
    ON_CALL(ops, refuted_goals(1101)).WillByDefault(ReturnRef(one_refuted_goal));
    EXPECT_CALL(store_descent, store(100u, _));
    InSequence in_order;
    EXPECT_CALL(store_descent, store(101u, _));
    EXPECT_CALL(erase_descent, erase(leaf));
    EXPECT_CALL(refuter, refute(101u));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ChildrenWithoutRefutedGoalsAreNotRefuted) {
    EXPECT_CALL(refuter, refute(_)).Times(0);
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, LeafWithNoCandidatesDiesWithNoChildren) {
    candidates.clear();
    EXPECT_CALL(live_child_counts, store(leaf, 0u));
    EXPECT_CALL(propagator, refute(leaf));
    EXPECT_CALL(store_descent, store(_, _)).Times(0);
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, LeafWithCandidatesDoesNotDie) {
    EXPECT_CALL(propagator, refute(_)).Times(0);
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, RefutedQueryFromInvalidationRefutesItsOpeningNode) {
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_refuted_query{.head_id = 50}}));
    EXPECT_CALL(head_nodes, get(50u)).WillOnce(Return(9));
    EXPECT_CALL(refuter, refute(9u));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, UnitQueryFromInvalidationRefutesNothing) {
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_unit_query{.head_id = 50}}));
    EXPECT_CALL(head_nodes, get(_)).Times(0);
    EXPECT_CALL(refuter, refute(_)).Times(0);
    expander.expand(leaf, goal);
}

}

TEST_F(PudExpanderTest, ExpandedLeafIsClosedOnceItIsUnfolded) {
    InSequence in_order;
    EXPECT_CALL(unfolder, unfold(leaf, goal, _));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf));
    EXPECT_CALL(open_leaf, mark_leaf_open(_)).Times(::testing::AnyNumber());
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ChildrenWithOpenGoalsBecomeOpenAfterTheirDescentsAreStored) {
    InSequence in_order;
    EXPECT_CALL(store_descent, store(100, _));
    EXPECT_CALL(open_leaf, mark_leaf_open(100));
    EXPECT_CALL(store_descent, store(101, _));
    EXPECT_CALL(open_leaf, mark_leaf_open(101));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ChildWithoutOpenGoalsStaysClosed) {
    ON_CALL(ops, body_goal_exprs(1101)).WillByDefault(ReturnRef(no_goals));
    EXPECT_CALL(open_leaf, mark_leaf_open(100));
    EXPECT_CALL(open_leaf, mark_leaf_open(101)).Times(0);
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, ChildWithARefutedGoalIsRefutedNotOpened) {
    ON_CALL(ops, refuted_goals(1101)).WillByDefault(ReturnRef(one_refuted_goal));
    EXPECT_CALL(open_leaf, mark_leaf_open(100));
    EXPECT_CALL(open_leaf, mark_leaf_open(101)).Times(0);
    EXPECT_CALL(refuter, refute(101));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, AlreadyRefutedChildIsNeverOpened) {
    ON_CALL(refuted_nodes, check_refuted(100)).WillByDefault(Return(true));
    EXPECT_CALL(open_leaf, mark_leaf_open(100)).Times(0);
    EXPECT_CALL(open_leaf, mark_leaf_open(101));
    expander.expand(leaf, goal);
}

TEST_F(PudExpanderTest, LeafWithoutCandidatesIsClosedAndOpensNothing) {
    candidates.clear();
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf));
    EXPECT_CALL(open_leaf, mark_leaf_open(_)).Times(0);
    expander.expand(leaf, goal);
}
