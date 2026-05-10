#ifndef DECISION_MEMORY_HPP
#define DECISION_MEMORY_HPP

#include <unordered_set>
#include "../domain/interfaces/i_decision_memory.hpp"

struct decision_memory : i_decision_memory {
    void insert(const resolution_lineage*) override;
    void clear() override;
    size_t size() const override;
    lemma derive_lemma() const override;
private:
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
