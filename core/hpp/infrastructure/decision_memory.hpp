#ifndef DECISION_MEMORY_HPP
#define DECISION_MEMORY_HPP

#include <unordered_set>
#include "../interfaces/i_decision_memory.hpp"

struct decision_memory : i_decision_memory {
    void record_decision(const resolution_lineage*) override;
    void clear_decision_record() override;
    size_t get_decision_count() const override;
    lemma derive_decision_lemma() const override;
private:
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
