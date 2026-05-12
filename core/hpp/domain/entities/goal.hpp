#ifndef GOAL_HPP
#define GOAL_HPP

#include <memory>
#include "../interfaces/i_goal.hpp"
#include "../../infrastructure/unordered_set.hpp"
#include "../../infrastructure/unordered_map.hpp"

struct goal : i_goal {
    explicit goal(const expr* e);
    const expr* e() const override;
    i_set<uint32_t>& e_reps() override;
    i_map<size_t, std::unique_ptr<i_candidate>>& candidates() override;
private:
    const expr* e_;
    unordered_set<uint32_t> e_reps_;
    unordered_map<size_t, std::unique_ptr<i_candidate>> candidates_;
};

#endif
