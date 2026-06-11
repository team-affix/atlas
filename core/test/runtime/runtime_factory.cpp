#include "runtime/runtime_factory.hpp"
#include "runtime/runtime_test_config.hpp"
#include <stdexcept>

i_runtime& make_runtime_session(
    runtime_session_holder& holder,
    runtime_kind kind,
    db& database,
    initial_goal_exprs& goals,
    size_t initial_var_count,
    size_t max_resolutions,
    uint32_t seed) {
    switch (kind) {
        case runtime_kind::basic:
            holder.basic.emplace(
                database, goals, initial_var_count, max_resolutions, seed);
            return *holder.basic;
        case runtime_kind::ridge:
            holder.ridge.emplace(
                database,
                goals,
                initial_var_count,
                max_resolutions,
                seed,
                kRidgeExplorationConstant);
            return *holder.ridge;
    }
    throw std::logic_error("unknown runtime_kind");
}
