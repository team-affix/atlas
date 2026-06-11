#ifndef RUNTIME_FACTORY_HPP
#define RUNTIME_FACTORY_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include "infrastructure/basic_runtime.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_runtime.hpp"
#include "interfaces/i_runtime.hpp"

enum class runtime_kind { basic, ridge };

struct runtime_session_holder {
    std::optional<basic_runtime> basic;
    std::optional<ridge_runtime> ridge;
};

i_runtime& make_runtime_session(
    runtime_session_holder& holder,
    runtime_kind kind,
    db& database,
    initial_goal_exprs& goals,
    size_t initial_var_count,
    size_t max_resolutions,
    uint32_t seed);

#endif
