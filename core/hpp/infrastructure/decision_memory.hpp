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

inline void decision_memory::record_decision(const resolution_lineage* rl) {
    decisions.insert(rl);
}

inline void decision_memory::clear_recorded_decisions() {
    decisions.clear();
}

inline size_t decision_memory::count() const {
    return decisions.size();
}

inline lemma decision_memory::derive_decision_lemma() const {
    return lemma{decisions};
}

#endif
