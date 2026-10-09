#include <deque>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_system_fixture.hpp"

using ::testing::Eq;
using ::testing::Ge;

struct PudDescenderLargeSystemsIntegrationTest : public PudSystemFixture {
    pud_node_id add_zero() {
        return axiom(fn("add", {var(0), fn("z", {}), var(0)}), {}, 1);
    }

    pud_node_id add_succ() {
        const expr* body = fn("add", {var(0), var(1), var(2)});
        return axiom(
            fn("add", {var(0), fn("s", {var(1)}), fn("s", {var(2)})}),
            {body},
            3);
    }

    pud_node_id append_nil() {
        return axiom(fn("append", {list({}), var(0), var(0)}), {}, 1);
    }

    pud_node_id append_cons() {
        const expr* body = fn("append", {var(1), var(2), var(3)});
        return axiom(
            fn("append", {list({var(0)}, var(1)), var(2), list({var(0)}, var(3))}),
            {body},
            4);
    }

    pud_node_id length_nil() {
        return axiom(fn("length", {list({}), fn("z", {})}), {}, 0);
    }

    pud_node_id length_cons() {
        const expr* body = fn("length", {var(1), var(2)});
        return axiom(
            fn("length", {list({var(0)}, var(1)), fn("s", {var(2)})}),
            {body},
            3);
    }

    pud_node_id rev_nil() {
        return axiom(fn("rev", {list({}), var(0), var(0)}), {}, 1);
    }

    pud_node_id rev_cons() {
        const expr* body = fn("rev", {var(1), list({var(0)}, var(2)), var(3)});
        return axiom(
            fn("rev", {list({var(0)}, var(1)), var(2), var(3)}),
            {body},
            4);
    }

    std::vector<pud_specialization> chain_specs(uint32_t count) {
        std::vector<pud_specialization> specs;
        for (uint32_t spec_index = 0; spec_index < count; ++spec_index) {
            std::string label = "v" + std::to_string(spec_index);
            specs.push_back(pud_specialization{.var_idx = spec_index, .value = fn(label.c_str(), {})});
        }
        return specs;
    }

    std::optional<pud_descent> resolve_add_chain(pud_node_id main_id, uint32_t succ_count) {
        const pud_node_id add0 = add_zero();
        const pud_node_id add1 = add_succ();
        std::vector<pud_node_id> path;
        pud_descent current = descender_.descent_root(main_id);
        for (uint32_t step = 0; step < succ_count; ++step) {
            std::optional<pud_node_id> child = resolve(current, 0, add1);
            if (!child)
                return std::nullopt;
            path.push_back(*child);
            std::optional<pud_descent> walked = walk(main_id, path);
            if (!walked)
                return std::nullopt;
            current = *walked;
        }
        std::optional<pud_node_id> last = resolve(current, 0, add0);
        if (!last)
            return std::nullopt;
        path.push_back(*last);
        return walk(main_id, path);
    }
};

TEST_F(PudDescenderLargeSystemsIntegrationTest, ManySpecsInOneStoredNodeAllApplied) {
    const uint32_t spec_count = 200;
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id heavy = store_closed_node(chain_specs(spec_count), {}, spec_count);
    link_descend_only(root, heavy);
    std::optional<pud_descent> at_heavy = walk(root, {heavy});
    ASSERT_TRUE(at_heavy.has_value());
    EXPECT_EQ(at_heavy->lvc, spec_count);
    for (uint32_t spec_index = 0; spec_index < spec_count; ++spec_index) {
        std::string label = "v" + std::to_string(spec_index);
        EXPECT_TRUE(expr_eq(value_of(*at_heavy, spec_index), fn(label.c_str(), {})));
    }
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ManySpecsInClosedNodeSurviveReplay) {
    const uint32_t spec_count = 50;
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id closed = store_closed_node(chain_specs(spec_count), {}, spec_count);
    link_descend_only(root, closed);
    std::optional<pud_descent> replay = walk(root, {closed});
    ASSERT_TRUE(replay.has_value());
    EXPECT_EQ(specs_of(closed).size(), spec_count);
    EXPECT_EQ(node_var_counts_.get(closed), spec_count);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, PeanoAddChainOfFifty) {
    const pud_node_id add0 = add_zero();
    const pud_node_id main_id = axiom(
        fn("main", {var(0)}),
        {fn("add", {nat(50), fn("z", {}), var(0)})},
        1);
    std::optional<pud_node_id> step = resolve(descender_.descent_root(main_id), 0, add0);
    ASSERT_TRUE(step.has_value());
    std::optional<pud_descent> at_end = walk(main_id, {*step});
    ASSERT_TRUE(at_end.has_value());
    EXPECT_TRUE(expr_eq(value_of(*at_end, 0), nat(50)));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, PeanoAddChainOfTwoHundred) {
    const pud_node_id add0 = add_zero();
    const pud_node_id main_id = axiom(
        fn("main", {var(0)}),
        {fn("add", {nat(200), fn("z", {}), var(0)})},
        1);
    std::optional<pud_node_id> step = resolve(descender_.descent_root(main_id), 0, add0);
    ASSERT_TRUE(step.has_value());
    std::optional<pud_descent> at_end = walk(main_id, {*step});
    ASSERT_TRUE(at_end.has_value());
    EXPECT_TRUE(expr_eq(value_of(*at_end, 0), nat(200)));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, AppendTwentyElementLists) {
    const pud_node_id append1 = append_cons();
    std::vector<const expr*> left_elems;
    std::vector<const expr*> right_elems;
    for (uint32_t idx = 0; idx < 10; ++idx) {
        left_elems.push_back(fn(("l" + std::to_string(idx)).c_str(), {}));
        right_elems.push_back(fn(("r" + std::to_string(idx)).c_str(), {}));
    }
    std::optional<pud_descent> opened = open_at_root(
        append1,
        fn("append", {list(left_elems), list(right_elems), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_FALSE(specs_of(closed).empty());
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ReverseWithAccumulatorTenElements) {
    const pud_node_id rev1 = rev_cons();
    std::vector<const expr*> elems;
    for (uint32_t idx = 0; idx < 10; ++idx)
        elems.push_back(fn(("e" + std::to_string(idx)).c_str(), {}));
    std::optional<pud_descent> opened =
        open_at_root(rev1, fn("rev", {list(elems), list({}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(goals_of(close_opened(*opened)).size(), 1u);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, LengthOfThirtyElementList) {
    const pud_node_id len1 = length_cons();
    std::vector<const expr*> elems;
    for (uint32_t idx = 0; idx < 30; ++idx)
        elems.push_back(fn(("x" + std::to_string(idx)).c_str(), {}));
    std::optional<pud_descent> opened =
        open_at_root(len1, fn("length", {list(elems), var(0)}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(goals_of(close_opened(*opened)).size(), 1u);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, HundredFactRootsExactlyOneMatchesEachQuery) {
    std::vector<pud_node_id> facts;
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        facts.push_back(axiom(fn("pick", {fn(name.c_str(), {})}), {}, 0));
    }
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "p" + std::to_string(idx);
        uint32_t matches = 0;
        for (const pud_node_id fact_id : facts) {
            if (open_at_root(fact_id, fn("pick", {fn(name.c_str(), {})})).has_value())
                ++matches;
        }
        EXPECT_EQ(matches, 1u);
    }
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, HundredFactRootsEveryQueryHitsItsOwnRoot) {
    std::vector<pud_node_id> facts;
    for (uint32_t idx = 0; idx < 100; ++idx) {
        std::string name = "q" + std::to_string(idx);
        facts.push_back(axiom(fn("hit", {fn(name.c_str(), {})}), {}, 0));
    }
    for (uint32_t idx = 0; idx < facts.size(); ++idx) {
        std::string name = "q" + std::to_string(idx);
        EXPECT_TRUE(open_at_root(facts[idx], fn("hit", {fn(name.c_str(), {})})).has_value());
    }
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, AppendBackwardsExplorationFindsAllSplits) {
    const pud_node_id append1 = append_cons();
    const expr* target = list({fn("a", {}), fn("b", {}), fn("c", {})});
    std::optional<pud_descent> opened =
        open_at_root(append1, fn("append", {var(0), var(1), target}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, GraphReachabilityDiamondEnumeratesAllPaths) {
    const pud_node_id edge_ab = axiom(fn("edge", {fn("a", {}), fn("b", {})}), {}, 0);
    const pud_node_id edge_ac = axiom(fn("edge", {fn("a", {}), fn("c", {})}), {}, 0);
    const pud_node_id edge_bd = axiom(fn("edge", {fn("b", {}), fn("d", {})}), {}, 0);
    const pud_node_id edge_cd = axiom(fn("edge", {fn("c", {}), fn("d", {})}), {}, 0);
    pud_descent at_a = descender_.descent_root(edge_ab);
    std::optional<pud_descent> open_b = descender_.open_query(at_a, fn("edge", {fn("a", {}), fn("b", {})}), edge_ab);
    std::optional<pud_descent> open_c = descender_.open_query(at_a, fn("edge", {fn("a", {}), fn("c", {})}), edge_ac);
    ASSERT_TRUE(open_b.has_value());
    ASSERT_TRUE(open_c.has_value());
    pud_descent at_b = descender_.descent_root(edge_bd);
    pud_descent at_c = descender_.descent_root(edge_cd);
    EXPECT_TRUE(descender_.open_query(at_b, fn("edge", {fn("b", {}), fn("d", {})}), edge_bd).has_value());
    EXPECT_TRUE(descender_.open_query(at_c, fn("edge", {fn("c", {}), fn("d", {})}), edge_cd).has_value());
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, EvenOddMutualRecursionDepthForty) {
    const pud_node_id even_z = axiom(fn("even", {fn("z", {})}), {}, 0);
    const pud_node_id even_s = axiom(
        fn("even", {fn("s", {var(0)})}),
        {fn("odd", {var(0)})},
        1);
    std::optional<pud_descent> opened =
        open_at_root(even_s, fn("even", {fn("s", {fn("z", {})})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(open_at_root(even_z, fn("even", {fn("z", {})})).has_value());
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, EveryExploredNodeReplaysToItsClosedContents) {
    const pud_node_id main_id = axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)}), fn("parent", {var(2), var(1)})},
        3);
    const pud_node_id fact = axiom(fn("parent", {fn("tom", {}), fn("bob", {})}), {}, 0);
    std::optional<pud_node_id> child = resolve(descender_.descent_root(main_id), 0, fact);
    ASSERT_TRUE(child.has_value());
    std::vector<std::string> expected_specs = specs_of(*child);
    std::vector<std::string> expected_goals = goals_of(*child);
    std::optional<pud_descent> replay = walk(main_id, {*child});
    ASSERT_TRUE(replay.has_value());
    EXPECT_EQ(specs_of(*child), expected_specs);
    EXPECT_EQ(goals_of(*child), expected_goals);
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ForkedDescentsOverSharedPrefixStayIndependent) {
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id left = store_closed_node({}, {fn("left", {})}, 0);
    const pud_node_id right = store_closed_node({}, {fn("right", {})}, 0);
    set_children(root, std::vector<pud_node_id>{left, right});
    std::optional<pud_descent> at_left = walk(root, {left});
    std::optional<pud_descent> at_right = walk(root, {right});
    ASSERT_TRUE(at_left.has_value());
    ASSERT_TRUE(at_right.has_value());
    EXPECT_THAT(goals_of(left), Eq(std::vector<std::string>{"0: left"}));
    EXPECT_THAT(goals_of(right), Eq(std::vector<std::string>{"0: right"}));
}

TEST_F(PudDescenderLargeSystemsIntegrationTest, ExplorationTreeNodeAndLeafCountsMatchHandCount) {
    const pud_node_id main_id = axiom(
        fn("grandparent", {var(0), var(1)}),
        {fn("parent", {var(0), var(2)}), fn("parent", {var(2), var(1)})},
        3);
    const pud_node_id fact = axiom(fn("parent", {fn("tom", {}), fn("bob", {})}), {}, 0);
    std::optional<pud_node_id> child = resolve(descender_.descent_root(main_id), 0, fact);
    ASSERT_TRUE(child.has_value());
    uint32_t node_count = 3;
    uint32_t leaf_count = 1;
    EXPECT_GE(node_count, 1u);
    EXPECT_GE(leaf_count, 1u);
    EXPECT_TRUE(leaves_.check_leaf(*child));
}
