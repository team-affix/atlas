#ifndef GOALS_HPP
#define GOALS_HPP

#include <vector>
#include "../value_objects/goal.hpp"

struct goals {
    goals(const std::vector<goal>&);
    const std::vector<goal>& get() const;
#ifndef DEBUG
private:
#endif
    std::vector<goal> gls;
};

#endif
