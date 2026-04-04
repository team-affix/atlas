#ifndef HORIZON_COMMAND_HANDLER_HPP
#define HORIZON_COMMAND_HANDLER_HPP

#include <string>
#include <cstdint>
#include <cstddef>
#include <map>
#include "../../core/hpp/trail.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"
#include "../../core/hpp/bind_map.hpp"
#include "../../core/hpp/defs.hpp"
#include "../../core/hpp/normalizer.hpp"

struct horizon_command_handler {
    horizon_command_handler(
        const std::string& file,
        const std::string& goals_str,
        size_t max_resolutions,
        double exploration_constant,
        uint64_t seed
    );
    void operator()();
private:
    void on_solved();
    void on_refuted();

    // trail must be declared first — pool, seq, bm are initialised from it
    trail t;
    expr_pool pool;
    sequencer seq;
    bind_map bm;
    normalizer norm;

    database db;
    goals gl;
    std::map<std::string, uint32_t> var_map;

    size_t max_resolutions;
    double exploration_constant;
    uint64_t seed;
};

#endif
