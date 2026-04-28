#ifndef RESOLUTION_STORE_HPP
#define RESOLUTION_STORE_HPP

#include <unordered_set>
#include "../../value_objects/lineage.hpp"

struct resolution_store {
    resolution_store();
    void insert(const resolution_lineage*);
    const std::unordered_set<const resolution_lineage*>& get() const;
    void clear();
#ifndef DEBUG
private:
#endif
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
