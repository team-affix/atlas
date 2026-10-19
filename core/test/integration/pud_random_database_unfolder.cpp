#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_random_database_unfolder.hpp"
#include "infrastructure/pud_random_unfold_chooser.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "pud_datalog_oracle.hpp"
#include "pud_expr_structure.hpp"
#include "value_objects/rule.hpp"

namespace {
using test_chooser_t = pud_random_unfold_chooser<pud_runtime>;
using test_unfolder_t = pud_random_database_unfolder<pud_runtime, pud_runtime, test_chooser_t>;

struct database_under_test {
    database_under_test(uint32_t random_seed)
        : database()
        , runtime(database)
        , rng(random_seed)
        , chooser(runtime, rng)
        , unfolder(runtime, runtime, chooser)
        , unfolding(unfolder.unfold_database())
        , proven()
        , opened_count(0)
        , ended(false) {}

    db database;
    pud_runtime runtime;
    std::mt19937 rng;
    test_chooser_t chooser;
    test_unfolder_t unfolder;
    coroutine<pud_frontier_change, void> unfolding;
    std::set<pud_derivation_id> proven;
    size_t opened_count;
    bool ended;

    std::optional<pud_frontier_change> next() {
        unfolding.resume();
        if (!unfolding.has_yield()) {
            ended = true;
            return std::nullopt;
        }
        const pud_frontier_change change = unfolding.consume_yield();
        opened_count += change.opened.size();
        for (pud_derivation_id derivation : change.proven)
            proven.insert(derivation);
        for (pud_derivation_id derivation : change.refuted)
            proven.erase(derivation);
        return change;
    }

    size_t drive(size_t max_changes) {
        size_t changes = 0;
        while (changes < max_changes && next().has_value())
            ++changes;
        return changes;
    }
};

struct PudRandomDatabaseUnfolderIntegrationTest : public ::testing::Test {
    expr_pool pool;
    test_functors functors;
    database_under_test subject{0};

    const expr* var(uint32_t index) { return pool.make_var(index); }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return pool.make_functor(functors.id(name), args);
    }

    const expr* atom(const std::string& name) { return functor(name, {}); }

    void axiom(database_under_test& target, const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        target.database.push(rule(head, std::move(body), var_count));
    }

    void add_colors(database_under_test& target) {
        axiom(target, functor("color", {atom("red")}), {}, 0);
        axiom(target, functor("color", {atom("blue")}), {}, 0);
        axiom(target, functor("warm", {atom("red")}), {}, 0);
        axiom(target, functor("warmcolor", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    }

    void add_nat(database_under_test& target) {
        axiom(target, functor("nat", {atom("z")}), {}, 0);
        axiom(target, functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    }

    void load(database_under_test& target, const datalog_program& program) {
        for (const datalog_atom& fact : program.facts)
            axiom(target, datalog_expr(*this, fact), {}, 0);
        for (const datalog_rule& datalog_rule : program.rules) {
            std::vector<const expr*> body;
            for (const datalog_atom& body_atom : datalog_rule.body)
                body.push_back(datalog_expr(*this, body_atom));
            axiom(target, datalog_expr(*this, datalog_rule.head), body, static_cast<uint32_t>(datalog_rule.var_count));
        }
    }

    bool proves(database_under_test& target, const expr* fact) {
        for (pud_derivation_id derivation : target.proven) {
            if (same_structure(target.runtime.head_expr(derivation), fact))
                return true;
        }
        return false;
    }

    std::set<datalog_fact> proven_facts(database_under_test& target) {
        std::set<datalog_fact> facts;
        for (pud_derivation_id derivation : target.proven) {
            const expr* head = target.runtime.head_expr(derivation);
            const expr::functor& head_functor = std::get<expr::functor>(head->content);
            std::vector<int> args;
            for (const expr* arg : head_functor.args) {
                for (int constant = 0; constant < k_datalog_constants; ++constant) {
                    if (same_structure(arg, atom("k" + std::to_string(constant))))
                        args.push_back(constant);
                }
            }
            EXPECT_EQ(args.size(), head_functor.args.size()) << "a proven head is not ground";
            for (int predicate = 0; predicate < 5; ++predicate) {
                if (head_functor.id == functors.id("p" + std::to_string(predicate)))
                    facts.insert({predicate, args});
            }
        }
        return facts;
    }
};

}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, OpeningTheDatabaseOpensEveryRuleAndProvesEveryFact) {
    add_colors(subject);
    add_nat(subject);
    const pud_frontier_change opened = subject.next().value();
    EXPECT_EQ(opened.opened.size(), 2u);
    EXPECT_EQ(opened.proven.size(), 4u);
    EXPECT_TRUE(opened.refuted.empty());
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, RuleWhoseGoalHasNoCandidatesIsRefutedAtOpening) {
    axiom(subject, functor("p", {var(0)}), {functor("missing", {var(0)})}, 1);
    const pud_frontier_change opened = subject.next().value();
    EXPECT_EQ(opened.refuted.size(), 1u);
    EXPECT_FALSE(subject.next().has_value());
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, EmptyDatabaseOnlyOpens) {
    EXPECT_EQ(subject.next(), std::optional<pud_frontier_change>{pud_frontier_change{}});
    EXPECT_FALSE(subject.next().has_value());
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, FiniteDatabaseUnfoldsUntilNothingIsOpen) {
    add_colors(subject);
    EXPECT_LT(subject.drive(100), 100u);
    EXPECT_TRUE(subject.ended);
    EXPECT_TRUE(proves(subject, functor("warmcolor", {atom("red")})));
    EXPECT_FALSE(proves(subject, functor("warmcolor", {atom("blue")})));
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, EveryChosenUnfoldMeetsTheRuntimesPreconditions) {
    add_colors(subject);
    add_nat(subject);
    EXPECT_NO_THROW(subject.drive(60));
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, UnfoldingReachesEveryAxiomTree) {
    add_colors(subject);
    add_nat(subject);
    subject.drive(200);
    EXPECT_TRUE(proves(subject, functor("warmcolor", {atom("red")}))) << "warmcolor tree never unfolded";
    EXPECT_TRUE(proves(subject, functor("nat", {functor("s", {atom("z")})}))) << "nat tree never unfolded";
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, InfiniteRecursionNeverEnds) {
    add_nat(subject);
    EXPECT_EQ(subject.drive(80), 80u);
    EXPECT_FALSE(subject.ended);
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, SameSeedUnfoldsTheSameWay) {
    database_under_test again{0};
    add_colors(subject);
    add_nat(subject);
    add_colors(again);
    add_nat(again);
    for (int step = 0; step < 40; ++step)
        ASSERT_EQ(subject.next(), again.next()) << "step " << step;
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, FlatProgramsProveExactlyTheLeastModel) {
    for (unsigned seed = 0; seed < 20; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        database_under_test seeded{seed};
        const datalog_program program = generate_datalog(seed, false);
        load(seeded, program);
        seeded.drive(20000);
        ASSERT_TRUE(seeded.ended) << "a flat program must unfold until nothing is open";
        EXPECT_EQ(proven_facts(seeded), datalog_least_model(program));
    }
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, EveryUnfoldOrderProvesTheSameModel) {
    const datalog_program program = generate_datalog(3, false);
    for (uint32_t random_seed = 0; random_seed < 8; ++random_seed) {
        SCOPED_TRACE("random seed " + std::to_string(random_seed));
        database_under_test seeded{random_seed};
        load(seeded, program);
        seeded.drive(20000);
        EXPECT_EQ(proven_facts(seeded), datalog_least_model(program));
    }
}

TEST_F(PudRandomDatabaseUnfolderIntegrationTest, RecursiveProgramsOnlyProveEntailedFacts) {
    for (unsigned seed = 0; seed < 20; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        database_under_test seeded{seed};
        const datalog_program program = generate_datalog(seed, true);
        load(seeded, program);
        size_t changes = 0;
        while (changes < 300 && seeded.opened_count < 2000 && seeded.next().has_value())
            ++changes;
        const std::set<datalog_fact> model = datalog_least_model(program);
        for (const datalog_fact& fact : proven_facts(seeded))
            EXPECT_TRUE(model.contains(fact)) << "predicate " << fact.first;
    }
}
