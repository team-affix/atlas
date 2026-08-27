#ifndef RESOLUTION_LINEAGE_PTR_LESS_HPP
#define RESOLUTION_LINEAGE_PTR_LESS_HPP

#include "value_objects/lineage.hpp"

struct resolution_lineage_ptr_less {
    bool operator()(const resolution_lineage* a, const resolution_lineage* b) const;
};

#endif
