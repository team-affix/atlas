#ifndef PARSER_FIXTURE_HPP
#define PARSER_FIXTURE_HPP

#include <map>
#include <optional>
#include <gtest/gtest.h>
#include "infrastructure/functor_names.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"

struct ParserCoreFixture : public ::testing::Test {
    void SetUp() override {
        pool.emplace();
        var_seq = non_backtracking_var_sequencer{};
        functor_map.clear();
        next_functor_id = k_first_user_functor_id;
    }

    std::optional<expr_pool> pool;
    non_backtracking_var_sequencer var_seq;
    std::map<std::string, uint32_t> functor_map;
    uint32_t next_functor_id = k_first_user_functor_id;
};

#endif
