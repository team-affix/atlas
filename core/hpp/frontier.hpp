#ifndef FRONTIER_HPP
#define FRONTIER_HPP

#include <unordered_map>
#include "defs.hpp"

template<typename T>
struct frontier {
    virtual ~frontier() = default;
    frontier(
        const database&,
        lineage_pool&,
        trail&
    );
    void upsert(const goal_lineage*, const T&);
    void resolve(const resolution_lineage*);
    virtual std::vector<T> expand(const T&, const rule&) = 0;

    std::unordered_map<const goal_lineage*, T>::const_iterator begin() const;
    std::unordered_map<const goal_lineage*, T>::const_iterator end() const;
    const T& at(const goal_lineage*) const;
    size_t size() const;
    bool empty() const;
#ifndef DEBUG
private:
#endif
    const database& db;
    lineage_pool& lp;
    trail& t;

    std::unordered_map<const goal_lineage*, T> members;
};

template<typename T>
frontier<T>::frontier(
    const database& db,
    lineage_pool& lp,
    trail& t) : db(db), lp(lp), t(t) {}

template<typename T>
void frontier<T>::upsert(const goal_lineage* gl, const T& value) {
    auto [it, inserted] = members.insert({gl, value});

    if (inserted) {
        t.log(
            [this, gl]{members.erase(gl);},
            [this, gl, value]{members.insert({gl, value});}
        );
    }
    else {
        t.log(
            [this, gl, old = it->second]{members.at(gl) = old;},
            [this, gl, value]{members.at(gl) = value;}
        );
        it->second = value;
    }
}

template<typename T>
void frontier<T>::resolve(const resolution_lineage* r) {
    // get the parent
    const goal_lineage* parent = r->parent;
    
    // get the value of the parent
    const T& parent_value = members.at(parent);
    
    // expand the parent's value with the rule at the resolution index
    auto child_values = expand(parent_value, db.at(r->idx));

    // log the erasure of the parent
    t.log(
        [this, parent, val = parent_value]{members.insert({parent, val});},
        [this, parent]{members.erase(parent);}
    );

    // erase the parent from the frontier
    members.erase(parent);
    
    // add the children to the frontier
    for (int i = 0; i < child_values.size(); i++)
        upsert(lp.goal(r, i), child_values[i]);
}

template<typename T>
std::unordered_map<const goal_lineage*, T>::const_iterator frontier<T>::begin() const {
    return members.begin();
}

template<typename T>
std::unordered_map<const goal_lineage*, T>::const_iterator frontier<T>::end() const {
    return members.end();
}

template<typename T>
const T& frontier<T>::at(const goal_lineage* gl) const {
    return members.at(gl);
}

template<typename T>
size_t frontier<T>::size() const {
    return members.size();
}

template<typename T>
bool frontier<T>::empty() const {
    return members.empty();
}

#endif
