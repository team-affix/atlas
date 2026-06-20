#ifndef DECISION_MEMORY_HPP
#define DECISION_MEMORY_HPP

#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

struct decision_memory {
    void record_decision(const resolution_lineage*);
    void clear_recorded_decisions();
    size_t count() const;
    lemma derive_decision_lemma() const;
private:
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
