#ifndef DECISION_STORE_HPP
#define DECISION_STORE_HPP

#include <unordered_set>
#include "../../../infrastructure/tracked_set.hpp"
#include "../../value_objects/lineage.hpp"

struct decision_store {
    decision_store();
    void insert(const resolution_lineage*);
    const std::unordered_set<const resolution_lineage*>& get() const;
#ifndef DEBUG
private:
#endif
    tracked_set<std::unordered_set<const resolution_lineage*>> decisions;
};

#endif
