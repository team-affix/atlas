#ifndef I_UNIFY_SYNCHRONIZER_HPP
#define I_UNIFY_SYNCHRONIZER_HPP

#include <cstdint>
#include "../value_objects/lineage.hpp"

struct i_unify_synchronizer {
    virtual ~i_unify_synchronizer() = default;
    virtual void primary_rep_changed(uint32_t var_index) = 0;
    virtual void register_candidate(const resolution_lineage*) = 0;
    virtual void unregister_candidate(const resolution_lineage*) = 0;
    virtual void accept(const resolution_lineage*) = 0;
};

#endif
