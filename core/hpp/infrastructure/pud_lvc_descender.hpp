#ifndef PUD_LVC_DESCENDER_HPP
#define PUD_LVC_DESCENDER_HPP

#include "value_objects/pud_lvc_descent.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedVarCount>
struct pud_lvc_descender {
    pud_lvc_descender(IGetNodeAddedVarCount& get_node_added_var_count);
    pud_lvc_descent root_descent(pud_node_id root);
    pud_lvc_descent descend(
        const pud_lvc_descent& descent,
        pud_node_id child);

private:
    IGetNodeAddedVarCount& get_node_added_var_count_;
};

template<typename IGetNodeAddedVarCount>
pud_lvc_descender<IGetNodeAddedVarCount>::pud_lvc_descender(
    IGetNodeAddedVarCount& get_node_added_var_count)
    : get_node_added_var_count_(get_node_added_var_count) {}

template<typename IGetNodeAddedVarCount>
pud_lvc_descent pud_lvc_descender<IGetNodeAddedVarCount>::root_descent(pud_node_id root) {
    const uint32_t var_count = get_node_added_var_count_.get(root);
    return pud_lvc_descent{.lvc = var_count};
}

template<typename IGetNodeAddedVarCount>
pud_lvc_descent pud_lvc_descender<IGetNodeAddedVarCount>::descend(
    const pud_lvc_descent& descent,
    pud_node_id child) {
    const uint32_t var_count = get_node_added_var_count_.get(child);
    return pud_lvc_descent{.lvc = descent.lvc + var_count};
}

#endif
