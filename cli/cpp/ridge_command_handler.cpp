#include "../hpp/ridge_command_handler.hpp"
#include "../../core/hpp/trail.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"
#include "../../core/hpp/bind_map.hpp"
#include "../../core/hpp/defs.hpp"
#include "../../core/hpp/normalizer.hpp"
#include "../../core/hpp/expr_printer.hpp"
#include "../../core/hpp/ridge.hpp"
#include "../../parser/hpp/import_database_from_file.hpp"
#include "../../parser/hpp/import_goals_from_string.hpp"
#include <iostream>
#include <random>

ridge_command_handler::ridge_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    size_t iterations_per_avoidance,
    double exploration_constant,
    uint64_t seed,
    size_t steps
) :
    file(file),
    goals_str(goals_str),
    max_resolutions(max_resolutions),
    iterations_per_avoidance(iterations_per_avoidance),
    exploration_constant(exploration_constant),
    seed(seed),
    steps(steps)
{}

void ridge_command_handler::operator()() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    bind_map bm(t);
    std::mt19937 rng(seed);

    database db = import_database_from_file(file, pool, seq);
    auto [gl, var_map] = import_goals_from_string(goals_str, pool, seq);

    ridge solver(db, gl, t, seq, bm,
                 max_resolutions, iterations_per_avoidance,
                 exploration_constant, rng);
    std::optional<resolutions> res;
    bool sat = solver(steps, res);

    if (sat) {
        normalizer norm(pool, bm);
        expr_printer print(std::cout);
        for (const auto& [name, idx] : var_map) {
            std::cout << name << " = ";
            print(norm(pool.var(idx)));
            std::cout << "\n";
        }
    } else {
        std::cout << "UNSAT\n";
    }
}
