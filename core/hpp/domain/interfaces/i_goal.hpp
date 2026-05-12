#ifndef I_GOAL_HPP
#define I_GOAL_HPP

#include <cstddef>
#include <memory>
#include "i_set.hpp"
#include "i_map.hpp"
#include "i_candidate.hpp"
#include "../value_objects/expr.hpp"

struct i_goal {
    virtual ~i_goal() = default;
    virtual const expr* e() const = 0;
    virtual i_set<uint32_t>& e_reps() = 0;
    virtual i_map<size_t, std::unique_ptr<i_candidate>>& candidates() = 0;
};

#endif
