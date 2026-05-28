#ifndef I_IMPORT_RESOLUTION_LINEAGE_HPP
#define I_IMPORT_RESOLUTION_LINEAGE_HPP

#include "value_objects/lineage.hpp"

struct i_import_resolution_lineage {
    virtual ~i_import_resolution_lineage() = default;
    virtual const resolution_lineage* import(const resolution_lineage*) = 0;
};

#endif

