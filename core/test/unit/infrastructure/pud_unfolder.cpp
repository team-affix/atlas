#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_unfolder.hpp"
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

struct MockDescent {
    struct expr_view {
        const body_goal_exprs_t* goals;
        const body_goal_exprs_t& body_goal_exprs() const { return *goals; }
    };
    struct binding_view {
        const pud_ubs_state* state;
        const pud_ubs_state& ubs_state() const { return *state; }
    };
    struct lvc_view {
        uint32_t count;
        uint32_t lvc() const { return count; }
    };

    const body_goal_exprs_t* goals;
    const pud_ubs_state* state;
    uint32_t count;
    expr_view body_goal_expr() const { return {goals}; }
    binding_view binding() const { return {state}; }
    lvc_view lvc() const { return {count}; }
};

struct MockGetNodeDescent {
    MOCK_METHOD(const MockDescent&, get, (pud_node_id));
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

using test_unfolder_t = pud_unfolder<
    NiceMock<MockGetNodeDescent>, NiceMock<MockAcceptCandidate>, NiceMock<MockMakeChild>,
    NiceMock<MockStoreCallSite>, NiceMock<MockStoreChildren>, NiceMock<MockUnsetLeaf>>;

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudUnfolderTest : public ::testing::Test {
    NiceMock<MockGetNodeDescent> get_descent;
    NiceMock<MockAcceptCandidate> acceptor;
    NiceMock<MockMakeChild> child_maker;
    NiceMock<MockStoreCallSite> call_sites;
    NiceMock<MockStoreChildren> children;
    NiceMock<MockUnsetLeaf> leaves;
    test_unfolder_t unfolder{get_descent, acceptor, child_maker, call_sites, children, leaves};

    pud_node_id leaf = 1;
    pud_body_goal_id goal = 2;
    pud_ubs_state leaf_ubs_state;
    body_goal_exprs_t leaf_goals = body_goal_exprs_t{}.set(2, expr_at(0x20)).set(3, expr_at(0x30));
    MockDescent leaf_descent{&leaf_goals, &leaf_ubs_state, 5};
    std::vector<pud_node_id> path_30{50, 60};
    std::vector<pud_node_id> path_31{50, 70};
    std::vector<std::vector<pud_node_id>> callees{path_30, path_31};
    pud_child_delta delta_60{.substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_child_delta delta_70{.substitutions = {}, .body_goals = {}, .var_count = 2};
    pud_node_id next_child = 100;

    void SetUp() override {
        ON_CALL(get_descent, get(leaf)).WillByDefault(ReturnRef(leaf_descent));
        ON_CALL(acceptor, accept(_, _, _, path_30)).WillByDefault(Return(delta_60));
        ON_CALL(acceptor, accept(_, _, _, path_31)).WillByDefault(Return(delta_70));
        ON_CALL(child_maker, make(_, _)).WillByDefault([&](pud_node_id, const pud_child_delta&) {
            return next_child++;
        });
    }
};

}

TEST_F(PudUnfolderTest, EveryCalleeIsAcceptedInTheLeafsEnvironment) {
    EXPECT_CALL(acceptor, accept(Ref(leaf_ubs_state), 5u, expr_at(0x20), path_30)).WillOnce(Return(delta_60));
    EXPECT_CALL(acceptor, accept(Ref(leaf_ubs_state), 5u, expr_at(0x20), path_31)).WillOnce(Return(delta_70));
    unfolder.unfold(leaf, goal, callees);
}

TEST_F(PudUnfolderTest, EachAcceptedDeltaBecomesAChildOfTheLeafInCalleeOrder) {
    InSequence in_order;
    EXPECT_CALL(child_maker, make(leaf, delta_60)).WillOnce(Return(100));
    EXPECT_CALL(child_maker, make(leaf, delta_70)).WillOnce(Return(101));
    EXPECT_THAT(unfolder.unfold(leaf, goal, callees), ElementsAre(100u, 101u));
}

TEST_F(PudUnfolderTest, LeafRecordsItsCallSiteAndChildrenAndStopsBeingALeaf) {
    InSequence in_order;
    EXPECT_CALL(call_sites, store(leaf, goal));
    EXPECT_CALL(children, store(leaf, ElementsAre(100u, 101u)));
    EXPECT_CALL(leaves, unset_leaf(leaf));
    unfolder.unfold(leaf, goal, callees);
}

TEST_F(PudUnfolderTest, NoCalleesLeaveTheLeafWithNoChildren) {
    EXPECT_CALL(acceptor, accept(_, _, _, _)).Times(0);
    EXPECT_CALL(children, store(leaf, ElementsAre()));
    EXPECT_CALL(leaves, unset_leaf(leaf));
    EXPECT_TRUE(unfolder.unfold(leaf, goal, {}).empty());
}
