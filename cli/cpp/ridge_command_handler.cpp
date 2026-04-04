#include "../hpp/ridge_command_handler.hpp"

ridge_command_handler::ridge_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    size_t iterations_per_avoidance,
    double exploration_constant,
    uint64_t seed
) :
    solver_cli_interface(file, goals_str),
    rng(seed),
    solver(db, gl, t, seq, bm, max_resolutions, iterations_per_avoidance, exploration_constant, rng)
{}

bool ridge_command_handler::advance() {
    return solver(std::numeric_limits<size_t>::max(), res);
}

