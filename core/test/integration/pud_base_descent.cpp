#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_base_descent.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_body_goal_expr_descent.hpp"
#include "infrastructure/pud_body_goal_pq_descent.hpp"
#include "value_objects/pud_mhq_head_admission.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_lvc_descent.hpp"
#include "infrastructure/pud_next_body_goal_id_descent.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_query_forks.hpp"
#include "infrastructure/pud_query_head_nodes.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_substitution.hpp"

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
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

pud_mhq_head_admission live(pud_mhq_head_id head_id) {
    return {head_id, std::nullopt};
}

pud_mhq_head_admission unit(pud_mhq_head_id head_id) {
    return {head_id, pud_unit_query{head_id}};
}

pud_mhq_head_admission refuted(pud_mhq_head_id head_id) {
    return {head_id, pud_refuted_query{head_id}};
}

}

struct PudBaseDescentIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_bind_map<
        pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state,
        pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state>;
    using binding_descent_t = pud_binding_descent<pud_node_added_substitutions>;
    using lvc_descent_t = pud_lvc_descent<pud_node_added_var_count>;
    using next_body_goal_id_descent_t = pud_next_body_goal_id_descent<pud_node_added_body_goals>;
    using body_goal_expr_descent_t = pud_body_goal_expr_descent<pud_node_added_body_goals, pud_call_sites>;
    using structure_watch_descent_t = pud_structure_watch_descent<pud_node_added_substitutions>;
    using body_goal_pq_descent_t = pud_body_goal_pq_descent<
        NiceMock<MockOpenQuery>,
        NiceMock<MockTryAddQueryHead>,
        NiceMock<MockTryForkQueryHead>,
        NiceMock<MockRemoveQueryHead>,
        pud_call_sites,
        pud_query_head_nodes,
        pud_query_forks>;
    using base_descent_t = pud_base_descent<
        binding_descent_t,
        lvc_descent_t,
        next_body_goal_id_descent_t,
        body_goal_expr_descent_t,
        structure_watch_descent_t,
        body_goal_pq_descent_t>;

    test_functors                  functors;
    expr_pool                      exprs;
    pud_node_added_substitutions node_substitutions;
    pud_node_added_var_count       var_counts;
    pud_node_added_body_goals      body_goals;
    pud_call_sites                 call_sites;
    pud_query_head_nodes           query_head_nodes;
    pud_query_forks                query_forks;
    pud_mhq_head_id                next_unique_head_id = 1000;
    NiceMock<MockOpenQuery>        open_query;
    NiceMock<MockTryAddQueryHead>  try_add_query_head;
    NiceMock<MockTryForkQueryHead> try_fork_query_head;
    NiceMock<MockRemoveQueryHead>  remove_query_head;

    pud_node_id root = 0;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id nephew = 3;
    pud_node_id great_nephew = 4;
    pud_node_id sibling = 5;
    pud_node_id linker = 6;
    pud_node_id linked = 7;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* atom(const char* name) { return functor(name, {}); }

    const expr* p0 = functor("p", {var(0)});
    const expr* q12 = functor("q", {var(1), var(2)});
    const expr* r3 = functor("r", {var(3)});
    const expr* r1 = functor("r", {var(1)});
    const expr* s = atom("s");

    void store_node(
        pud_node_id node,
        uint32_t var_count,
        std::vector<const expr*> goals,
        std::vector<pud_substitution> substitutions) {
        var_counts.store(node, var_count);
        body_goals.store(node, std::move(goals));
        node_substitutions.store(node, std::move(substitutions));
    }

    void SetUp() override {
        ON_CALL(try_add_query_head, try_add_head(_)).WillByDefault([&](const std::vector<int>&) {
            return live(next_unique_head_id++);
        });
        ON_CALL(try_fork_query_head, try_fork_head(_, _)).WillByDefault([&](pud_mhq_head_id, const std::vector<int>&) {
            return live(next_unique_head_id++);
        });
        store_node(root, 3, {p0, q12}, {});
        call_sites.store(root, 0);
        store_node(child, 1, {r3}, {pud_substitution{.var_idx = 0, .value = atom("a")}});
        call_sites.store(child, 2);
        store_node(grandchild, 0, {}, {pud_substitution{.var_idx = 3, .value = atom("b")}});
        store_node(nephew, 0, {s}, {pud_substitution{.var_idx = 2, .value = functor("f", {var(3)})}});
        call_sites.store(nephew, 3);
        store_node(great_nephew, 0, {}, {pud_substitution{.var_idx = 1, .value = atom("c")}});
        store_node(sibling, 0, {}, {pud_substitution{.var_idx = 1, .value = var(2)}});
        store_node(linker, 0, {r1}, {});
        call_sites.store(linker, 1);
        store_node(linked, 0, {}, {pud_substitution{.var_idx = 1, .value = atom("a")}});
    }

    base_descent_t make_root(pud_mhq_head_admission first, pud_mhq_head_admission second) {
        EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(first)).WillOnce(Return(second));
        const std::vector<const expr*> root_goals{p0, q12};
        base_descent_t descent{
            binding_descent_t{node_substitutions},
            lvc_descent_t{var_counts, root},
            next_body_goal_id_descent_t{body_goals, root},
            body_goal_expr_descent_t{body_goals, call_sites, root},
            structure_watch_descent_t{node_substitutions, root_goals},
            body_goal_pq_descent_t{
                open_query,
                try_add_query_head,
                try_fork_query_head,
                remove_query_head,
                call_sites,
                query_head_nodes,
                query_forks,
                root,
                immer::map<pud_body_goal_id, const expr*>{}.set(0, p0).set(1, q12),
                pud_ubs_state{},
                3,
                {0, 1},
            },
        };
        Mock::VerifyAndClearExpectations(&try_add_query_head);
        return descent;
    }

    base_descent_t make_live_root() {
        return make_root(live(100), live(101));
    }
};

TEST_F(PudBaseDescentIntegrationTest, ChildOpensItsNewGoalAtItsFrameOffset) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(open_query, open(_, r3, 4u)).WillOnce(Return(std::vector<int>{3}));
    EXPECT_CALL(try_add_query_head, try_add_head(::testing::ElementsAre(3))).WillOnce(Return(live(102)));
    at_root.descend(root, child);
}

TEST_F(PudBaseDescentIntegrationTest, FactChildOpensNothing) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).Times(0);
    at_root.descend(root, sibling);
}

TEST_F(PudBaseDescentIntegrationTest, BindingTheClosedGoalsVariableForksNothing) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    at_root.descend(root, child);
}

TEST_F(PudBaseDescentIntegrationTest, CollapseForksTheAffectedGoal) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(open_query, open(_, q12, 3u)).WillOnce(Return(std::vector<int>{1}));
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, ::testing::ElementsAre(1))).WillOnce(Return(live(201)));
    at_root.descend(root, sibling);
}

TEST_F(PudBaseDescentIntegrationTest, InstantiationForksTheAffectedGoal) {
    base_descent_t at_root = make_live_root();
    base_descent_t at_child = at_root.descend(root, child);
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(201)));
    at_child.descend(child, nephew);
}

TEST_F(PudBaseDescentIntegrationTest, ForkedQueryShadowsTheParentsQuery) {
    base_descent_t at_root = make_live_root();
    base_descent_t at_child = at_root.descend(root, child);
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(201)));
    base_descent_t at_nephew = at_child.descend(child, nephew);
    EXPECT_CALL(try_fork_query_head, try_fork_head(201u, _)).WillOnce(Return(live(202)));
    at_nephew.descend(nephew, great_nephew);
}

TEST_F(PudBaseDescentIntegrationTest, SiblingsForkFromTheSameRootQuery) {
    base_descent_t at_root = make_live_root();
    base_descent_t at_child = at_root.descend(root, child);
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _))
        .WillOnce(Return(live(201)))
        .WillOnce(Return(live(202)));
    base_descent_t at_sibling = at_root.descend(root, sibling);
    base_descent_t at_nephew = at_child.descend(child, nephew);
}

TEST_F(PudBaseDescentIntegrationTest, NewGoalOverAnExistingVariableForksWhenThatVariableIsBound) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(live(102)));
    base_descent_t at_linker = at_root.descend(root, linker);
    EXPECT_CALL(try_fork_query_head, try_fork_head(102u, _)).WillOnce(Return(live(202)));
    at_linker.descend(linker, linked);
}

TEST_F(PudBaseDescentIntegrationTest, FactBelowClosesTheCallSiteGoalWithoutForking) {
    base_descent_t at_root = make_live_root();
    base_descent_t at_child = at_root.descend(root, child);
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    EXPECT_CALL(try_add_query_head, try_add_head(_)).Times(0);
    at_child.descend(child, grandchild);
}

TEST_F(PudBaseDescentIntegrationTest, UnitForkSurfacesThroughTheBase) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(unit(201)));
    base_descent_t at_sibling = at_root.descend(root, sibling);
    EXPECT_EQ(at_sibling.body_goal_pq().unit_goals(), (std::vector<pud_body_goal_id>{1}));
    EXPECT_TRUE(at_sibling.body_goal_pq().refuted_goals().empty());
}

TEST_F(PudBaseDescentIntegrationTest, RefutedForkSurfacesThroughTheBase) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(refuted(201)));
    base_descent_t at_sibling = at_root.descend(root, sibling);
    EXPECT_EQ(at_sibling.body_goal_pq().refuted_goals(), (std::vector<pud_body_goal_id>{1}));
    EXPECT_TRUE(at_sibling.body_goal_pq().unit_goals().empty());
}

TEST_F(PudBaseDescentIntegrationTest, RefutedNewGoalSurfacesThroughTheBase) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(refuted(102)));
    base_descent_t at_child = at_root.descend(root, child);
    EXPECT_EQ(at_child.body_goal_pq().refuted_goals(), (std::vector<pud_body_goal_id>{2}));
}

TEST_F(PudBaseDescentIntegrationTest, HeadsAreRemovedWhenTheirLastDescentDies) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).WillOnce(Return(live(102)));
    {
        base_descent_t at_child = at_root.descend(root, child);
        EXPECT_CALL(remove_query_head, remove_head(102u));
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
    EXPECT_CALL(remove_query_head, remove_head(100u));
    EXPECT_CALL(remove_query_head, remove_head(101u));
}

TEST_F(PudBaseDescentIntegrationTest, ForkedHeadIsRemovedWithItsDescentOnly) {
    base_descent_t at_root = make_live_root();
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(201)));
    {
        base_descent_t at_sibling = at_root.descend(root, sibling);
        EXPECT_CALL(remove_query_head, remove_head(201u));
    }
    Mock::VerifyAndClearExpectations(&remove_query_head);
}

TEST_F(PudBaseDescentIntegrationTest, DeepChainForksOnlyTheSurvivingGoal) {
    const uint32_t depth = 100;
    base_descent_t at = make_live_root();
    EXPECT_CALL(try_add_query_head, try_add_head(_)).Times(depth).WillRepeatedly([&](const std::vector<int>&) {
        return live(next_unique_head_id++);
    });
    EXPECT_CALL(try_fork_query_head, try_fork_head(_, _)).Times(0);
    pud_node_id parent_node = root;
    for (uint32_t level = 0; level < depth; ++level) {
        const pud_node_id level_node = 1000 + level;
        store_node(level_node, 0, {r1}, {});
        call_sites.store(level_node, 2 + level);
        at = at.descend(parent_node, level_node);
        parent_node = level_node;
    }
    Mock::VerifyAndClearExpectations(&try_fork_query_head);
    const pud_node_id last = 2000;
    store_node(last, 0, {}, {pud_substitution{.var_idx = 1, .value = atom("a")}});
    EXPECT_CALL(try_fork_query_head, try_fork_head(101u, _)).WillOnce(Return(live(600)));
    at.descend(parent_node, last);
}

TEST_F(PudBaseDescentIntegrationTest, QueriesAreRecordedAtTheNodeThatOpenedOrForkedThem) {
    base_descent_t at_root = make_live_root();
    EXPECT_EQ(query_head_nodes.get(100), root);
    EXPECT_EQ(query_head_nodes.get(101), root);
    base_descent_t at_child = at_root.descend(root, child);
    EXPECT_EQ(query_head_nodes.get(at_child.body_goal_pq().find_query_head(2).value()), child);
    EXPECT_EQ(at_child.body_goal_pq().find_query_head(1), 101u);
    base_descent_t at_nephew = at_child.descend(child, nephew);
    const pud_mhq_head_id forked = at_nephew.body_goal_pq().find_query_head(1).value();
    EXPECT_NE(forked, 101u);
    EXPECT_EQ(query_head_nodes.get(forked), nephew);
    EXPECT_EQ(query_head_nodes.get(at_nephew.body_goal_pq().find_query_head(3).value()), nephew);
}
