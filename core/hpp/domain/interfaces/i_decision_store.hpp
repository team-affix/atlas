#ifndef I_DECISION_STORE_HPP
#define I_DECISION_STORE_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/lemma.hpp"

struct i_decision_store {
    virtual ~i_decision_store() = default;
    virtual void insert(const resolution_lineage*) = 0;
    virtual void clear() = 0;
    virtual size_t size() = 0;
    virtual lemma derive_lemma() = 0;
};

#endif
