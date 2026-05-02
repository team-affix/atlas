#ifndef DECISION_STORE_HPP
#define DECISION_STORE_HPP

#include <unordered_set>
#include "../domain/interfaces/i_decision_store.hpp"

struct decision_store : i_decision_store {
    void insert(const resolution_lineage*) override;
    void clear() override;
    size_t size() override;
    lemma derive_lemma() override;
private:
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
