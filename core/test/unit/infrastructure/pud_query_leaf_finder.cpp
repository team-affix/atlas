#include <map>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_leaf_finder.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};
using test_node_queries_t = pud_node_queries<NiceMock<MockRemoveQuery>>;
using test_owner_box_t = immer::box<pud_mhq_head_owner<NiceMock<MockRemoveQuery>>>;

struct MockFindQueryIntroduction {
    MOCK_METHOD(std::optional<pud_query_introduction>, find_introduction, (pud_mhq_head_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id), (const));
};
struct MockGetInference {
    MOCK_METHOD(const pud_inference&, get_inference, (pud_global_node_id), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockFindNodeQueries {
    MOCK_METHOD(const test_node_queries_t*, find, (pud_global_node_id), (const));
};

using test_leaf_finder_t = pud_query_leaf_finder<
    NiceMock<MockFindQueryIntroduction>, NiceMock<MockGetChildren>, NiceMock<MockGetInference>,
    NiceMock<MockIsNodeRefuted>, NiceMock<MockFindNodeQueries>>;

pud_inference resolved_on(pud_node_id parent, pud_body_goal_id call_site) {
    return pud_inference{
        .parent = parent, .call_site = call_site, .substitutions = {}, .body_goals = {}, .var_count = 0};
}

// Query 40 was introduced at node 1 on goal 3. Node 1 was unfolded into 2, 3 and 4, and node 3 into 5, 6 and 7.
// Leaves 2 and 6 still hold query 40; leaf 5 forked goal 3 into query 41; child 4 closed goal 3; child 7 is
// refuted. Every child was resolved on goal 0 unless a test says otherwise.
struct PudQueryLeafFinderTest : public ::testing::Test {
    PudQueryLeafFinderTest();
    NiceMock<MockRemoveQuery> remover;
    NiceMock<MockFindQueryIntroduction> introducers;
    NiceMock<MockGetChildren> children;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockFindNodeQueries> node_queries;
    test_leaf_finder_t finder;
    std::vector<pud_node_id> children_of_one;
    std::vector<pud_node_id> children_of_three;
    std::vector<pud_node_id> no_children;
    pud_inference on_goal_zero;
    pud_inference on_goal_three;
    test_node_queries_t holding;
    test_node_queries_t forked;

    void SetUp() override {
        ON_CALL(introducers, find_introduction(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(introducers, find_introduction(40))
            .WillByDefault(Return(pud_query_introduction{.node = pud_test_node(1), .goal = 3}));
        ON_CALL(children, get_children(_)).WillByDefault(ReturnRef(no_children));
        ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(children_of_one));
        ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(children_of_three));
        ON_CALL(inferences, get_inference(_)).WillByDefault(ReturnRef(on_goal_zero));
        ON_CALL(inferences, get_inference(pud_test_node(4))).WillByDefault(ReturnRef(on_goal_three));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(refuted, is_refuted(pud_test_node(7))).WillByDefault(Return(true));
        ON_CALL(node_queries, find(_)).WillByDefault(Return(nullptr));
        ON_CALL(node_queries, find(pud_test_node(2))).WillByDefault(Return(&holding));
        ON_CALL(node_queries, find(pud_test_node(5))).WillByDefault(Return(&forked));
        ON_CALL(node_queries, find(pud_test_node(6))).WillByDefault(Return(&holding));
    }

    std::vector<pud_unfold> find_all(pud_mhq_head_id head) {
        coroutine<pud_unfold, void> leaves = finder.find_leaves(head);
        std::vector<pud_unfold> found;
        for (std::optional<pud_unfold> leaf = leaves.next(); leaf.has_value(); leaf = leaves.next())
            found.push_back(leaf.value());
        return found;
    }
};

PudQueryLeafFinderTest::PudQueryLeafFinderTest()
    : remover()
    , introducers()
    , children()
    , inferences()
    , refuted()
    , node_queries()
    , finder(introducers, children, inferences, refuted, node_queries)
    , children_of_one{2, 3, 4}
    , children_of_three{5, 6, 7}
    , no_children()
    , on_goal_zero(resolved_on(1, 0))
    , on_goal_three(resolved_on(1, 3))
    , holding(test_node_queries_t{}.set(3, test_owner_box_t{remover, 40}).set(8, test_owner_box_t{remover, 42}))
    , forked(test_node_queries_t{}.set(3, test_owner_box_t{remover, 41})) {}

}

TEST_F(PudQueryLeafFinderTest, EveryLeafStillHoldingTheQueryIsFoundOnItsGoalDepthFirst) {
    EXPECT_THAT(
        find_all(40),
        ElementsAre(pud_unfold{.leaf = pud_test_node(2), .goal = 3}, pud_unfold{.leaf = pud_test_node(6), .goal = 3}));
}

TEST_F(PudQueryLeafFinderTest, AQueryWithoutAnIntroductionIsFoundOnNoLeaf) {
    EXPECT_THAT(find_all(39), IsEmpty());
}

TEST_F(PudQueryLeafFinderTest, ALiveIntroducerHoldingTheQueryIsItsOnlyLeaf) {
    ON_CALL(node_queries, find(pud_test_node(1))).WillByDefault(Return(&holding));
    EXPECT_CALL(children, get_children(_)).Times(0);
    EXPECT_THAT(find_all(40), ElementsAre(pud_unfold{.leaf = pud_test_node(1), .goal = 3}));
}

TEST_F(PudQueryLeafFinderTest, ASubtreeThatClosedTheGoalOrWasRefutedIsNotEntered) {
    EXPECT_CALL(children, get_children(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(children, get_children(pud_test_node(4))).Times(0);
    EXPECT_CALL(children, get_children(pud_test_node(7))).Times(0);
    EXPECT_CALL(node_queries, find(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(node_queries, find(pud_test_node(4))).Times(0);
    EXPECT_CALL(node_queries, find(pud_test_node(7))).Times(0);
    find_all(40);
}

TEST_F(PudQueryLeafFinderTest, TheWalkStopsWhereTheCallerStopsReading) {
    coroutine<pud_unfold, void> leaves = finder.find_leaves(40);
    EXPECT_CALL(node_queries, find(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(node_queries, find(pud_test_node(6))).Times(0);
    EXPECT_EQ(leaves.next(), (pud_unfold{.leaf = pud_test_node(2), .goal = 3}));
}
