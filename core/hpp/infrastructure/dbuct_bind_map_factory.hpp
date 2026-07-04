#ifndef DBUCT_BIND_MAP_FACTORY_HPP
#define DBUCT_BIND_MAP_FACTORY_HPP

#include "infrastructure/dbuct_bind_map.hpp"

// Delayed-backtracking variant of bind_map_factory: mints the throwaway local
// bind maps MHU uses for per-head unification trials.
struct dbuct_bind_map_factory {
    explicit dbuct_bind_map_factory(globalizer& g) : globalizer_(g) {}
    dbuct_bind_map make() const { return dbuct_bind_map{globalizer_}; }
private:
    globalizer& globalizer_;
};

#endif
