#include "value_objects/resolution_lineage_ptr_less.hpp"

bool resolution_lineage_ptr_less::operator()(
    const resolution_lineage* a, const resolution_lineage* b) const {
    return *a < *b;
}
