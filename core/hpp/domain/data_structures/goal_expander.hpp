#ifndef GOAL_EXPANDER_HPP
#define GOAL_EXPANDER_HPP

#include <map>
#include "expr.hpp"
#include "rule.hpp"
#include "copier.hpp"

struct goal_expander {
    goal_expander(
        const expr* const& goal,
        const rule& r);
    const expr* operator()();
#ifndef DEBUG
private:
#endif
    copier& cp;

    std::map<uint32_t, uint32_t> translation_map;
    std::vector<const expr*> rule_body;
    size_t subgoal_index;
};

#endif
