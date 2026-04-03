#include "../hpp/horizon_command_handler.hpp"
#include "../../core/hpp/trail.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"
#include "../../core/hpp/bind_map.hpp"
#include "../../core/hpp/defs.hpp"
#include "../../core/hpp/horizon.hpp"
#include "../../parser/hpp/import_database_from_file.hpp"
#include "../../parser/hpp/import_goals_from_string.hpp"
#include <iostream>
#include <random>

horizon_command_handler::horizon_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    double exploration_constant,
    uint64_t seed,
    size_t steps
) :
    file(file),
    goals_str(goals_str),
    max_resolutions(max_resolutions),
    exploration_constant(exploration_constant),
    seed(seed),
    steps(steps)
{}

void horizon_command_handler::operator()() {
    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    bind_map bm(t);
    std::mt19937 rng(seed);

    database db = import_database_from_file(file, pool, seq);
    goals gl = import_goals_from_string(goals_str, pool, seq);

    horizon solver(db, gl, t, seq, bm,
                   max_resolutions, exploration_constant, rng);
    std::optional<resolutions> res;
    bool sat = solver(steps, res);
    std::cout << (sat ? "SAT" : "UNSAT") << "\n";
}
