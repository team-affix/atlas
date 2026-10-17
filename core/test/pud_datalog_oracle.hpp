#ifndef PUD_DATALOG_ORACLE_HPP
#define PUD_DATALOG_ORACLE_HPP

#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "pud_manifest_harness.hpp"

// Small random function-free programs over a three-constant domain, and their least model.
// An argument >= 0 is a constant; an argument < 0 is the variable -(argument + 1).
struct datalog_atom {
    int predicate;
    std::vector<int> args;
};

struct datalog_rule {
    datalog_atom head;
    std::vector<datalog_atom> body;
    int var_count;
};

struct datalog_program {
    std::vector<int> arities;
    std::vector<datalog_atom> facts;
    std::vector<datalog_rule> rules;
    datalog_atom conjecture;
    int conjecture_var_count;
};

constexpr int k_datalog_constants = 3;

inline int datalog_var(int index) { return -(index + 1); }

inline int datalog_pick(std::mt19937& rng, int bound) {
    return static_cast<int>(rng() % static_cast<unsigned>(bound));
}

inline datalog_program generate_datalog(unsigned seed, bool recursive) {
    std::mt19937 rng{seed};
    datalog_program program;
    program.arities = {2, 2, 1, 2, 1};
    for (int predicate = 0; predicate < 3; ++predicate) {
        const int fact_count = 2 + datalog_pick(rng, 3);
        for (int fact = 0; fact < fact_count; ++fact) {
            datalog_atom atom{predicate, {}};
            for (int slot = 0; slot < program.arities.at(predicate); ++slot)
                atom.args.push_back(datalog_pick(rng, k_datalog_constants));
            program.facts.push_back(atom);
        }
    }
    for (int predicate = 3; predicate < 5; ++predicate) {
        const int rule_count = 1 + datalog_pick(rng, 2);
        const int arity = program.arities.at(predicate);
        for (int rule_index = 0; rule_index < rule_count; ++rule_index) {
            datalog_rule rule{{predicate, {}}, {}, arity + 2};
            for (int slot = 0; slot < arity; ++slot)
                rule.head.args.push_back(datalog_var(slot));
            const int body_count = 1 + datalog_pick(rng, 3);
            for (int body_index = 0; body_index < body_count; ++body_index) {
                const int allowed = recursive ? 5 : predicate;
                const int body_predicate = datalog_pick(rng, allowed);
                datalog_atom atom{body_predicate, {}};
                for (int slot = 0; slot < program.arities.at(body_predicate); ++slot)
                    atom.args.push_back(datalog_pick(rng, 4) == 0 ? datalog_pick(rng, k_datalog_constants) : datalog_var(datalog_pick(rng, arity + 2)));
                rule.body.push_back(atom);
            }
            for (int slot = 0; slot < arity; ++slot) {
                bool bound = false;
                for (const datalog_atom& atom : rule.body) {
                    for (int arg : atom.args)
                        bound = bound || arg == datalog_var(slot);
                }
                if (!bound)
                    rule.body.push_back(datalog_atom{2, {datalog_var(slot)}});
            }
            program.rules.push_back(rule);
        }
    }
    const int conjecture_predicate = datalog_pick(rng, 5);
    program.conjecture = datalog_atom{conjecture_predicate, {}};
    program.conjecture_var_count = 0;
    for (int slot = 0; slot < program.arities.at(conjecture_predicate); ++slot) {
        if (datalog_pick(rng, 3) == 0) {
            program.conjecture.args.push_back(datalog_pick(rng, k_datalog_constants));
            continue;
        }
        program.conjecture.args.push_back(datalog_var(program.conjecture_var_count++));
    }
    return program;
}

using datalog_fact = std::pair<int, std::vector<int>>;

inline std::vector<int> datalog_instantiate(const std::vector<int>& args, const std::vector<int>& assignment) {
    std::vector<int> ground;
    for (int arg : args)
        ground.push_back(arg >= 0 ? arg : assignment.at(-(arg + 1)));
    return ground;
}

inline bool datalog_next_assignment(std::vector<int>& assignment) {
    for (int& value : assignment) {
        if (++value < k_datalog_constants)
            return true;
        value = 0;
    }
    return false;
}

inline std::set<datalog_fact> datalog_least_model(const datalog_program& program) {
    std::set<datalog_fact> model;
    for (const datalog_atom& fact : program.facts)
        model.insert({fact.predicate, fact.args});
    bool changed = true;
    while (changed) {
        changed = false;
        for (const datalog_rule& rule : program.rules) {
            std::vector<int> assignment(rule.var_count, 0);
            do {
                bool holds = true;
                for (const datalog_atom& atom : rule.body)
                    holds = holds && model.contains({atom.predicate, datalog_instantiate(atom.args, assignment)});
                if (!holds)
                    continue;
                changed = model.insert({rule.head.predicate, datalog_instantiate(rule.head.args, assignment)}).second || changed;
            } while (datalog_next_assignment(assignment));
        }
    }
    return model;
}

inline std::set<std::vector<int>> datalog_answers(const datalog_program& program) {
    const std::set<datalog_fact> model = datalog_least_model(program);
    std::set<std::vector<int>> answers;
    std::vector<int> assignment(program.conjecture_var_count, 0);
    do {
        if (model.contains({program.conjecture.predicate, datalog_instantiate(program.conjecture.args, assignment)}))
            answers.insert(assignment);
    } while (datalog_next_assignment(assignment));
    return answers;
}

inline const expr* datalog_expr(pud_manifest_harness& harness, const datalog_atom& atom) {
    std::vector<const expr*> args;
    for (int arg : atom.args)
        args.push_back(arg >= 0 ? harness.atom("k" + std::to_string(arg)) : harness.var(-(arg + 1)));
    return harness.functor("p" + std::to_string(atom.predicate), args);
}

// Loads the program as axioms and starts its conjecture; returns the conjecture root.
inline pud_node_id load_datalog(pud_manifest_harness& harness, const datalog_program& program) {
    for (const datalog_atom& fact : program.facts)
        harness.axiom(datalog_expr(harness, fact), {}, 0);
    for (const datalog_rule& rule : program.rules) {
        std::vector<const expr*> body;
        for (const datalog_atom& atom : rule.body)
            body.push_back(datalog_expr(harness, atom));
        harness.axiom(datalog_expr(harness, rule.head), body, static_cast<uint32_t>(rule.var_count));
    }
    std::vector<const expr*> answer_args;
    for (int index = 0; index < program.conjecture_var_count; ++index)
        answer_args.push_back(harness.var(index));
    return harness.start_conjecture(
        harness.functor("ans", answer_args),
        {datalog_expr(harness, program.conjecture)},
        static_cast<uint32_t>(program.conjecture_var_count));
}

// Reads an answer back into constant indices; nullopt if it is not ground over the domain.
inline std::optional<std::vector<int>> datalog_read_answer(pud_manifest_harness& harness, const expr* answer) {
    std::vector<int> values;
    for (const expr* arg : std::get<expr::functor>(answer->content).args) {
        std::optional<int> value;
        for (int constant = 0; constant < k_datalog_constants; ++constant) {
            if (arg == harness.atom("k" + std::to_string(constant)))
                value = constant;
        }
        if (!value.has_value())
            return std::nullopt;
        values.push_back(value.value());
    }
    return values;
}

#endif
