#ifndef RUNTIME_TEST_FIXTURE_HPP
#define RUNTIME_TEST_FIXTURE_HPP

#include <iostream>
#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/var_names.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_var_names.hpp"
#include "runtime/runtime_factory.hpp"
#include "runtime/runtime_test_config.hpp"

struct RuntimeTestBase {
    db database;
    initial_goal_exprs initial_goals;

    trail saved_trail_;
    locator saved_loc_;
    var_names saved_var_names_;
    expr_printer saved_printer_{std::cout,
        (saved_loc_.bind_as<i_var_names>(saved_var_names_), saved_loc_)};
    expr_pool saved_expr_pool_{
        (saved_loc_.bind_as<i_log_to_current_trail_frame>(saved_trail_), saved_loc_)};
};

struct RuntimeParamTest
    : RuntimeTestBase
    , ::testing::TestWithParam<runtime_kind> {
    runtime_session_holder holder_;

    i_runtime& make_session(
        size_t initial_var_count,
        size_t max_resolutions = kMaxResolutions) {
        return make_runtime_session(
            holder_,
            GetParam(),
            database,
            initial_goals,
            initial_var_count,
            max_resolutions,
            kSeed);
    }
};

#endif
