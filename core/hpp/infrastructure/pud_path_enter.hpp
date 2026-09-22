#ifndef PUD_PATH_ENTER_HPP
#define PUD_PATH_ENTER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"

template<typename ITryEnter, typename IGetParent>
struct pud_path_enter {
    pud_path_enter(ITryEnter& try_enter, IGetParent& get_parent);
    const pud_rule_id* enter_to(const pud_rule_id* query_leaf,
                                size_t body_goal_idx,
                                uint32_t frame_offset,
                                const pud_rule_id* dest);
private:
    ITryEnter& try_enter_;
    IGetParent& get_parent_;
};

template<typename ITE, typename IGP>
pud_path_enter<ITE, IGP>::pud_path_enter(ITE& try_enter, IGP& get_parent)
    : try_enter_(try_enter)
    , get_parent_(get_parent) {}

template<typename ITE, typename IGP>
const pud_rule_id* pud_path_enter<ITE, IGP>::enter_to(
        const pud_rule_id* query_leaf,
        size_t body_goal_idx,
        uint32_t frame_offset,
        const pud_rule_id* dest) {
    if (dest == nullptr)
        return nullptr;
    std::vector<const pud_rule_id*> chain;
    for (const pud_rule_id* node = dest; node != nullptr;
            node = get_parent_.get(node))
        chain.push_back(node);
    pud_witness_search_context context{
        query_leaf, body_goal_idx, nullptr, frame_offset, dest, dest};
    const pud_rule_id* last = nullptr;
    for (size_t idx = chain.size(); idx > 0; --idx) {
        if (!try_enter_.try_enter(context, chain[idx - 1]))
            return last;
        last = chain[idx - 1];
    }
    return last;
}

#endif
