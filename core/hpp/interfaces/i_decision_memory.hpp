#ifndef I_DECISION_MEMORY_HPP
#define I_DECISION_MEMORY_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/lemma.hpp"

struct i_decision_memory {
    virtual ~i_decision_memory() = default;
    virtual void insert(const resolution_lineage*) = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
    virtual lemma derive_lemma() const = 0;
};

#endif
