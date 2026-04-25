#ifndef FRONTIER_HPP
#define FRONTIER_HPP

#include <unordered_map>
#include "defs.hpp"

template<typename T, typename Expander>
struct frontier {
    virtual ~frontier() = default;
    frontier(
        const database&,
        lineage_pool&
    );
    void insert(const goal_lineage*, const T&);
    void resolve(const resolution_lineage*);
    virtual void on_insert(const goal_lineage*, const T&);
    virtual void on_resolve(const resolution_lineage*);

    T& at(const goal_lineage*);
    const T& at(const goal_lineage*) const;
    bool empty() const;
#ifndef DEBUG
private:
#endif
    const database& db;
    lineage_pool& lp;

    std::unordered_map<const goal_lineage*, T> members;
};

template<typename T, typename Expander>
frontier<T, Expander>::frontier(
    const database& db,
    lineage_pool& lp) : db(db), lp(lp) {}

template<typename T, typename Expander>
void frontier<T, Expander>::insert(const goal_lineage* gl, const T& value) {
    members.insert({gl, value});
    on_insert(gl, value);
}

template<typename T, typename Expander>
void frontier<T, Expander>::resolve(const resolution_lineage* r) {
    // get the parent
    const goal_lineage* parent = r->parent;
    
    // get the value of the parent
    const T& parent_value = members.at(parent);

    // get the rule at the resolution index
    const rule& db_rule = db.at(r->idx);
    
    // create the expander for the parent
    Expander exp(parent_value);
    
    // add the children to the frontier
    for (int i = 0; i < db_rule.body.size(); i++)
        insert(lp.goal(r, i), exp());

    // erase the parent from the frontier
    members.erase(parent);

    // notify the subclass
    on_resolve(r);
}

template<typename T, typename Expander>
void frontier<T, Expander>::on_insert(const goal_lineage* gl, const T& value) {
}

template<typename T, typename Expander>
void frontier<T, Expander>::on_resolve(const resolution_lineage* r) {
}

template<typename T, typename Expander>
T& frontier<T, Expander>::at(const goal_lineage* gl) {
    return members.at(gl);
}

template<typename T, typename Expander>
const T& frontier<T, Expander>::at(const goal_lineage* gl) const {
    return members.at(gl);
}

template<typename T, typename Expander>
bool frontier<T, Expander>::empty() const {
    return members.empty();
}

#endif
