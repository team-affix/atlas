#ifndef PUD_AXIOM_REGISTRAR_HPP
#define PUD_AXIOM_REGISTRAR_HPP

#include <set>
#include <utility>
#include <vector>
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

// Registers rule roots into axiom groups. register_axiom_group registers every rule in the batch,
// then activates them together so each query's try_open sees the full group membership.
template<
    typename IMakeRoot,
    typename IStoreNodeAxiomGroup,
    typename IAddAxiomGroupMember,
    typename IActivateRoot,
    typename IRefuteLeaf,
    typename IDeactivateNode,
    typename IGetQueryHolders,
    typename ICollectChange>
struct pud_axiom_registrar {
    pud_axiom_registrar(
        IMakeRoot& make_root,
        IStoreNodeAxiomGroup& store_node_axiom_group,
        IAddAxiomGroupMember& add_axiom_group_member,
        IActivateRoot& activate_root,
        IRefuteLeaf& refute_leaf,
        IDeactivateNode& deactivate_node,
        IGetQueryHolders& get_query_holders,
        ICollectChange& collect_change);
    pud_node_id register_axiom(const rule& axiom, pud_axiom_group_id group);
    pud_frontier_change register_axiom_group(pud_axiom_group_id group, const std::vector<rule>& axioms);
    pud_frontier_change activate_axiom_group(pud_axiom_group_id group, const std::vector<pud_node_id>& roots);

    IMakeRoot& make_root_;
    IStoreNodeAxiomGroup& store_node_axiom_group_;
    IAddAxiomGroupMember& add_axiom_group_member_;
    IActivateRoot& activate_root_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
    IGetQueryHolders& get_query_holders_;
    ICollectChange& collect_change_;
};

#define PUD_AXIOM_REGISTRAR_TEMPLATE                                                       \
    template<typename IMakeRoot, typename IStoreNodeAxiomGroup, typename IAddAxiomGroupMember, \
             typename IActivateRoot, typename IRefuteLeaf, typename IDeactivateNode,         \
             typename IGetQueryHolders, typename ICollectChange>
#define PUD_AXIOM_REGISTRAR                                                                \
    pud_axiom_registrar<IMakeRoot, IStoreNodeAxiomGroup, IAddAxiomGroupMember, IActivateRoot, IRefuteLeaf, \
        IDeactivateNode, IGetQueryHolders, ICollectChange>

PUD_AXIOM_REGISTRAR_TEMPLATE
PUD_AXIOM_REGISTRAR::pud_axiom_registrar(
    IMakeRoot& make_root,
    IStoreNodeAxiomGroup& store_node_axiom_group,
    IAddAxiomGroupMember& add_axiom_group_member,
    IActivateRoot& activate_root,
    IRefuteLeaf& refute_leaf,
    IDeactivateNode& deactivate_node,
    IGetQueryHolders& get_query_holders,
    ICollectChange& collect_change)
    : make_root_(make_root)
    , store_node_axiom_group_(store_node_axiom_group)
    , add_axiom_group_member_(add_axiom_group_member)
    , activate_root_(activate_root)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node)
    , get_query_holders_(get_query_holders)
    , collect_change_(collect_change) {}

PUD_AXIOM_REGISTRAR_TEMPLATE
pud_node_id PUD_AXIOM_REGISTRAR::register_axiom(const rule& axiom, pud_axiom_group_id group) {
    const pud_node_id root = make_root_.make_root(axiom);
    store_node_axiom_group_.store_node_group(root, group);
    add_axiom_group_member_.add_member(group, root);
    return root;
}

PUD_AXIOM_REGISTRAR_TEMPLATE
pud_frontier_change PUD_AXIOM_REGISTRAR::register_axiom_group(
    pud_axiom_group_id group,
    const std::vector<rule>& axioms) {
    std::vector<pud_node_id> roots;
    for (const rule& axiom : axioms)
        roots.push_back(register_axiom(axiom, group));
    return activate_axiom_group(group, roots);
}

PUD_AXIOM_REGISTRAR_TEMPLATE
pud_frontier_change PUD_AXIOM_REGISTRAR::activate_axiom_group(
    pud_axiom_group_id group,
    const std::vector<pud_node_id>& roots) {
    (void)group;
    std::vector<pud_node_id> refuted_roots;
    for (pud_node_id root : roots) {
        if (!activate_root_.activate_root(root))
            continue;
        refuted_roots.push_back(root);
    }
    std::vector<pud_node_id> all_refuted;
    std::vector<pud_node_id> releasing;
    std::vector<pud_node_id> refuting = refuted_roots;
    while (!releasing.empty() || !refuting.empty()) {
        for (pud_node_id node : refuting) {
            refute_leaf_.refute(node);
            all_refuted.push_back(node);
            releasing.push_back(node);
        }
        refuting.clear();
        const std::vector<pud_node_id> releasable = std::move(releasing);
        releasing.clear();
        std::set<pud_node_id> next_refuting;
        for (pud_node_id node : releasable) {
            for (pud_mhq_head_id head_id : deactivate_node_.deactivate(node)) {
                const std::vector<pud_node_id> held_by = get_query_holders_.get_holders(head_id);
                next_refuting.insert(held_by.begin(), held_by.end());
            }
        }
        refuting.assign(next_refuting.begin(), next_refuting.end());
    }
    for (pud_node_id root : roots)
        collect_change_.add_produced(root);
    for (pud_node_id refuted : all_refuted)
        collect_change_.add_refuted(refuted);
    return collect_change_.finish();
}

#undef PUD_AXIOM_REGISTRAR
#undef PUD_AXIOM_REGISTRAR_TEMPLATE

#endif
