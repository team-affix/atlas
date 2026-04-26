#ifndef GOAL_EXPANDER_HPP
#define GOAL_EXPANDER_HPP

#include <map>
#include "../value_objects/expr.hpp"
#include "../value_objects/rule.hpp"
#include "bind_map.hpp"
#include "copier.hpp"

struct goal_expander {
    goal_expander(
        const expr* const& goal,
        const rule& r,
        bind_map&,
        copier&
    );
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
