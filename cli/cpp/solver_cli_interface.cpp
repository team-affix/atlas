#include "../hpp/solver_cli_interface.hpp"
#include "../../core/hpp/expr_printer.hpp"
#include "../../parser/hpp/import_database_from_file.hpp"
#include "../../parser/hpp/import_goals_from_string.hpp"
#include <iostream>
#include <limits>

solver_cli_interface::solver_cli_interface(
    const std::string& file,
    const std::string& goals_str
) :
    pool(t), seq(t), bm(t), norm(pool, bm),
    db(import_database_from_file(file, pool, seq))
{
    auto [g, vm] = import_goals_from_string(goals_str, pool, seq);
    gl = std::move(g);
    var_map = std::move(vm);
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
    expr_printer print(std::cout);
    for (const auto& [name, idx] : var_map) {
        std::cout << "  " << name << " = ";
        print(norm(pool.var(idx)));
        std::cout << "\n";
    }
}
