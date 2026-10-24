#ifndef PUD_RELOAD_HARNESS_HPP
#define PUD_RELOAD_HARNESS_HPP

#include <algorithm>
#include <cstdint>
#include <deque>
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
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/rule.hpp"

// Tests reloading through nothing but a runtime's interface: constructed on the caller's expression
// pool, it registers whole modules and unfolds leaves. Any runtime type with that interface can be
// tested here. Everything a test checks besides what the runtime reports is read from the modules,
// which belong to the caller.

// One module of a program, before anything is derived under it.
struct module_spec {
    pud_module_id id;
    std::vector<pud_module_id> dependencies;
    std::vector<rule> axioms;
};

using program_t = std::vector<module_spec>;

struct unfold_step {
    pud_global_node_id leaf;
    pud_body_goal_id goal;
};

// Which goal of the chosen leaf is unfolded. Unfolding the leftmost goal first, as Prolog does,
// keeps the programs below finite when they have finitely many answers.
enum class goal_choice { random, leftmost };

// One runtime over modules of its own, registered in order, and the frontier its changes describe.
// Restarting throws the runtime away and registers the same modules, in the same order, with a new
// one.
template<typename IRuntime>
struct pud_world {
    expr_pool& exprs;
    std::deque<pud_module> modules;
    std::optional<IRuntime> runtime;
    std::set<pud_global_node_id> live;
    std::vector<pud_global_node_id> refuted_on_registration;

    pud_world(expr_pool& pool)
        : exprs(pool)
        , modules()
        , runtime()
        , live()
        , refuted_on_registration() {
        runtime.emplace(exprs);
    }

    void register_module(pud_module module) {
        register_stored(modules.emplace_back(std::move(module)));
    }

    void restart() {
        runtime.reset();
        runtime.emplace(exprs);
        live.clear();
        refuted_on_registration.clear();
        for (pud_module& module : modules)
            register_stored(module);
    }

    pud_frontier_change unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
        const pud_frontier_change change = runtime->unfold(leaf, goal);
        live.erase(leaf);
        track(change);
        return change;
    }

    const pud_module& module_of(pud_module_id id) const {
        for (const pud_module& module : modules) {
            if (module.get_module_id() == id)
                return module;
        }
        throw std::out_of_range("module not in this world");
    }

    void register_stored(pud_module& module) {
        const pud_frontier_change change = runtime->register_module(module);
        refuted_on_registration.insert(refuted_on_registration.end(), change.refuted.begin(), change.refuted.end());
        track(change);
    }

    void track(const pud_frontier_change& change) {
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }
};

// Worlds, programs, a seeded driver that picks a live leaf and one of its goals from the modules
// alone, and the checks every reload must pass. A faithful reload puts back the frontier the original
// reported, derives no refutation, and leaves the modules exactly as they were. Every frontier is also
// checked against the one the modules imply on their own: the leaves reached from every live axiom
// through live children.
template<typename IRuntime>
struct pud_reload_harness {
    using world_t = pud_world<IRuntime>;

    test_functors functors;
    expr_pool exprs;
    std::deque<world_t> worlds;
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

    const expr* nested_unary(const char* name, const expr* base, uint32_t depth) {
        const expr* term = base;
        for (uint32_t level = 0; level < depth; ++level)
            term = functor(name, {term});
        return term;
    }

    const expr* nested_binary(const char* name, const expr* base, uint32_t depth) {
        const expr* term = base;
        for (uint32_t level = 0; level < depth; ++level)
            term = functor(name, {term, term});
        return term;
    }

    world_t& build(const program_t& program) {
        world_t& world = worlds.emplace_back(exprs);
        for (const module_spec& spec : program)
            world.register_module(pud_module(spec.id, spec.dependencies, spec.axioms));
        return world;
    }

    world_t& reload(const world_t& source) {
        world_t& reloaded = worlds.emplace_back(exprs);
        for (const pud_module& module : source.modules)
            reloaded.register_module(module);
        return reloaded;
    }

    pud_goal_list goals_of(const world_t& world, pud_global_node_id node) {
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

    std::vector<pud_body_goal_id> goal_ids_of(const world_t& world, pud_global_node_id node) {
        std::vector<pud_body_goal_id> ids;
        for (const auto& [id, goal] : goals_of(world, node).goals)
            ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    std::optional<unfold_step> choose_step(
        const world_t& world, std::optional<pud_module_id> only_module, goal_choice choice) {
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

    size_t unfold_steps(world_t& world, size_t steps, std::optional<pud_module_id> only_module, goal_choice choice) {
        for (size_t taken = 0; taken < steps; ++taken) {
            const std::optional<unfold_step> step = choose_step(world, only_module, choice);
            if (!step.has_value())
                return taken;
            world.unfold(step->leaf, step->goal);
        }
        return steps;
    }

    size_t unfold_randomly(world_t& world, size_t steps, std::optional<pud_module_id> only_module) {
        return unfold_steps(world, steps, only_module, goal_choice::random);
    }

    size_t unfold_to_exhaustion(world_t& world, std::optional<pud_module_id> only_module) {
        const size_t cap = 2000;
        const size_t taken = unfold_steps(world, cap, only_module, goal_choice::leftmost);
        EXPECT_LT(taken, cap) << "the program did not run out of unfolds";
        return taken;
    }

    size_t count_module_nodes(const pud_module& module) {
        size_t nodes = 0;
        std::vector<pud_node_id> pending;
        for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom)
            pending.push_back(axiom);
        while (!pending.empty()) {
            const pud_node_id node = pending.back();
            pending.pop_back();
            ++nodes;
            pending.insert(pending.end(), module.get_children(node).begin(), module.get_children(node).end());
        }
        return nodes;
    }

    size_t count_nodes(const world_t& world) {
        size_t nodes = 0;
        for (const pud_module& module : world.modules)
            nodes += count_module_nodes(module);
        return nodes;
    }

    // Collects the leaves a world's modules imply, and fails on a live interior node with no live child.
    std::set<pud_global_node_id> frontier_of(const world_t& world) {
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

    void expect_same_modules(const world_t& expected, const world_t& actual) {
        ASSERT_EQ(actual.modules.size(), expected.modules.size());
        for (size_t index = 0; index < expected.modules.size(); ++index)
            expect_same_module(expected.modules.at(index), actual.modules.at(index));
    }

    void expect_consistent(const world_t& world) {
        EXPECT_EQ(world.live, frontier_of(world));
    }

    world_t& expect_faithful_reload(const world_t& source) {
        world_t& reloaded = reload(source);
        EXPECT_EQ(reloaded.live, source.live);
        EXPECT_EQ(reloaded.live, frontier_of(reloaded));
        EXPECT_THAT(reloaded.refuted_on_registration, ::testing::IsEmpty());
        expect_same_modules(source, reloaded);
        return reloaded;
    }

    // Restarts a world on its own modules and checks it against a twin that went through the same
    // unfolds without ever restarting.
    void expect_faithful_restart(world_t& restarted, const world_t& twin) {
        restarted.restart();
        EXPECT_EQ(restarted.live, twin.live);
        EXPECT_EQ(restarted.live, frontier_of(restarted));
        EXPECT_THAT(restarted.refuted_on_registration, ::testing::IsEmpty());
        expect_same_modules(twin, restarted);
    }

    // Unfolds the same leaf on the same goal in both worlds, chosen from the reference's frontier, and
    // fails on any difference. Returns whether there was a leaf left to unfold.
    bool unfold_in_lockstep(
        world_t& reference, world_t& follower, std::optional<pud_module_id> only_module, goal_choice choice) {
        EXPECT_EQ(follower.live, reference.live);
        const std::optional<unfold_step> step = choose_step(reference, only_module, choice);
        if (!step.has_value())
            return false;
        const pud_frontier_change expected = reference.unfold(step->leaf, step->goal);
        const pud_frontier_change actual = follower.unfold(step->leaf, step->goal);
        EXPECT_EQ(actual.offspring, expected.offspring)
            << "unfolding (" << step->leaf.module << ", " << step->leaf.node << ") on " << step->goal;
        EXPECT_THAT(actual.refuted, ::testing::UnorderedElementsAreArray(expected.refuted));
        return true;
    }

    size_t resume_in_lockstep(
        world_t& reference,
        world_t& follower,
        size_t steps,
        std::optional<pud_module_id> only_module,
        goal_choice choice) {
        size_t taken = 0;
        while (taken < steps && !::testing::Test::HasFailure() && unfold_in_lockstep(reference, follower, only_module, choice))
            ++taken;
        expect_same_modules(reference, follower);
        expect_consistent(reference);
        expect_consistent(follower);
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

    std::vector<rule> add_axioms() {
        return {
            rule(functor("add", {atom("z"), var(0), var(0)}), {}, 1),
            rule(
                functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
                {functor("add", {var(0), var(1), var(2)})},
                3),
        };
    }

    std::vector<rule> mul_axioms() {
        return {
            rule(functor("mul", {atom("z"), var(0), atom("z")}), {}, 1),
            rule(
                functor("mul", {functor("s", {var(0)}), var(1), var(2)}),
                {functor("mul", {var(0), var(1), var(3)}), functor("add", {var(3), var(1), var(2)})},
                4),
        };
    }

    std::vector<rule> member_axioms() {
        return {
            rule(functor("mem", {var(0), functor("cons", {var(0), var(1)})}), {}, 2),
            rule(
                functor("mem", {var(0), functor("cons", {var(1), var(2)})}),
                {functor("mem", {var(0), var(2)})},
                3),
        };
    }

    program_t arithmetic() {
        std::vector<rule> base = nat_axioms();
        for (const rule& axiom : add_axioms())
            base.push_back(axiom);
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

    program_t peano_mul() {
        std::vector<rule> base = nat_axioms();
        for (const rule& axiom : add_axioms())
            base.push_back(axiom);
        for (const rule& axiom : mul_axioms())
            base.push_back(axiom);
        return {
            module_spec{.id = 9001, .dependencies = {}, .axioms = base},
            module_spec{.id = 9002, .dependencies = {9001}, .axioms = {
                rule(functor("factor", {var(0), var(1)}), {functor("mul", {var(0), var(1), peano(6)})}, 2),
                rule(functor("triple", {var(0)}), {functor("mul", {peano(3), var(0), peano(9)})}, 1),
                rule(
                    functor("add_mul", {var(0), var(1), var(2)}),
                    {functor("add", {var(0), var(1), var(2)}), functor("mul", {var(0), peano(2), var(3)})},
                    4),
            }},
        };
    }

    program_t list_members() {
        return {
            module_spec{.id = 9101, .dependencies = {}, .axioms = member_axioms()},
            module_spec{.id = 9102, .dependencies = {9101}, .axioms = {
                rule(
                    functor("every", {var(0)}),
                    {functor("mem", {var(0), list({atom("x"), atom("y"), atom("z")})})},
                    1),
                rule(
                    functor("pair_mem", {var(0), var(1)}),
                    {
                        functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})}),
                        functor("mem", {var(1), list({atom("b"), atom("c"), atom("d")})}),
                    },
                    2),
            }},
        };
    }

    program_t mirror_trees() {
        const expr* sample =
            functor("branch", {functor("leaf", {atom("l")}), functor("branch", {functor("leaf", {atom("m")}), functor("leaf", {atom("r")})})});
        return {
            module_spec{.id = 9201, .dependencies = {}, .axioms = {
                rule(functor("mirror", {functor("leaf", {var(0)}), functor("leaf", {var(0)})}), {}, 1),
                rule(
                    functor("mirror", {functor("branch", {var(0), var(1)}), functor("branch", {var(2), var(3)})}),
                    {functor("mirror", {var(0), var(3)}), functor("mirror", {var(1), var(2)})},
                    4),
            }},
            module_spec{.id = 9202, .dependencies = {9201}, .axioms = {
                rule(functor("flip", {var(0)}), {functor("mirror", {sample, var(0)})}, 1),
            }},
        };
    }

    program_t nested_unary_chain() {
        std::vector<rule> base = nat_axioms();
        base.push_back(rule(
            functor("deep_nat", {nested_unary("s", atom("z"), 8)}),
            {functor("nat", {nested_unary("s", atom("z"), 8)})},
            0));
        base.push_back(rule(
            functor("shallow", {var(0)}),
            {functor("nat", {var(0)}), functor("add", {var(0), peano(2), nested_unary("s", nested_unary("s", atom("z"), 3), 2)})},
            1));
        for (const rule& axiom : add_axioms())
            base.push_back(axiom);
        return {
            module_spec{.id = 9301, .dependencies = {}, .axioms = base},
            module_spec{.id = 9302, .dependencies = {9301}, .axioms = {
                rule(
                    functor("reach", {var(0)}),
                    {functor("nat", {nested_unary("s", atom("z"), 5)}), functor("add", {var(0), peano(1), nested_unary("s", atom("z"), 5)})},
                    1),
            }},
        };
    }

    program_t triple_append() {
        return {
            module_spec{.id = 9401, .dependencies = {}, .axioms = {
                rule(functor("app", {atom("nil"), var(0), var(0)}), {}, 1),
                rule(
                    functor("app", {functor("cons", {var(0), var(1)}), var(2), functor("cons", {var(0), var(3)})}),
                    {functor("app", {var(1), var(2), var(3)})},
                    4),
            }},
            module_spec{.id = 9402, .dependencies = {9401}, .axioms = {
                rule(
                    atom("triple_split"),
                    {
                        functor("app", {var(0), var(1), list({atom("a"), atom("b"), atom("c")})}),
                        functor("app", {var(1), var(2), list({atom("d"), atom("e")})}),
                        functor("app", {var(0), var(2), var(3)}),
                    },
                    4),
                rule(
                    functor("nested_app", {var(0)}),
                    {
                        functor("app", {var(0), var(1), list({atom("p"), atom("q")})}),
                        functor("app", {var(1), list({atom("r")}), list({atom("p"), atom("q"), atom("r")})}),
                    },
                    2),
            }},
        };
    }

    program_t shared_binary() {
        const expr* shared = nested_binary("pair", atom("v"), 4);
        std::vector<rule> base = nat_axioms();
        base.push_back(rule(functor("unpack", {shared, var(0)}), {functor("same", {var(0), shared})}, 1));
        base.push_back(rule(functor("same", {var(0), var(0)}), {}, 1));
        base.push_back(rule(
            functor("both", {var(0), var(1)}),
            {functor("same", {var(0), var(1)}), functor("nat", {var(0)})},
            2));
        return {
            module_spec{.id = 9501, .dependencies = {}, .axioms = base},
            module_spec{.id = 9502, .dependencies = {9501}, .axioms = {
                rule(functor("grounded", {var(0)}), {functor("unpack", {shared, var(0)})}, 1),
                rule(functor("nat_query", {var(0)}), {functor("both", {peano(2), var(0)})}, 1),
            }},
        };
    }

    std::vector<program_t> every_program() {
        return {
            colors(),
            arithmetic(),
            lists(),
            graph(),
            tiers(),
            siblings(),
            refutations(),
            cascade(),
            peano_mul(),
            list_members(),
            mirror_trees(),
            nested_unary_chain(),
            triple_append(),
            shared_binary(),
        };
    }

    std::vector<program_t> stress_programs() {
        return {
            peano_mul(),
            list_members(),
            mirror_trees(),
            nested_unary_chain(),
            triple_append(),
            shared_binary(),
        };
    }
};

#endif
