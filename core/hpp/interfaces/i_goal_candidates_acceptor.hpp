#ifndef I_GOAL_CANDIDATES_ACCEPTOR_HPP
#define I_GOAL_CANDIDATES_ACCEPTOR_HPP

#include "../interfaces/i_visitor.hpp"
#include "../value_objects/lineage.hpp"

struct i_goal_candidates_acceptor {
    virtual ~i_goal_candidates_acceptor() = default;
    virtual void accept(i_visitor<const resolution_lineage*>&) = 0;
};

#endif
