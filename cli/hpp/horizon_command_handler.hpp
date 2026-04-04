#ifndef HORIZON_COMMAND_HANDLER_HPP
#define HORIZON_COMMAND_HANDLER_HPP

#include <cstdint>
#include <cstddef>
#include <optional>
#include <random>
#include "solver_cli_interface.hpp"
#include "../../core/hpp/horizon.hpp"

struct horizon_command_handler : solver_cli_interface {
    horizon_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        double exploration_constant,
        uint64_t seed
    );
protected:
    bool advance() override;
private:
    std::optional<resolutions> res;
    std::mt19937 rng;
    horizon solver;
};

#endif
