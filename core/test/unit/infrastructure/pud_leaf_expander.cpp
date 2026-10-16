#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_leaf_expander.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using body_goal_exprs_t = immer::map<body_goal_id, const expr*>;

struct DescentOps {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, find_query_head, (int, body_goal_id));
    MOCK_METHOD(const std::vector<body_goal_id>&, refuted_goals, (int));
    MOCK_METHOD(const body_goal_exprs_t&, body_goal_exprs, (int));
    MOCK_METHOD(const pud_ubs_state&, ubs_state, (int));
    MOCK_METHOD(uint32_t, lvc, (int));
    MOCK_METHOD(int, descend, (int, pud_node_id, pud_node_id));
};

DescentOps* descent_ops = nullptr;

struct MockDescent {
    struct pq_view {
        int tag;
        std::optional<pud_mhq_head_id> find_query_head(body_goal_id goal) const {
            return descent_ops->find_query_head(tag, goal);
        }
        const std::vector<body_goal_id>& refuted_goals() const { return descent_ops->refuted_goals(tag); }
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
struct MockAcceptCandidate {
    MOCK_METHOD(pud_child_delta, accept, (const pud_ubs_state&, uint32_t, const expr*, const std::vector<pud_node_id>&));
};
struct MockMakeChild {
    MOCK_METHOD(pud_node_id, make, (pud_node_id, const pud_child_delta&));
};
struct MockStoreCallSite {
    MOCK_METHOD(void, store, (pud_node_id, size_t));
};
struct MockStoreChildren {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<pud_node_id>));
};
struct MockUnsetLeaf {
    MOCK_METHOD(void, unset_leaf, (pud_node_id));
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

using test_expander_t = pud_leaf_expander<
    NiceMock<MockGetNodeDescent>, NiceMock<MockGetQueryCandidates>, NiceMock<MockGetCandidatePath>,
    NiceMock<MockAcceptCandidate>, NiceMock<MockMakeChild>, NiceMock<MockStoreCallSite>,
    NiceMock<MockStoreChildren>, NiceMock<MockUnsetLeaf>, NiceMock<MockStoreLiveChildCount>,
    NiceMock<MockPropagateRefutation>, NiceMock<MockInvalidateLeaves>, NiceMock<MockGetQueryHeadNode>,
    NiceMock<MockRefuteNode>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockStoreNodeDescent>,
    NiceMock<MockEraseNodeDescent>>;

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

// Leaf 1 (descent 10) is expanded at goal 2, whose query head 40 has candidates 31 and 30,
// at the ends of paths {50, 70} and {50, 60}. Children are made as nodes 100, 101 with descents 1000+.
struct PudLeafExpanderTest : public ::testing::Test {
    NiceMock<DescentOps> ops;
    NiceMock<MockGetNodeDescent> get_descent;
    NiceMock<MockGetQueryCandidates> query_candidates;
    NiceMock<MockGetCandidatePath> candidate_paths;
    NiceMock<MockAcceptCandidate> acceptor;
    NiceMock<MockMakeChild> child_maker;
    NiceMock<MockStoreCallSite> call_sites;
    NiceMock<MockStoreChildren> children;
    NiceMock<MockUnsetLeaf> leaves;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    NiceMock<MockPropagateRefutation> propagator;
    NiceMock<MockInvalidateLeaves> invalidator;
    NiceMock<MockGetQueryHeadNode> head_nodes;
    NiceMock<MockRefuteNode> refuter;
    NiceMock<MockCheckNodeRefuted> refuted_nodes;
    NiceMock<MockStoreNodeDescent> store_descent;
    NiceMock<MockEraseNodeDescent> erase_descent;
    test_expander_t expander{
        get_descent, query_candidates, candidate_paths, acceptor, child_maker, call_sites,
        children, leaves, live_child_counts, propagator, invalidator, head_nodes, refuter,
        refuted_nodes, store_descent, erase_descent};

    pud_node_id leaf = 1;
    body_goal_id goal = 2;
    MockDescent leaf_descent{10};
    pud_ubs_state leaf_ubs_state;
    body_goal_exprs_t leaf_goals = body_goal_exprs_t{}.set(2, expr_at(0x20)).set(3, expr_at(0x30));
    std::unordered_set<pud_mhcs_head_id> candidates{31, 30};
    std::vector<pud_node_id> path_30{50, 60};
    std::vector<pud_node_id> path_31{50, 70};
    std::vector<body_goal_id> no_refuted_goals;
    std::vector<body_goal_id> one_refuted_goal{4};
    pud_child_delta delta_60{.substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_child_delta delta_70{.substitutions = {}, .body_goals = {}, .var_count = 2};
    pud_node_id next_child = 100;

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
        ON_CALL(acceptor, accept(_, _, _, path_30)).WillByDefault(Return(delta_60));
        ON_CALL(acceptor, accept(_, _, _, path_31)).WillByDefault(Return(delta_70));
        ON_CALL(child_maker, make(_, _)).WillByDefault([&](pud_node_id, const pud_child_delta&) {
            return next_child++;
        });
    }

    void TearDown() override { descent_ops = nullptr; }
};

// ---------- children ----------

TEST_F(PudLeafExpanderTest, EveryCandidateIsAcceptedInTheLeafsEnvironment) {
    EXPECT_CALL(acceptor, accept(Ref(leaf_ubs_state), 5u, expr_at(0x20), path_30)).WillOnce(Return(delta_60));
    EXPECT_CALL(acceptor, accept(Ref(leaf_ubs_state), 5u, expr_at(0x20), path_31)).WillOnce(Return(delta_70));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, EachAcceptedDeltaBecomesAChildOfTheLeaf) {
    InSequence in_order;
    EXPECT_CALL(child_maker, make(leaf, delta_60)).WillOnce(Return(100));
    EXPECT_CALL(child_maker, make(leaf, delta_70)).WillOnce(Return(101));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, LeafRecordsItsCallSiteChildrenAndLiveChildCount) {
    EXPECT_CALL(call_sites, store(leaf, goal));
    EXPECT_CALL(children, store(leaf, ElementsAre(100u, 101u)));
    EXPECT_CALL(live_child_counts, store(leaf, 2u));
    EXPECT_CALL(leaves, unset_leaf(leaf));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, LeafStopsBeingALeafBeforeAnyChildDescentOpensQueries) {
    InSequence in_order;
    EXPECT_CALL(leaves, unset_leaf(leaf));
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf)));
    EXPECT_CALL(ops, descend(10, leaf, _)).Times(2);
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, EveryLiveChildGetsItsOwnDescent) {
    EXPECT_CALL(ops, descend(10, leaf, 100u)).WillOnce(Return(1100));
    EXPECT_CALL(ops, descend(10, leaf, 101u)).WillOnce(Return(1101));
    EXPECT_CALL(store_descent, store(100u, _));
    EXPECT_CALL(store_descent, store(101u, _));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, ChildRefutedDuringInvalidationGetsNoDescent) {
    ON_CALL(refuted_nodes, check_refuted(100u)).WillByDefault(Return(true));
    EXPECT_CALL(ops, descend(_, _, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(ops, descend(_, _, 100u)).Times(0);
    EXPECT_CALL(store_descent, store(100u, _)).Times(0);
    EXPECT_CALL(store_descent, store(101u, _));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, LeafsDescentIsReleasedAfterItsChildrenAreStored) {
    InSequence in_order;
    EXPECT_CALL(store_descent, store(100u, _));
    EXPECT_CALL(store_descent, store(101u, _));
    EXPECT_CALL(erase_descent, erase(leaf));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, ChildArrivingWithARefutedGoalIsRefutedAfterItIsStored) {
    ON_CALL(ops, refuted_goals(1101)).WillByDefault(ReturnRef(one_refuted_goal));
    EXPECT_CALL(store_descent, store(100u, _));
    InSequence in_order;
    EXPECT_CALL(store_descent, store(101u, _));
    EXPECT_CALL(erase_descent, erase(leaf));
    EXPECT_CALL(refuter, refute(101u));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, ChildrenWithoutRefutedGoalsAreNotRefuted) {
    EXPECT_CALL(refuter, refute(_)).Times(0);
    expander.expand(leaf, goal);
}

// ---------- no candidates ----------

TEST_F(PudLeafExpanderTest, LeafWithNoCandidatesDiesWithNoChildren) {
    candidates.clear();
    EXPECT_CALL(children, store(leaf, ElementsAre()));
    EXPECT_CALL(live_child_counts, store(leaf, 0u));
    EXPECT_CALL(propagator, refute(leaf));
    EXPECT_CALL(store_descent, store(_, _)).Times(0);
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, LeafWithCandidatesDoesNotDie) {
    EXPECT_CALL(propagator, refute(_)).Times(0);
    expander.expand(leaf, goal);
}

// ---------- invalidation ----------

TEST_F(PudLeafExpanderTest, RefutedQueryFromInvalidationRefutesItsOpeningNode) {
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_refuted_query{.head_id = 50}}));
    EXPECT_CALL(head_nodes, get(50u)).WillOnce(Return(9));
    EXPECT_CALL(refuter, refute(9u));
    expander.expand(leaf, goal);
}

TEST_F(PudLeafExpanderTest, UnitQueryFromInvalidationRefutesNothing) {
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_unit_query{.head_id = 50}}));
    EXPECT_CALL(head_nodes, get(_)).Times(0);
    EXPECT_CALL(refuter, refute(_)).Times(0);
    expander.expand(leaf, goal);
}

}  // namespace
