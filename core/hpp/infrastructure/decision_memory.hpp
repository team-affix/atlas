#ifndef DECISION_MEMORY_HPP
#define DECISION_MEMORY_HPP

#include <unordered_set>
#include "../interfaces/i_clear_decision_record.hpp"
#include "../interfaces/i_derive_decision_lemma.hpp"
#include "../interfaces/i_get_decision_count.hpp"
#include "../interfaces/i_record_decision.hpp"

struct decision_memory
    : i_record_decision
    , i_clear_decision_record
    , i_get_decision_count
    , i_derive_decision_lemma {
    void record_decision(const resolution_lineage*) override;
    void clear_decision_record() override;
    size_t count() const override;
    lemma derive() const override;
private:
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
