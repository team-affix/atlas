#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "../interfaces/i_bind_map.hpp"
#include "../interfaces/i_overlay_bind_map.hpp"
#include "../interfaces/i_unifier.hpp"

struct candidate {
    virtual ~candidate() = default;
    candidate(
        const std::vector<const expr*>& rule_body,
        std::unordered_map<uint32_t, uint32_t> tm,
        std::unique_ptr<i_bind_map> local_bind_map,
        std::unique_ptr<i_overlay_bind_map> overlay_bind_map,
        std::unique_ptr<i_unifier> unifier);
    const std::vector<const expr*>& rule_body;
    std::unordered_map<uint32_t, uint32_t> tm;
    std::unique_ptr<i_bind_map> local_bind_map;
    std::unique_ptr<i_overlay_bind_map> overlay_bind_map;
    std::unique_ptr<i_unifier> unifier;
};

#endif
