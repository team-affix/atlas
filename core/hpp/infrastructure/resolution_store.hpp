#ifndef RESOLUTION_STORE_HPP
#define RESOLUTION_STORE_HPP

#include <unordered_set>
#include "../domain/interfaces/i_resolution_store.hpp"

struct resolution_store : i_resolution_store {
    void insert(const resolution_lineage*) override;
    void clear() override;
    lemma derive_lemma() override;
private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
