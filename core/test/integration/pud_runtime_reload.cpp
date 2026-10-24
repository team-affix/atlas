#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "pud_test_node.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/rule.hpp"

using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAreArray;

namespace {

// One module of a program, before anything is derived under it.
struct module_spec {
    pud_module_id id;
    std::vector<pud_module_id> dependencies;
    std::vector<rule> axioms;
};

using program_t = std::vector<module_spec>;

// One runtime over its own modules, registered in order, and the frontier its changes describe.
struct pud_world {
    std::deque<pud_module> modules;
    pud_runtime<pud_module, expr_pool> runtime;
    std::set<pud_global_node_id> live;
    std::vector<pud_global_node_id> refuted_on_registration;

    pud_world(expr_pool& exprs)
        : modules()
        , runtime(exprs)
        , live()
        , refuted_on_registration() {}

    void register_module(pud_module module) {
        const pud_frontier_change change = runtime.register_module(modules.emplace_back(std::move(module)));
        refuted_on_registration.insert(refuted_on_registration.end(), change.refuted.begin(), change.refuted.end());
        track(change);
    }

    pud_frontier_change unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
        const pud_frontier_change change = runtime.unfold(leaf, goal);
        live.erase(leaf);
        track(change);
        return change;
    }

    void track(const pud_frontier_change& change) {
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    const pud_module& module_of(pud_module_id id) const {
        for (const pud_module& module : modules) {
            if (module.get_module_id() == id)
                return module;
        }
        throw std::out_of_range("module not in this world");
    }
};

struct unfold_step {
    pud_global_node_id leaf;
    pud_body_goal_id goal;
};

// Which goal of the chosen leaf is unfolded. Unfolding the leftmost goal first, as Prolog does,
// keeps the programs below finite when they have finitely many answers.
enum class goal_choice { random, leftmost };

// Reloading means registering copies of a world's modules, in the same order, with a fresh runtime.
// A faithful reload puts back the frontier the original reported, derives no refutation, and leaves
// the modules exactly as they were. Every frontier is also checked against the one the modules imply
// on their own: the leaves reached from every live axiom through live children.
struct PudRuntimeReloadIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    std::deque<pud_world> worlds;
    std::mt19937 rng{0};

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* peano(uint32_t value) {
        const expr* number = atom("z");
        for (uint32_t step = 0; step < value; ++step)
            number = functor("s", {number});
        return number;
    }

    const expr* list(std::vector<const expr*> items) {
        const expr* built = atom("nil");
        for (auto item = items.rbegin(); item != items.rend(); ++item)
            built = functor("cons", {*item, built});
        return built;
    }

    pud_world& build(const program_t& program) {
        pud_world& world = worlds.emplace_back(exprs);
        for (const module_spec& spec : program)
            world.register_module(pud_module(spec.id, spec.dependencies, spec.axioms));
        return world;
    }

    pud_world& reload(const pud_world& source) {
        pud_world& reloaded = worlds.emplace_back(exprs);
        for (const pud_module& module : source.modules)
            reloaded.register_module(module);
        return reloaded;
    }

    pud_goal_list goals_of(const pud_world& world, pud_global_node_id node) {
        const pud_module& module = world.module_of(node.module);
        if (node.node < module.count_axioms()) {
            pud_goal_list goals{.goals = {}, .next_id = 0};
            for (const expr* goal : module.get_axiom(node.node).body)
                goals = goals.adding(goal);
            return goals;
        }
        const pud_inference& inference = module.get_inference(node.node);
        pud_goal_list goals =
            goals_of(world, pud_global_node_id{.module = node.module, .node = inference.parent}).closing(inference.call_site);
        for (const expr* goal : inference.body_goals)
            goals = goals.adding(goal);
        return goals;
    }

    std::vector<pud_body_goal_id> goal_ids_of(const pud_world& world, pud_global_node_id node) {
        std::vector<pud_body_goal_id> ids;
        for (const auto& [id, goal] : goals_of(world, node).goals)
            ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    std::optional<unfold_step> choose_step(
        const pud_world& world, std::optional<pud_module_id> only_module, goal_choice choice) {
        std::vector<pud_global_node_id> unfoldable;
        for (pud_global_node_id leaf : world.live) {
            const bool outside = only_module.has_value() && leaf.module != only_module.value();
            if (outside || goal_ids_of(world, leaf).empty())
                continue;
            unfoldable.push_back(leaf);
        }
        if (unfoldable.empty())
            return std::nullopt;
        const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
        const std::vector<pud_body_goal_id> goals = goal_ids_of(world, leaf);
        const size_t goal_index = choice == goal_choice::leftmost ? 0 : rng() % goals.size();
        return unfold_step{.leaf = leaf, .goal = goals.at(goal_index)};
    }

    size_t unfold_steps(
        pud_world& world, size_t steps, std::optional<pud_module_id> only_module, goal_choice choice) {
        for (size_t taken = 0; taken < steps; ++taken) {
            const std::optional<unfold_step> step = choose_step(world, only_module, choice);
            if (!step.has_value())
                return taken;
            world.unfold(step->leaf, step->goal);
        }
        return steps;
    }

    size_t unfold_randomly(pud_world& world, size_t steps, std::optional<pud_module_id> only_module) {
        return unfold_steps(world, steps, only_module, goal_choice::random);
    }

    size_t unfold_to_exhaustion(pud_world& world, std::optional<pud_module_id> only_module) {
        const size_t cap = 2000;
        const size_t taken = unfold_steps(world, cap, only_module, goal_choice::leftmost);
        EXPECT_LT(taken, cap) << "the program did not run out of unfolds";
        return taken;
    }

    size_t count_nodes(const pud_world& world) {
        size_t nodes = 0;
        for (const pud_module& module : world.modules) {
            std::vector<pud_node_id> pending;
            for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom)
                pending.push_back(axiom);
            while (!pending.empty()) {
                const pud_node_id node = pending.back();
                pending.pop_back();
                ++nodes;
                pending.insert(pending.end(), module.get_children(node).begin(), module.get_children(node).end());
            }
        }
        return nodes;
    }

    // Collects the leaves a world's modules imply, and fails on a live interior node with no live child.
    std::set<pud_global_node_id> frontier_of(const pud_world& world) {
        std::set<pud_global_node_id> frontier;
        for (const pud_module& module : world.modules) {
            std::vector<pud_node_id> pending;
            for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom) {
                if (!module.check_refuted(axiom))
                    pending.push_back(axiom);
            }
            while (!pending.empty()) {
                const pud_node_id node = pending.back();
                pending.pop_back();
                const std::vector<pud_node_id>& children = module.get_children(node);
                if (children.empty()) {
                    frontier.insert(pud_global_node_id{.module = module.get_module_id(), .node = node});
                    continue;
                }
                size_t live_children = 0;
                for (pud_node_id child : children) {
                    if (module.check_refuted(child))
                        continue;
                    pending.push_back(child);
                    ++live_children;
                }
                EXPECT_GT(live_children, 0u) << "live interior node " << node << " of module " << module.get_module_id()
                                             << " has no live child";
            }
        }
        return frontier;
    }

    void expect_same_module(const pud_module& expected, const pud_module& actual) {
        ASSERT_EQ(actual.get_module_id(), expected.get_module_id());
        EXPECT_EQ(actual.get_dependencies(), expected.get_dependencies());
        ASSERT_EQ(actual.count_axioms(), expected.count_axioms());
        std::vector<pud_node_id> pending;
        for (pud_node_id axiom = 0; axiom < expected.count_axioms(); ++axiom) {
            EXPECT_EQ(actual.get_axiom(axiom), expected.get_axiom(axiom));
            pending.push_back(axiom);
        }
        while (!pending.empty()) {
            const pud_node_id node = pending.back();
            pending.pop_back();
            EXPECT_EQ(actual.check_refuted(node), expected.check_refuted(node)) << "node " << node;
            ASSERT_EQ(actual.get_children(node), expected.get_children(node)) << "node " << node;
            if (node >= expected.count_axioms())
                EXPECT_EQ(actual.get_inference(node), expected.get_inference(node)) << "node " << node;
            pending.insert(pending.end(), expected.get_children(node).begin(), expected.get_children(node).end());
        }
    }

    void expect_same_modules(const pud_world& expected, const pud_world& actual) {
        ASSERT_EQ(actual.modules.size(), expected.modules.size());
        for (size_t index = 0; index < expected.modules.size(); ++index)
            expect_same_module(expected.modules.at(index), actual.modules.at(index));
    }

    void expect_consistent(const pud_world& world) {
        EXPECT_EQ(world.live, frontier_of(world));
    }

    pud_world& expect_faithful_reload(const pud_world& source) {
        pud_world& reloaded = reload(source);
        EXPECT_EQ(reloaded.live, source.live);
        EXPECT_EQ(reloaded.live, frontier_of(reloaded));
        EXPECT_THAT(reloaded.refuted_on_registration, IsEmpty());
        expect_same_modules(source, reloaded);
        return reloaded;
    }

    // Unfolds the same leaf on the same goal in both worlds, chosen from the reference's frontier, and
    // fails on any difference. Returns whether there was a leaf left to unfold.
    bool unfold_in_lockstep(
        pud_world& reference, pud_world& resumed, std::optional<pud_module_id> only_module, goal_choice choice) {
        EXPECT_EQ(resumed.live, reference.live);
        const std::optional<unfold_step> step = choose_step(reference, only_module, choice);
        if (!step.has_value())
            return false;
        const pud_frontier_change expected = reference.unfold(step->leaf, step->goal);
        const pud_frontier_change actual = resumed.unfold(step->leaf, step->goal);
        EXPECT_EQ(actual.offspring, expected.offspring) << "unfolding (" << step->leaf.module << ", " << step->leaf.node << ") on " << step->goal;
        EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
        return true;
    }

    size_t resume_in_lockstep(
        pud_world& reference,
        pud_world& resumed,
        size_t steps,
        std::optional<pud_module_id> only_module,
        goal_choice choice) {
        size_t taken = 0;
        while (taken < steps && !HasFailure() && unfold_in_lockstep(reference, resumed, only_module, choice))
            ++taken;
        expect_same_modules(reference, resumed);
        expect_consistent(reference);
        expect_consistent(resumed);
        return taken;
    }

    program_t colors() {
        return {module_spec{.id = 1001, .dependencies = {}, .axioms = {
            rule(functor("color", {atom("red")}), {}, 0),
            rule(functor("color", {atom("blue")}), {}, 0),
            rule(functor("color", {atom("green")}), {}, 0),
            rule(functor("warm", {atom("red")}), {}, 0),
            rule(functor("warm", {atom("orange")}), {}, 0),
            rule(functor("pair", {var(0), var(1)}), {functor("color", {var(0)}), functor("color", {var(1)})}, 2),
            rule(functor("warm_color", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1),
            rule(atom("impossible"), {functor("color", {atom("purple")})}, 0),
        }}};
    }

    std::vector<rule> nat_axioms() {
        return {
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        };
    }

    program_t arithmetic() {
        std::vector<rule> base = nat_axioms();
        base.push_back(rule(functor("add", {atom("z"), var(0), var(0)}), {}, 1));
        base.push_back(rule(
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3));
        return {
            module_spec{.id = 2001, .dependencies = {}, .axioms = base},
            module_spec{.id = 2002, .dependencies = {2001}, .axioms = {
                rule(functor("half", {var(0)}), {functor("add", {var(0), var(0), peano(4)})}, 1),
                rule(functor("number", {var(0)}), {functor("nat", {var(0)})}, 1),
            }},
        };
    }

    program_t lists() {
        return {
            module_spec{.id = 3001, .dependencies = {}, .axioms = {
                rule(functor("app", {atom("nil"), var(0), var(0)}), {}, 1),
                rule(
                    functor("app", {functor("cons", {var(0), var(1)}), var(2), functor("cons", {var(0), var(3)})}),
                    {functor("app", {var(1), var(2), var(3)})},
                    4),
            }},
            module_spec{.id = 3002, .dependencies = {3001}, .axioms = {
                rule(
                    functor("split", {var(0), var(1)}),
                    {functor("app", {var(0), var(1), list({atom("a"), atom("b"), atom("c")})})},
                    2),
                rule(
                    functor("suffix", {var(0)}),
                    {functor("app", {list({atom("a")}), var(0), list({atom("a"), atom("b")})})},
                    1),
            }},
        };
    }

    program_t graph() {
        return {
            module_spec{.id = 4001, .dependencies = {}, .axioms = {
                rule(functor("edge", {atom("a"), atom("b")}), {}, 0),
                rule(functor("edge", {atom("b"), atom("c")}), {}, 0),
                rule(functor("edge", {atom("c"), atom("d")}), {}, 0),
                rule(functor("edge", {atom("b"), atom("e")}), {}, 0),
                rule(functor("path", {var(0), var(1)}), {functor("edge", {var(0), var(1)})}, 2),
                rule(
                    functor("path", {var(0), var(2)}),
                    {functor("edge", {var(0), var(1)}), functor("path", {var(1), var(2)})},
                    3),
            }},
            module_spec{.id = 4002, .dependencies = {4001}, .axioms = {
                rule(functor("reach", {var(0)}), {functor("path", {atom("a"), var(0)})}, 1),
                rule(atom("unreachable"), {functor("path", {atom("d"), atom("a")})}, 0),
            }},
        };
    }

    program_t tiers() {
        return {
            module_spec{.id = 5001, .dependencies = {}, .axioms = {
                rule(functor("p", {atom("a")}), {}, 0),
                rule(functor("p", {atom("b")}), {}, 0),
            }},
            module_spec{.id = 5002, .dependencies = {5001}, .axioms = {
                rule(functor("r", {var(0)}), {functor("p", {var(0)})}, 1),
                rule(functor("r", {atom("c")}), {}, 0),
            }},
            module_spec{.id = 5003, .dependencies = {5002}, .axioms = {
                rule(functor("q", {var(0)}), {functor("r", {var(0)}), functor("p", {var(0)})}, 1),
                rule(functor("only_c", {var(0)}), {functor("r", {var(0)}), functor("s", {var(0)})}, 1),
            }},
        };
    }

    program_t siblings() {
        return {
            module_spec{.id = 6001, .dependencies = {}, .axioms = nat_axioms()},
            module_spec{.id = 6002, .dependencies = {6001}, .axioms = {
                rule(functor("even", {atom("z")}), {}, 0),
                rule(functor("even", {functor("s", {functor("s", {var(0)})})}), {functor("even", {var(0)})}, 1),
            }},
            module_spec{.id = 6003, .dependencies = {6001}, .axioms = {
                rule(functor("odd", {peano(1)}), {}, 0),
                rule(functor("odd", {functor("s", {functor("s", {var(0)})})}), {functor("odd", {var(0)})}, 1),
            }},
            module_spec{.id = 6004, .dependencies = {6002, 6003}, .axioms = {
                rule(functor("even_nat", {var(0)}), {functor("even", {var(0)}), functor("nat", {var(0)})}, 1),
                rule(functor("odd_nat", {var(0)}), {functor("odd", {var(0)}), functor("nat", {var(0)})}, 1),
            }},
        };
    }

    program_t refutations() {
        return {module_spec{.id = 7001, .dependencies = {}, .axioms = {
            rule(functor("a", {atom("one")}), {}, 0),
            rule(functor("a", {atom("two")}), {}, 0),
            rule(functor("b", {atom("two")}), {}, 0),
            rule(functor("d", {atom("one")}), {}, 0),
            rule(functor("c", {var(0)}), {functor("a", {var(0)}), functor("b", {var(0)}), functor("d", {var(0)})}, 1),
            rule(functor("e", {var(0)}), {functor("c", {var(0)})}, 1),
            rule(atom("f"), {atom("missing")}, 0),
            rule(functor("g", {var(0)}), {functor("a", {var(0)}), functor("missing", {var(0)})}, 1),
            rule(functor("h", {var(0)}), {functor("a", {var(0)}), functor("d", {var(0)})}, 1),
        }}};
    }

    program_t cascade() {
        return {
            module_spec{.id = 8001, .dependencies = {}, .axioms = {
                rule(functor("base", {atom("c")}), {}, 0),
                rule(functor("check", {atom("a")}), {}, 0),
                rule(functor("mid", {var(0)}), {functor("base", {var(0)}), functor("check", {var(0)})}, 1),
            }},
            module_spec{.id = 8002, .dependencies = {8001}, .axioms = {
                rule(atom("top"), {functor("mid", {atom("c")})}, 0),
                rule(atom("other"), {functor("check", {atom("a")})}, 0),
            }},
        };
    }

    std::vector<program_t> every_program() {
        return {colors(), arithmetic(), lists(), graph(), tiers(), siblings(), refutations(), cascade()};
    }
};

}

// Reloading: the frontier, the refutations and the modules come back exactly as they were left.

TEST_F(PudRuntimeReloadIntegrationTest, AModuleWithoutAxiomsReloadsToAnEmptyFrontier) {
    pud_world& original = build({module_spec{.id = 9, .dependencies = {}, .axioms = {}}});
    EXPECT_THAT(expect_faithful_reload(original).live, IsEmpty());
}

TEST_F(PudRuntimeReloadIntegrationTest, FactsAloneReloadAsTheirOwnFrontier) {
    pud_world& original = build({module_spec{.id = 10, .dependencies = {}, .axioms = {
        rule(functor("p", {atom("a")}), {}, 0),
        rule(functor("p", {atom("b")}), {}, 0),
        rule(functor("q", {atom("a")}), {}, 0),
    }}});
    EXPECT_EQ(expect_faithful_reload(original).live.size(), 3u);
}

TEST_F(PudRuntimeReloadIntegrationTest, AxiomsWithNothingDerivedReload) {
    pud_world& original = build(colors());
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, ARefutationFoundOnRegistrationIsNotFoundAgainOnReload) {
    pud_world& original = build(refutations());
    ASSERT_THAT(original.refuted_on_registration, Not(IsEmpty()));
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, AFewUnfoldsReload) {
    pud_world& original = build(colors());
    ASSERT_EQ(unfold_randomly(original, 3, std::nullopt), 3u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, AProgramUnfoldedToExhaustionReloads) {
    pud_world& original = build(colors());
    EXPECT_GT(unfold_to_exhaustion(original, std::nullopt), 5u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, AFrontierOfOnlySolvedLeavesReloads) {
    pud_world& original = build(colors());
    unfold_to_exhaustion(original, std::nullopt);
    for (pud_global_node_id leaf : original.live)
        EXPECT_THAT(goal_ids_of(original, leaf), IsEmpty());
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, ADeeplyUnfoldedDatabaseReloads) {
    pud_world& original = build(arithmetic());
    pud_global_node_id deepest{.module = 2001, .node = 1};
    pud_body_goal_id goal = 0;
    for (int level = 0; level < 12; ++level) {
        const pud_frontier_change change = original.unfold(deepest, goal);
        ASSERT_FALSE(change.offspring.empty()) << "level " << level;
        deepest = change.offspring.back();
        ASSERT_THAT(goal_ids_of(original, deepest), Not(IsEmpty())) << "level " << level;
        goal = goal_ids_of(original, deepest).front();
    }
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, ManyRandomUnfoldsOfArithmeticReload) {
    rng.seed(1);
    pud_world& original = build(arithmetic());
    ASSERT_EQ(unfold_randomly(original, 40, std::nullopt), 40u);
    EXPECT_GT(count_nodes(original), 60u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, UnfoldingOnlyTheQueriesOfArithmeticReloads) {
    rng.seed(2);
    pud_world& original = build(arithmetic());
    ASSERT_EQ(unfold_randomly(original, 30, 2002), 30u);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, RandomUnfoldsOfListsReload) {
    rng.seed(3);
    pud_world& original = build(lists());
    ASSERT_EQ(unfold_randomly(original, 30, std::nullopt), 30u);
    EXPECT_GT(count_nodes(original), 40u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, ListQueriesUnfoldedToExhaustionReload) {
    rng.seed(4);
    pud_world& original = build(lists());
    EXPECT_GT(unfold_to_exhaustion(original, 3002), 3u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, GraphQueriesUnfoldedToExhaustionReload) {
    rng.seed(5);
    pud_world& original = build(graph());
    EXPECT_GT(unfold_to_exhaustion(original, 4002), 5u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, AGraphUnfoldedEverywhereReloads) {
    rng.seed(6);
    pud_world& original = build(graph());
    EXPECT_GT(unfold_randomly(original, 40, std::nullopt), 20u);
    EXPECT_GT(count_nodes(original), 50u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, TiersUnfoldedOnlyInTheBottomModuleReload) {
    rng.seed(7);
    pud_world& original = build(tiers());
    EXPECT_GT(unfold_randomly(original, 10, 5002), 0u);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, TiersUnfoldedOnlyInTheTopModuleReload) {
    rng.seed(8);
    pud_world& original = build(tiers());
    EXPECT_GT(unfold_to_exhaustion(original, 5003), 1u);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, TiersUnfoldedEverywhereReload) {
    rng.seed(9);
    pud_world& original = build(tiers());
    EXPECT_GT(unfold_to_exhaustion(original, std::nullopt), 2u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, SiblingModulesOverOneBaseReload) {
    rng.seed(10);
    pud_world& original = build(siblings());
    ASSERT_EQ(unfold_randomly(original, 40, std::nullopt), 40u);
    EXPECT_GT(count_nodes(original), 50u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, CascadingRefutationsReload) {
    rng.seed(11);
    pud_world& original = build(refutations());
    EXPECT_GT(unfold_to_exhaustion(original, std::nullopt), 2u);
    expect_consistent(original);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, NothingBelowARefutedInteriorNodeComesBackOnReload) {
    pud_world& original = build(cascade());
    original.unfold(pud_global_node_id{.module = 8001, .node = 2}, 0);
    const pud_module& base = original.module_of(8001);
    ASSERT_TRUE(base.check_refuted(2));
    ASSERT_THAT(base.get_children(2), Not(IsEmpty()));
    const pud_world& reloaded = expect_faithful_reload(original);
    for (pud_node_id child : base.get_children(2))
        EXPECT_FALSE(reloaded.live.contains(pud_global_node_id{.module = 8001, .node = child}));
}

TEST_F(PudRuntimeReloadIntegrationTest, ARefutationThatCrossedModulesStaysOnReload) {
    pud_world& original = build(cascade());
    original.unfold(pud_global_node_id{.module = 8001, .node = 2}, 0);
    ASSERT_TRUE(original.module_of(8002).check_refuted(0));
    const pud_world& reloaded = expect_faithful_reload(original);
    EXPECT_FALSE(reloaded.live.contains(pud_global_node_id{.module = 8002, .node = 0}));
    EXPECT_TRUE(reloaded.live.contains(pud_global_node_id{.module = 8002, .node = 1}));
}

TEST_F(PudRuntimeReloadIntegrationTest, AReloadOfAReloadIsFaithfulToo) {
    rng.seed(12);
    pud_world& original = build(lists());
    unfold_randomly(original, 25, std::nullopt);
    const pud_world& first = expect_faithful_reload(original);
    const pud_world& second = expect_faithful_reload(first);
    EXPECT_EQ(second.live, original.live);
    expect_same_modules(original, second);
}

TEST_F(PudRuntimeReloadIntegrationTest, ReloadingTheSameModulesManyTimesNeverChangesThem) {
    rng.seed(13);
    pud_world& original = build(graph());
    unfold_randomly(original, 30, std::nullopt);
    for (int reload_index = 0; reload_index < 6; ++reload_index)
        expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, ModuleIdsAreOnlyNames) {
    const pud_module_id large = std::numeric_limits<pud_module_id>::max() - 3;
    pud_world& original = build({
        module_spec{.id = large, .dependencies = {}, .axioms = nat_axioms()},
        module_spec{.id = 0, .dependencies = {large}, .axioms = {
            rule(functor("number", {var(0)}), {functor("nat", {var(0)})}, 1),
        }},
    });
    rng.seed(14);
    unfold_randomly(original, 20, std::nullopt);
    expect_faithful_reload(original);
}

TEST_F(PudRuntimeReloadIntegrationTest, EveryProgramReloadsFaithfullyAfterAnyNumberOfRandomUnfolds) {
    for (uint32_t seed = 0; seed < 6 && !HasFailure(); ++seed) {
        for (const program_t& program : every_program()) {
            rng.seed(seed);
            pud_world& original = build(program);
            unfold_randomly(original, rng() % 30, std::nullopt);
            expect_consistent(original);
            expect_faithful_reload(original);
            if (HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}

// Resuming: a reloaded runtime unfolds exactly as the original goes on unfolding.

TEST_F(PudRuntimeReloadIntegrationTest, ResumingFromAxiomsAloneIsLikeNeverHavingReloaded) {
    rng.seed(20);
    pud_world& original = build(arithmetic());
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_EQ(resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TEST_F(PudRuntimeReloadIntegrationTest, AResumedProgramUnfoldsToExhaustionLikeTheOriginal) {
    rng.seed(21);
    pud_world& original = build(colors());
    unfold_randomly(original, 4, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_LT(resume_in_lockstep(original, resumed, 2000, std::nullopt, goal_choice::leftmost), 2000u);
    EXPECT_FALSE(choose_step(original, std::nullopt, goal_choice::leftmost).has_value());
    EXPECT_FALSE(choose_step(resumed, std::nullopt, goal_choice::leftmost).has_value());
}

TEST_F(PudRuntimeReloadIntegrationTest, ResumedArithmeticUnfoldsLikeTheOriginal) {
    rng.seed(22);
    pud_world& original = build(arithmetic());
    unfold_randomly(original, 20, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_EQ(resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TEST_F(PudRuntimeReloadIntegrationTest, ResumedListsUnfoldLikeTheOriginal) {
    rng.seed(23);
    pud_world& original = build(lists());
    unfold_randomly(original, 15, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_EQ(resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TEST_F(PudRuntimeReloadIntegrationTest, ResumedGraphQueriesUnfoldToExhaustionLikeTheOriginal) {
    rng.seed(24);
    pud_world& original = build(graph());
    unfold_randomly(original, 10, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_LT(resume_in_lockstep(original, resumed, 2000, 4002, goal_choice::leftmost), 2000u);
    EXPECT_FALSE(choose_step(original, 4002, goal_choice::leftmost).has_value());
    EXPECT_FALSE(choose_step(resumed, 4002, goal_choice::leftmost).has_value());
}

TEST_F(PudRuntimeReloadIntegrationTest, ResumedTiersUnfoldLikeTheOriginal) {
    rng.seed(25);
    pud_world& original = build(tiers());
    unfold_randomly(original, 3, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_LT(resume_in_lockstep(original, resumed, 2000, std::nullopt, goal_choice::leftmost), 2000u);
}

TEST_F(PudRuntimeReloadIntegrationTest, ResumedSiblingModulesUnfoldLikeTheOriginal) {
    rng.seed(26);
    pud_world& original = build(siblings());
    unfold_randomly(original, 20, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_EQ(resume_in_lockstep(original, resumed, 30, std::nullopt, goal_choice::random), 30u);
}

TEST_F(PudRuntimeReloadIntegrationTest, ResumedRefutationsCascadeLikeTheOriginal) {
    rng.seed(27);
    pud_world& original = build(refutations());
    unfold_randomly(original, 2, std::nullopt);
    pud_world& resumed = expect_faithful_reload(original);
    EXPECT_LT(resume_in_lockstep(original, resumed, 2000, std::nullopt, goal_choice::leftmost), 2000u);
}

TEST_F(PudRuntimeReloadIntegrationTest, AResumedUnfoldInADependencyRefutesTheDependentsLeavesLikeTheOriginal) {
    pud_world& original = build(cascade());
    pud_world& resumed = expect_faithful_reload(original);
    const pud_global_node_id mid{.module = 8001, .node = 2};
    const pud_frontier_change expected = original.unfold(mid, 0);
    const pud_frontier_change actual = resumed.unfold(mid, 0);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    EXPECT_TRUE(resumed.module_of(8002).check_refuted(0));
    expect_same_modules(original, resumed);
}

TEST_F(PudRuntimeReloadIntegrationTest, AResumedUnfoldInADependencyKeepsADependentsQueryResolvable) {
    pud_world& original = build({
        module_spec{.id = 1, .dependencies = {}, .axioms = nat_axioms()},
        module_spec{.id = 2, .dependencies = {1}, .axioms = {rule(atom("q"), {functor("nat", {peano(3)})}, 0)}},
    });
    pud_world& resumed = expect_faithful_reload(original);
    const pud_global_node_id step{.module = 1, .node = 1};
    original.unfold(step, 0);
    resumed.unfold(step, 0);
    const pud_global_node_id query{.module = 2, .node = 0};
    const pud_frontier_change expected = original.unfold(query, 0);
    const pud_frontier_change actual = resumed.unfold(query, 0);
    ASSERT_EQ(actual.offspring.size(), 1u);
    EXPECT_EQ(actual.offspring, expected.offspring);
    EXPECT_THAT(actual.refuted, UnorderedElementsAreArray(expected.refuted));
    expect_same_modules(original, resumed);
}

TEST_F(PudRuntimeReloadIntegrationTest, ReloadingAfterEveryUnfoldKeepsUnfoldingLikeTheOriginal) {
    rng.seed(28);
    pud_world& original = build(arithmetic());
    pud_world* resumed = &expect_faithful_reload(original);
    for (int unfold_index = 0; unfold_index < 20 && !HasFailure(); ++unfold_index) {
        ASSERT_TRUE(unfold_in_lockstep(original, *resumed, std::nullopt, goal_choice::random)) << "unfold " << unfold_index;
        resumed = &expect_faithful_reload(*resumed);
    }
    expect_same_modules(original, *resumed);
}

TEST_F(PudRuntimeReloadIntegrationTest, ReloadingEveryFewUnfoldsKeepsUnfoldingLikeTheOriginal) {
    rng.seed(29);
    pud_world& original = build(lists());
    pud_world* resumed = &expect_faithful_reload(original);
    for (int generation = 0; generation < 6 && !HasFailure(); ++generation) {
        resume_in_lockstep(original, *resumed, 6, std::nullopt, goal_choice::random);
        resumed = &expect_faithful_reload(*resumed);
    }
    EXPECT_EQ(resumed->live, original.live);
}

TEST_F(PudRuntimeReloadIntegrationTest, AResumedRuntimeCanItselfBeReloadedAndResumed) {
    rng.seed(30);
    pud_world& original = build(graph());
    unfold_randomly(original, 10, std::nullopt);
    pud_world& first = expect_faithful_reload(original);
    resume_in_lockstep(original, first, 15, std::nullopt, goal_choice::random);
    pud_world& second = expect_faithful_reload(first);
    resume_in_lockstep(original, second, 15, std::nullopt, goal_choice::random);
}

TEST_F(PudRuntimeReloadIntegrationTest, EveryProgramResumesLikeTheOriginalFromAnyPoint) {
    for (uint32_t seed = 0; seed < 5 && !HasFailure(); ++seed) {
        for (const program_t& program : every_program()) {
            rng.seed(100 + seed);
            pud_world& original = build(program);
            unfold_randomly(original, rng() % 25, std::nullopt);
            pud_world& resumed = expect_faithful_reload(original);
            resume_in_lockstep(original, resumed, 20, std::nullopt, goal_choice::random);
            if (HasFailure()) {
                ADD_FAILURE() << "seed " << seed << ", program of module " << program.front().id;
                return;
            }
        }
    }
}
