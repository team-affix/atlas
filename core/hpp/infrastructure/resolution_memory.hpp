#ifndef RESOLUTION_MEMORY_HPP
#define RESOLUTION_MEMORY_HPP

#include <unordered_set>
#include "../domain/interfaces/i_resolution_memory.hpp"

struct resolution_memory : i_resolution_memory {
    void insert(const resolution_lineage*) override;
    void clear() override;
    lemma derive_lemma() const override;
private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
