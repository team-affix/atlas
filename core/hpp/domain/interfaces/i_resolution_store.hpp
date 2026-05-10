#ifndef I_RESOLUTION_STORE_HPP
#define I_RESOLUTION_STORE_HPP

#include "../value_objects/lineage.hpp"

template<typename T>
struct i_resolution_store {
    virtual ~i_resolution_store() = default;
    virtual void insert(const resolution_lineage*, T) = 0;
    virtual T& get(const resolution_lineage*) = 0;
    virtual void erase(const resolution_lineage*) = 0;
    virtual void clear() = 0;
};

#endif
