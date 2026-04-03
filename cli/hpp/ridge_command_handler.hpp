#ifndef RIDGE_COMMAND_HANDLER_HPP
#define RIDGE_COMMAND_HANDLER_HPP

#include <string>
#include <cstdint>
#include <cstddef>

struct ridge_command_handler {
    ridge_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        size_t iterations_per_avoidance,
        double exploration_constant,
        uint64_t seed,
        size_t steps
    );
    void operator()();
private:
    std::string file;
    std::string goals_str;
    size_t max_resolutions;
    size_t iterations_per_avoidance;
    double exploration_constant;
    uint64_t seed;
    size_t steps;
};

#endif
