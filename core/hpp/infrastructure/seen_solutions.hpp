#ifndef SEEN_SOLUTIONS_HPP
#define SEEN_SOLUTIONS_HPP

#include <cstddef>
#include <unordered_set>
#include <vector>
#include "value_objects/lemma.hpp"

struct seen_solutions {
    seen_solutions();
    bool is_repeat_solution(const lemma&) const;
    void remember_solution(const lemma&);
private:
    using key_t = std::vector<const resolution_lineage*>;
    struct key_hash {
        size_t operator()(const key_t&) const noexcept;
    };
    key_t make_key(const lemma&) const;

    std::unordered_set<key_t, key_hash> seen_;
};

#endif
