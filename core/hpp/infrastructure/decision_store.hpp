#ifndef DECISION_STORE_HPP
#define DECISION_STORE_HPP

#include <unordered_set>
#include "../../value_objects/lineage.hpp"

struct decision_store {
    decision_store();
    void insert(const resolution_lineage*);
    const std::unordered_set<const resolution_lineage*>& get() const;
    void clear();
#ifndef DEBUG
private:
#endif
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
