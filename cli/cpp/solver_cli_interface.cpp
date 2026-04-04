#include "../hpp/solver_cli_interface.hpp"
#include "../../parser/hpp/import_database_from_file.hpp"
#include "../../parser/hpp/import_goals_from_string.hpp"
#include <iostream>

solver_cli_interface::solver_cli_interface(
    const std::string& file,
    const std::string& goals_str
) :
    pool(t), seq(t), bm(t), norm(pool, bm),
    db(import_database_from_file(file, pool, seq)),
    printer(std::cout, var_idx_to_name)
{
    auto [g, name_to_idx] = import_goals_from_string(goals_str, pool, seq);
    gl = std::move(g);
    var_name_to_idx = std::move(name_to_idx);
    var_idx_to_name = invert(var_name_to_idx);
}

void solver_cli_interface::operator()() {
    while (advance()) {
        std::cout << "SOLVED\n";
        print_bindings();
        std::cout << "[press Enter for next solution]";
        std::cin.get();
    }
    std::cout << "REFUTED\n";
}

void solver_cli_interface::print_bindings() {
    for (const auto& [idx, name] : var_idx_to_name) {
        std::cout << "  " << name << " = ";
        printer(norm(pool.var(idx)));
        std::cout << "\n";
    }
}

std::map<uint32_t, std::string> solver_cli_interface::invert(const std::map<std::string, uint32_t>& m) {
    std::map<uint32_t, std::string> inv;
    for (const auto& [name, idx] : m)
        inv[idx] = name;
    return inv;
}
