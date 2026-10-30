#include <algorithm>
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_demoduler.hpp"
#include "infrastructure/pud_functor_demoduler.hpp"
#include "infrastructure/pud_functor_linker.hpp"
#include "infrastructure/pud_link_table.hpp"
#include "infrastructure/pud_lodestar_runtime.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_symbol_table.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

namespace {

void append_names(pud_module& module, std::initializer_list<const char*> functor_names) {
    for (const char* functor_name : functor_names)
        module.append_functor_name(functor_name);
}

struct PudFunctorDependencyTest : public ::testing::Test {
    expr_pool exprs;

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        return exprs.make_functor(id, std::move(args));
    }

    const expr* var(uint32_t index) { return exprs.make_var(index); }
};

uint32_t arg_functor_id(const expr* term) {
    const expr::functor& head = std::get<expr::functor>(term->content);
    return std::get<expr::functor>(head.args.at(0)->content).id;
}

}

// Top, Left, and Right each store a different predicate at local id 3, applied to a constant
// that is local id 2. Bottom's goal also uses local id 3, and that name is Left's predicate.
// [] and cons occupy global ids 0 and 1, so the first name each module introduces after those
// is assigned from 2 upward in registration order.
TEST_F(PudFunctorDependencyTest, TheSameLocalIdInADiamondDoesNotUnifyDifferentNames) {
    const expr* top_fact = functor(3, {functor(2, {})});
    const expr* left_fact = functor(3, {functor(2, {})});
    const expr* right_fact = functor(3, {functor(2, {})});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(3, {var(0)});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{rule(top_fact, {}, 0)});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{rule(left_fact, {}, 0)});
    modules.emplace_back(3, std::vector<pud_module_id>{1}, std::vector<rule>{rule(right_fact, {}, 0)});
    modules.emplace_back(4, std::vector<pud_module_id>{2, 3}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "top_c", "nat"});
    append_names(modules[1], {"[]", "cons", "left_c", "left_only"});
    append_names(modules[2], {"[]", "cons", "right_c", "right_only"});
    append_names(modules[3], {"[]", "cons", "query", "left_only"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 4, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    const expr* left_c = functor(4, {});
    const expr* query = functor(8, {left_c});
    EXPECT_EQ(runtime.solution(), query);
    EXPECT_GE(runtime.solution_depth(), 1u);
    EXPECT_FALSE(runtime.next());
}

TEST_F(PudFunctorDependencyTest, TheSameNameAtDifferentLocalIdsUnifies) {
    const expr* fact = functor(3, {functor(2, {})});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(4, {var(0)});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{rule(fact, {}, 0)});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "top_c", "nat"});
    append_names(modules[1], {"[]", "cons", "query", "spare", "nat"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    EXPECT_EQ(runtime.solution(), functor(4, {functor(2, {})}));
    EXPECT_FALSE(runtime.next());
}

TEST_F(PudFunctorDependencyTest, AFactInTopIsVisibleThroughBothSidesOfTheDiamond) {
    const expr* top_fact = functor(3, {functor(2, {})});
    const expr* left_fact = functor(4, {functor(2, {})});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(3, {var(0)});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{rule(top_fact, {}, 0)});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{rule(left_fact, {}, 0)});
    modules.emplace_back(3, std::vector<pud_module_id>{1}, std::vector<rule>{});
    modules.emplace_back(4, std::vector<pud_module_id>{2, 3}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "top_c", "nat"});
    append_names(modules[1], {"[]", "cons", "left_c", "pad", "nat"});
    append_names(modules[2], {"[]", "cons", "right_c"});
    append_names(modules[3], {"[]", "cons", "query", "nat"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 4, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    ASSERT_NE(runtime.solution(), nullptr);
    const uint32_t first_arg = arg_functor_id(runtime.solution());
    ASSERT_TRUE(runtime.next());
    ASSERT_NE(runtime.solution(), nullptr);
    const uint32_t second_arg = arg_functor_id(runtime.solution());
    EXPECT_EQ(std::min(first_arg, second_arg), 2u);
    EXPECT_EQ(std::max(first_arg, second_arg), 4u);
    EXPECT_FALSE(runtime.next());
}

TEST_F(PudFunctorDependencyTest, AMiddleModulePermutesLocalIdsAndTheQueryStillSolves) {
    const expr* fact = functor(2, {functor(3, {})});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(3, {var(0)});
    std::deque<pud_module> modules;
    modules.emplace_back(10, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(11, std::vector<pud_module_id>{10}, std::vector<rule>{rule(fact, {}, 0)});
    modules.emplace_back(12, std::vector<pud_module_id>{11}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "z"});
    append_names(modules[1], {"[]", "cons", "nat", "z"});
    append_names(modules[2], {"[]", "cons", "query", "nat"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 12, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    EXPECT_EQ(runtime.solution(), functor(4, {functor(2, {})}));
}

TEST_F(PudFunctorDependencyTest, ALocalIdOfZeroIsNotTheNilGlobalId) {
    const expr* fact = functor(0, {});
    const expr* query_head = functor(2, {});
    const expr* query_body = functor(3, {});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{rule(fact, {}, 0)});
    modules.emplace_back(0, std::vector<pud_module_id>{1}, std::vector<rule>{rule(query_head, {query_body}, 0)});
    append_names(modules[0], {"odd_pred"});
    append_names(modules[1], {"[]", "cons", "query", "odd_pred"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 0, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    EXPECT_EQ(runtime.solution(), functor(3, {}));
    EXPECT_EQ(runtime.solution_depth(), 1u);
}

TEST_F(PudFunctorDependencyTest, ANestedForeignTermIsAdoptedOnceIntoTheAcceptor) {
    const expr* left_c = functor(2, {});
    const expr* pair = functor(3, {left_c, left_c});
    const expr* fact = functor(4, {pair});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(3, {var(0)});
    std::deque<pud_module> modules;
    modules.emplace_back(2, std::vector<pud_module_id>{}, std::vector<rule>{rule(fact, {}, 0)});
    modules.emplace_back(4, std::vector<pud_module_id>{2}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "left_c", "pair", "left_only"});
    append_names(modules[1], {"[]", "cons", "query", "left_only"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 4, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    EXPECT_EQ(runtime.solution(), functor(5, {functor(3, {functor(2, {}), functor(2, {})})}));
    const std::vector<std::string>& names = modules[1].functor_names();
    ASSERT_EQ(names.size(), 6u);
    EXPECT_EQ(names[4], "left_c");
    EXPECT_EQ(names[5], "pair");
    const pud_inference& inference = modules[1].get_inference(1);
    ASSERT_EQ(inference.substitutions.size(), 1u);
    const expr::functor& stored = std::get<expr::functor>(inference.substitutions[0].value->content);
    EXPECT_EQ(stored.id, 5u);
    ASSERT_EQ(stored.args.size(), 2u);
    EXPECT_EQ(std::get<expr::functor>(stored.args[0]->content).id, 4u);
    EXPECT_EQ(std::get<expr::functor>(stored.args[1]->content).id, 4u);
    EXPECT_EQ(stored.args[0], stored.args[1]);
}

TEST_F(PudFunctorDependencyTest, ACalleeBodyGoalIsStoredWithTheAcceptorsLocalIds) {
    const expr* left_c = functor(2, {});
    const expr* left_head = functor(3, {left_c});
    const expr* mark_goal = functor(4, {left_c});
    const expr* mark_fact = functor(4, {left_c});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(3, {var(0)});
    std::deque<pud_module> modules;
    // Two facts keep mark off the unit queue. One fact is proved inside the callee before the
    // acceptor runs, and the copied goal list is then empty.
    modules.emplace_back(2, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(left_head, {mark_goal}, 0),
        rule(mark_fact, {}, 0),
        rule(mark_fact, {}, 0),
    });
    modules.emplace_back(4, std::vector<pud_module_id>{2}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "left_c", "left_only", "mark"});
    append_names(modules[1], {"[]", "cons", "query", "left_only"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 4, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    EXPECT_EQ(runtime.solution(), functor(5, {functor(2, {})}));
    const std::vector<std::string>& names = modules[1].functor_names();
    ASSERT_EQ(names.size(), 6u);
    EXPECT_EQ(names[4], "left_c");
    EXPECT_EQ(names[5], "mark");
    const pud_inference& inference = modules[1].get_inference(1);
    ASSERT_EQ(inference.body_goals.size(), 1u);
    const expr::functor& stored_goal = std::get<expr::functor>(inference.body_goals[0]->content);
    EXPECT_EQ(stored_goal.id, 5u);
    ASSERT_EQ(stored_goal.args.size(), 1u);
    EXPECT_EQ(std::get<expr::functor>(stored_goal.args[0]->content).id, 4u);
}

TEST_F(PudFunctorDependencyTest, AnUnboundVariableInsideAForeignTermStaysAVariable) {
    const expr* box = functor(2, {var(0)});
    const expr* fact = functor(3, {box});
    const expr* query_head = functor(2, {var(0)});
    const expr* query_body = functor(3, {var(0)});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{rule(fact, {}, 1)});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{rule(query_head, {query_body}, 1)});
    append_names(modules[0], {"[]", "cons", "box", "holds"});
    append_names(modules[1], {"[]", "cons", "query", "holds"});
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 32, 10, 64);
    for (pud_module& module : modules)
        runtime.register_module(module);

    ASSERT_TRUE(runtime.next());
    const expr::functor& head = std::get<expr::functor>(runtime.solution()->content);
    EXPECT_EQ(head.id, 4u);
    ASSERT_EQ(head.args.size(), 1u);
    const expr::functor& stored_box = std::get<expr::functor>(head.args[0]->content);
    EXPECT_EQ(stored_box.id, 2u);
    ASSERT_EQ(stored_box.args.size(), 1u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(stored_box.args[0]->content));
}

TEST_F(PudFunctorDependencyTest, OneSkeletonDemoduledInTwoModulesKeepsEachGlobalId) {
    pud_symbol_table symbols;
    pud_link_table links;
    pud_functor_linker<pud_symbol_table, pud_link_table> linker{symbols, links};
    struct linked_demoduler {
        struct demoduler_t;
        struct functor_demoduler_t : pud_functor_demoduler<demoduler_t, pud_link_table, expr_pool> {
            using pud_functor_demoduler<demoduler_t, pud_link_table, expr_pool>::pud_functor_demoduler;
        };
        struct demoduler_t : pud_demoduler<functor_demoduler_t, expr_pool> {
            using pud_demoduler<functor_demoduler_t, expr_pool>::pud_demoduler;
        };
        functor_demoduler_t functor_demoduler;
        demoduler_t demoduler;
        linked_demoduler(pud_link_table& links, expr_pool& exprs)
            : functor_demoduler(demoduler, links, exprs)
            , demoduler(functor_demoduler, exprs) {}
    };
    linked_demoduler linked{links, exprs};
    const expr* shared = functor(2, {});
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{rule(shared, {}, 0)});
    modules.emplace_back(2, std::vector<pud_module_id>{}, std::vector<rule>{rule(shared, {}, 0)});
    append_names(modules[0], {"[]", "cons", "add"});
    append_names(modules[1], {"[]", "cons", "mul"});
    linker.link(modules[0]);
    linker.link(modules[1]);

    const expr* add = linked.demoduler.demodule(moduled_expr{shared, 0, 1});
    const expr* mul = linked.demoduler.demodule(moduled_expr{shared, 0, 2});
    EXPECT_EQ(std::get<expr::functor>(add->content).id, 2u);
    EXPECT_EQ(std::get<expr::functor>(mul->content).id, 3u);
    EXPECT_NE(add, mul);
    EXPECT_EQ(linked.demoduler.demodule(moduled_expr{shared, 0, 1}), add);
}
