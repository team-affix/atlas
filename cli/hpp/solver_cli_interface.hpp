#ifndef SOLVER_CLI_INTERFACE_HPP
#define SOLVER_CLI_INTERFACE_HPP

#include <string>
#include <map>
#include "../../core/hpp/trail.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"
#include "../../core/hpp/bind_map.hpp"
#include "../../core/hpp/normalizer.hpp"
#include "../../core/hpp/defs.hpp"

struct solver_cli_interface {
    solver_cli_interface(const std::string& file, const std::string& goals_str);
    virtual ~solver_cli_interface() = default;
    void operator()();
protected:
    virtual bool advance() = 0;
    void print_bindings();

    // trail must be declared first — pool, seq, bm, norm are initialised from it
    trail t;
    expr_pool pool;
    sequencer seq;
    bind_map bm;
    normalizer norm;

    database db;
    goals gl;
    std::map<std::string, uint32_t> var_map;
};

#endif
