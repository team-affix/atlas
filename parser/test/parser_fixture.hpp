#ifndef PARSER_FIXTURE_HPP
#define PARSER_FIXTURE_HPP

#include <optional>
#include <gtest/gtest.h>
#include "infrastructure/locator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/var_sequencer.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"

struct ParserCoreFixture : public ::testing::Test {
    void SetUp() override {
        loc.bind_as<i_log_to_current_trail_frame>(t);
        pool.emplace(loc);
        var_seq.emplace(loc, 0u);
    }

    locator loc;
    trail t;
    std::optional<expr_pool> pool;
    std::optional<var_sequencer> var_seq;
};

#endif
