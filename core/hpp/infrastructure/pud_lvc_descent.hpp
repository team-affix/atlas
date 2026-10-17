#ifndef PUD_LVC_DESCENT_HPP
#define PUD_LVC_DESCENT_HPP

#include <cstdint>
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedVarCount>
struct pud_lvc_descent {
    pud_lvc_descent(IGetNodeAddedVarCount& get_node_added_var_count, pud_node_id root);
    pud_lvc_descent descend(pud_node_id child) const;
    uint32_t lvc() const;
private:
    // Pointers rather than references, so the descent stays copy-assignable.
    IGetNodeAddedVarCount* get_node_added_var_count_;
    uint32_t lvc_;
};

template<typename IGetNodeAddedVarCount>
pud_lvc_descent<IGetNodeAddedVarCount>::pud_lvc_descent(
    IGetNodeAddedVarCount& get_node_added_var_count,
    pud_node_id root)
    : get_node_added_var_count_(&get_node_added_var_count)
    , lvc_(get_node_added_var_count.get(root)) {}

template<typename IGetNodeAddedVarCount>
pud_lvc_descent<IGetNodeAddedVarCount> pud_lvc_descent<IGetNodeAddedVarCount>::descend(
    pud_node_id child) const {
    pud_lvc_descent result = *this;
    result.lvc_ += get_node_added_var_count_->get(child);
    return result;
}

template<typename IGetNodeAddedVarCount>
uint32_t pud_lvc_descent<IGetNodeAddedVarCount>::lvc() const {
    return lvc_;
}

#endif
