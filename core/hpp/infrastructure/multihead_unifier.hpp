#ifndef MULTIHEAD_UNIFIER_HPP
#define MULTIHEAD_UNIFIER_HPP

#include <unordered_map>
#include <unordered_set>
#include "../domain/interfaces/i_multihead_unifier.hpp"
#include "../domain/interfaces/i_database.hpp"

struct multihead_unifier : i_multihead_unifier {
    virtual ~multihead_unifier() = default;
    multihead_unifier();
    void add_head(const resolution_lineage*) override {}
    void remove_head(const resolution_lineage*) override {}
    void accept(const resolution_lineage*) override {}
    void re_root(uint32_t) override {}
private:
    i_database* db_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_heads_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> head_to_reps_;
};

#endif