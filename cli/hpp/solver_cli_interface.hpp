#ifndef SOLVER_CLI_INTERFACE_HPP
#define SOLVER_CLI_INTERFACE_HPP

#include <string>
#include <map>
#include "../../core/hpp/trail.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"
#include "../../core/hpp/domain/entities/unifier.hpp"
#include "../../core/hpp/normalizer.hpp"
#include "../../core/hpp/expr_printer.hpp"
#include "../../core/hpp/defs.hpp"

struct solver_cli_interface {
    solver_cli_interface(const std::string& file, const std::string& goals_str);
    virtual ~solver_cli_interface() = default;
    void operator()();
protected:
    virtual bool advance() = 0;
    void print_bindings();

    trail t;
    expr_pool pool;
    sequencer seq;
    unifier bm;
    normalizer norm;

    database db;
    goals gl;
private:
    static std::map<uint32_t, std::string> invert(const std::map<std::string, uint32_t>&);

    std::map<std::string, uint32_t> var_name_to_idx;
    std::map<uint32_t, std::string> var_idx_to_name;
    expr_printer printer;
};

#endif
