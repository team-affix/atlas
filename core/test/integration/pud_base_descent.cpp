#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_base_descent.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_body_goal_expr_descent.hpp"
#include "infrastructure/pud_body_goal_pq_descent.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_lvc_descent.hpp"
#include "infrastructure/pud_next_body_goal_id_descent.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_specialization.hpp"

struct PudBaseDescentIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool, expr_pool>;
    using binding_descent_t = pud_binding_descent<
        bind_map_t,
        pud_node_added_specializations,
        expr_pool,
        expr_pool>;
    using lvc_descent_t = pud_lvc_descent<pud_node_added_var_count>;
    using next_body_goal_id_descent_t = pud_next_body_goal_id_descent<pud_node_added_body_goals>;
    using body_goal_expr_descent_t = pud_body_goal_expr_descent<pud_node_added_body_goals, pud_call_sites>;
    using body_goal_pq_descent_t = pud_body_goal_pq_descent<pud_node_added_body_goals, pud_call_sites>;
    using base_descent_t = pud_base_descent<
        binding_descent_t,
        lvc_descent_t,
        next_body_goal_id_descent_t,
        body_goal_expr_descent_t,
        body_goal_pq_descent_t>;

    test_functors                  functors;
    expr_pool                      exprs;
    pud_node_added_specializations node_specs_;
    pud_node_added_var_count       var_counts_;
    pud_node_added_body_goals      body_goals_;
    pud_call_sites                 call_sites_;
    pud_node_id root = 0;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* goal(const char* name) {
        return functor(name, {});
    }

    void store_node(
        pud_node_id node,
        uint32_t var_count,
        std::vector<const expr*> goals,
        std::vector<pud_specialization> specs) {
        var_counts_.store(node, var_count);
        body_goals_.store(node, std::move(goals));
        node_specs_.store(node, std::move(specs));
    }

    base_descent_t at_root() {
        return base_descent_t{
            binding_descent_t{node_specs_, exprs, exprs},
            lvc_descent_t{var_counts_, root},
            next_body_goal_id_descent_t{body_goals_, root},
            body_goal_expr_descent_t{body_goals_, call_sites_, root},
            body_goal_pq_descent_t{body_goals_, call_sites_, root},
        };
    }

    const expr* resolve(const base_descent_t& at, uint32_t var_index) {
        pud_transient_ubs_state transient{at.binding().ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        return bind_map.resolve(var_index);
    }

    static std::vector<body_goal_id> goal_ids(const base_descent_t& at) {
        std::vector<body_goal_id> ids;
        for (const auto& [id, goal_expr] : at.body_goal_expr().body_goal_exprs())
            ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    static std::vector<body_goal_id> pq_ids(const base_descent_t& at) {
        std::vector<body_goal_id> ids;
        for (const auto& [id, box] : at.body_goal_pq().body_goal_pqs())
            ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    const expr* g0 = goal("g0");
    const expr* g1 = goal("g1");
    const expr* c0 = goal("c0");
    const expr* s0 = goal("s0");
    const expr* s1 = goal("s1");

    void SetUp() override {
        store_node(root, 2, {g0, g1}, {});
        call_sites_.store(root, 0);
        store_node(child, 1, {c0}, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
        call_sites_.store(child, 2);
        store_node(grandchild, 0, {}, {pud_specialization{.var_idx = 2, .value = var(1)}});
        store_node(sibling, 4, {s0, s1}, {pud_specialization{.var_idx = 0, .value = functor("b", {})}});
    }
};

TEST_F(PudBaseDescentIntegrationTest, RootStartsFromTheRootNode) {
    base_descent_t at = at_root();
    EXPECT_EQ(at.lvc().lvc(), 2u);
    EXPECT_EQ(goal_ids(at), (std::vector<body_goal_id>{0, 1}));
    EXPECT_EQ(pq_ids(at), (std::vector<body_goal_id>{0, 1}));
    EXPECT_EQ(resolve(at, 0), var(0));
}

TEST_F(PudBaseDescentIntegrationTest, ChildAppliesEveryAspect) {
    base_descent_t at_child = at_root().descend(root, child);
    EXPECT_EQ(at_child.lvc().lvc(), 3u);
    EXPECT_EQ(goal_ids(at_child), (std::vector<body_goal_id>{1, 2}));
    EXPECT_EQ(*at_child.body_goal_expr().body_goal_exprs().find(1), g1);
    EXPECT_EQ(*at_child.body_goal_expr().body_goal_exprs().find(2), c0);
    EXPECT_EQ(pq_ids(at_child), (std::vector<body_goal_id>{1, 2}));
    EXPECT_EQ(resolve(at_child, 0), functor("a", {}));
}

TEST_F(PudBaseDescentIntegrationTest, FactGrandchildClosesTheChildsGoal) {
    base_descent_t at_grandchild = at_root().descend(root, child).descend(child, grandchild);
    EXPECT_EQ(at_grandchild.lvc().lvc(), 3u);
    EXPECT_EQ(goal_ids(at_grandchild), (std::vector<body_goal_id>{1}));
    EXPECT_EQ(pq_ids(at_grandchild), (std::vector<body_goal_id>{1}));
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 2), var(1));
}

TEST_F(PudBaseDescentIntegrationTest, SiblingSeesOnlyItsOwnAspects) {
    base_descent_t root_descent = at_root();
    base_descent_t at_child = root_descent.descend(root, child);
    base_descent_t at_sibling = root_descent.descend(root, sibling);
    EXPECT_EQ(at_sibling.lvc().lvc(), 6u);
    EXPECT_EQ(goal_ids(at_sibling), (std::vector<body_goal_id>{1, 2, 3}));
    EXPECT_EQ(*at_sibling.body_goal_expr().body_goal_exprs().find(2), s0);
    EXPECT_EQ(*at_sibling.body_goal_expr().body_goal_exprs().find(3), s1);
    EXPECT_EQ(resolve(at_sibling, 0), functor("b", {}));
    EXPECT_EQ(resolve(at_child, 0), functor("a", {}));
    EXPECT_EQ(goal_ids(at_child), (std::vector<body_goal_id>{1, 2}));
}

TEST_F(PudBaseDescentIntegrationTest, RootIsUntouchedByDescending) {
    base_descent_t root_descent = at_root();
    root_descent.descend(root, child).descend(child, grandchild);
    EXPECT_EQ(root_descent.lvc().lvc(), 2u);
    EXPECT_EQ(goal_ids(root_descent), (std::vector<body_goal_id>{0, 1}));
    EXPECT_EQ(resolve(root_descent, 0), var(0));
}

TEST_F(PudBaseDescentIntegrationTest, ExprAndPqAlwaysTrackTheSameGoalIds) {
    base_descent_t root_descent = at_root();
    base_descent_t at_child = root_descent.descend(root, child);
    base_descent_t at_grandchild = at_child.descend(child, grandchild);
    base_descent_t at_sibling = root_descent.descend(root, sibling);
    EXPECT_EQ(goal_ids(root_descent), pq_ids(root_descent));
    EXPECT_EQ(goal_ids(at_child), pq_ids(at_child));
    EXPECT_EQ(goal_ids(at_grandchild), pq_ids(at_grandchild));
    EXPECT_EQ(goal_ids(at_sibling), pq_ids(at_sibling));
}

TEST_F(PudBaseDescentIntegrationTest, DeepChainKeepsEveryAspectConsistent) {
    const uint32_t depth = 100;
    const expr* left = goal("left");
    const expr* right = goal("right");
    base_descent_t at = at_root();
    pud_node_id parent_node = root;
    body_goal_id expected_last_id = 1;
    for (uint32_t level = 1; level <= depth; ++level) {
        const pud_node_id level_node = 1000 + level;
        store_node(level_node, 1, {left, right}, {pud_specialization{.var_idx = level + 1, .value = var(level)}});
        if (level > 1)
            call_sites_.store(parent_node, expected_last_id);
        at = at.descend(parent_node, level_node);
        expected_last_id += 2;
        parent_node = level_node;
    }
    EXPECT_EQ(at.lvc().lvc(), 2u + depth);
    EXPECT_EQ(goal_ids(at).size(), depth + 2u);
    EXPECT_EQ(goal_ids(at), pq_ids(at));
    EXPECT_EQ(*at.body_goal_expr().body_goal_exprs().find(expected_last_id), right);
    EXPECT_EQ(*at.body_goal_expr().body_goal_exprs().find(1), g1);
    EXPECT_EQ(resolve(at, depth + 1), var(1));
}
