#include "infrastructure/seen_solutions.hpp"
#include <algorithm>
#include <functional>

seen_solutions::seen_solutions() : seen_() {}

bool seen_solutions::is_repeat_solution(const lemma& l) const {
    return seen_.contains(make_key(l));
}

void seen_solutions::remember_solution(const lemma& l) {
    seen_.insert(make_key(l));
}

seen_solutions::key_t seen_solutions::make_key(const lemma& l) const {
    key_t key(l.get_resolutions().begin(), l.get_resolutions().end());
    std::sort(key.begin(), key.end());
    return key;
}

size_t seen_solutions::key_hash::operator()(const key_t& key) const noexcept {
    size_t seed = key.size();
    for (const resolution_lineage* rl : key) {
        const size_t value = std::hash<const resolution_lineage*>{}(rl);
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}
