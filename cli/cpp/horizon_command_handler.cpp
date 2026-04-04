#include "../hpp/horizon_command_handler.hpp"
#include "../../core/hpp/expr_printer.hpp"
#include "../../core/hpp/horizon.hpp"
#include "../../parser/hpp/import_database_from_file.hpp"
#include "../../parser/hpp/import_goals_from_string.hpp"
#include <iostream>
#include <limits>
#include <random>

horizon_command_handler::horizon_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    double exploration_constant,
    uint64_t seed
) :
    pool(t), seq(t), bm(t), norm(pool, bm),
    db(import_database_from_file(file, pool, seq)),
    max_resolutions(max_resolutions),
    exploration_constant(exploration_constant),
    seed(seed)
{
    auto [g, vm] = import_goals_from_string(goals_str, pool, seq);
    gl = std::move(g);
    var_map = std::move(vm);
}

void horizon_command_handler::operator()() {
    std::mt19937 rng(seed);
    horizon solver(db, gl, t, seq, bm,
                   max_resolutions, exploration_constant, rng);
    std::optional<resolutions> res;

    while (solver(std::numeric_limits<size_t>::max(), res))
        on_solved();

    on_refuted();
}

void horizon_command_handler::on_refuted() {
    std::cout << "REFUTED\n";
}

void horizon_command_handler::on_solved() {
    expr_printer print(std::cout);
    std::cout << "SOLVED\n";
    for (const auto& [name, idx] : var_map) {
        std::cout << "  " << name << " = ";
        print(norm(pool.var(idx)));
        std::cout << "\n";
    }
    std::cout << "[press Enter for next solution]";
    std::cin.get();
}
