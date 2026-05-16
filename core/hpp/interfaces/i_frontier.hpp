#ifndef I_FRONTIER_HPP
#define I_FRONTIER_HPP

#include <cstddef>
#include <memory>
#include <utility>
#include "../value_objects/goal.hpp"
#include "../value_objects/lineage.hpp"
#include "i_visitor.hpp"

struct i_frontier {
    virtual ~i_frontier() = default;
    virtual void insert(const goal_lineage*, std::unique_ptr<goal>) = 0;
    virtual bool contains(const goal_lineage*) const = 0;
    virtual std::unique_ptr<goal>& at(const goal_lineage*) = 0;
    virtual const std::unique_ptr<goal>& at(const goal_lineage*) const = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void eliminate(const resolution_lineage*) = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
    virtual void accept(i_visitor<const std::pair<const goal_lineage* const, std::unique_ptr<goal>>&>&) const = 0;
};

#endif
