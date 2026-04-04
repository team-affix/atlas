#ifndef RIDGE_COMMAND_HANDLER_HPP
#define RIDGE_COMMAND_HANDLER_HPP

#include <cstdint>
#include <cstddef>
#include <optional>
#include <random>
#include "solver_cli_interface.hpp"
#include "../../core/hpp/ridge.hpp"

struct ridge_command_handler : solver_cli_interface {
    ridge_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        size_t iterations_per_avoidance,
        double exploration_constant,
        uint64_t seed
    );
protected:
    bool advance() override;
private:
    std::optional<resolutions> res;
    std::mt19937 rng;
    ridge solver;
};

#endif
