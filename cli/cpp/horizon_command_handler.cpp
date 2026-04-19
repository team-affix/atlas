#include "../hpp/horizon_command_handler.hpp"

horizon_command_handler::horizon_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    double exploration_constant,
    uint64_t seed
) :
    solver_cli_interface(file, goals_str),
    rng(seed),
    solver(solver_context{db, gl, t, seq, bm, max_resolutions}, mcts_params{exploration_constant, rng})
{}

bool horizon_command_handler::advance() {
    std::optional<resolutions> soln;
    while (solver(soln)) {
        if (soln.has_value()) return true;
    }
    return false;
}
