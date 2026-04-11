#ifndef FRONTIER_HPP
#define FRONTIER_HPP

#include "delta_map.hpp"
#include "defs.hpp"

template<typename T>
struct frontier : delta<std::map<const goal_lineage*, T>> {
    virtual ~frontier() = default;
    frontier(
        const database&,
        lineage_pool&,
        trail&
    );
    bool resolve(const resolution_lineage*);
    virtual std::optional<std::vector<T>> expand(const T&, const rule&) = 0;
#ifndef DEBUG
private:
#endif
    const database& db;
    lineage_pool& lp;
};

template<typename T>
frontier<T>::frontier(
    const database& db,
    lineage_pool& lp,
    trail& t)
    :
    delta<std::map<const goal_lineage*, T>>(t, {}),
    db(db),
    lp(lp) {}

template<typename T>
bool frontier<T>::resolve(const resolution_lineage* r) {
    // get the parent
    const goal_lineage* parent = r->parent;
    
    // get the value of the parent
    const T& parent_value =
        delta<std::map<const goal_lineage*, T>>::get().at(parent);
    
    // expand the parent's value with the rule at the resolution index
    auto child_values = expand(parent_value, db.at(r->idx));

    // if the expansion failed, return false
    if (!child_values.has_value())
        return false;

    // erase the parent from the frontier
    delta<std::map<const goal_lineage*, T>>::erase(parent);
    
    // add the children to the frontier (children should not be in frontier yet)
    for (int i = 0; i < child_values.size(); i++)
        insert(lp.goal(r, i), child_values[i]);

    return true;
}

#endif
