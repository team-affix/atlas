#ifndef PUD_HPP
#define PUD_HPP

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IMakeAxiom,
         typename IMakeInference,
         typename IAllocateRootInterval,
         typename IAllocateChildInterval>
struct pud {
    struct rule_id_less {
        bool operator()(const pud_rule_id* a, const pud_rule_id* b) const;
    };
    using child_set_t = std::set<const pud_rule_id*, rule_id_less>;

    pud(IMakeAxiom& make_axiom,
        IMakeInference& make_inference,
        IAllocateRootInterval& allocate_root_interval,
        IAllocateChildInterval& allocate_child_interval);

    const pud_rule_id* add_axiom(size_t entry_idx, pud_db_node node);
    const pud_rule_id* add_inference(const pud_rule_id* caller,
                                     size_t call_site,
                                     const pud_rule_id* callee,
                                     pud_db_node node);
    void insert(const pud_rule_id* id, pud_db_node node);
    void link(const pud_rule_id* parent, child_set_t children);
    void link_child(const pud_rule_id* parent, const pud_rule_id* child);
    void unlink(const pud_rule_id* child);
    void erase(const pud_rule_id* id);

    const std::unordered_set<const pud_rule_id*>& roots() const;
    const std::unordered_set<const pud_rule_id*>& leaves() const;
    std::vector<const pud_rule_id*> ordered_roots() const;
    std::vector<const pud_rule_id*> ordered_leaves() const;
    const child_set_t& children(const pud_rule_id* parent) const;
    std::vector<const pud_rule_id*> ordered_children(const pud_rule_id* parent) const;
    const pud_rule_id* parent(const pud_rule_id* child) const;
    const pud_rule_id* try_parent(const pud_rule_id* child) const;
    const pud_db_node& get_node(const pud_rule_id* id) const;
    bool is_leaf(const pud_rule_id* id) const;

private:
    using map_t = std::unordered_map<const pud_rule_id*, pud_db_node>;
    using children_map_t = std::unordered_map<const pud_rule_id*, child_set_t>;
    using parent_map_t = std::unordered_map<const pud_rule_id*, const pud_rule_id*>;
    using set_t = std::unordered_set<const pud_rule_id*>;

    void insert_isolated(const pud_rule_id* id, pud_db_node node);
    std::vector<const pud_rule_id*> ordered_ids(const set_t& ids) const;

    IMakeAxiom& make_axiom_;
    IMakeInference& make_inference_;
    IAllocateRootInterval& allocate_root_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    map_t by_id_;
    children_map_t children_;
    parent_map_t parents_;
    set_t roots_;
    set_t leaves_;
};

template<typename IMakeAxiom, typename IMakeInference,
         typename IAllocateRootInterval, typename IAllocateChildInterval>
bool pud<IMakeAxiom, IMakeInference, IAllocateRootInterval, IAllocateChildInterval>::
rule_id_less::operator()(const pud_rule_id* a, const pud_rule_id* b) const {
    return *a < *b;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
pud<IMA, IMI, IARI, IACI>::pud(IMA& make_axiom,
                               IMI& make_inference,
                               IARI& allocate_root_interval,
                               IACI& allocate_child_interval)
    : make_axiom_(make_axiom)
    , make_inference_(make_inference)
    , allocate_root_interval_(allocate_root_interval)
    , allocate_child_interval_(allocate_child_interval)
    , by_id_()
    , children_()
    , parents_()
    , roots_()
    , leaves_() {}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud<IMA, IMI, IARI, IACI>::insert_isolated(const pud_rule_id* id, pud_db_node node) {
    DEBUG_ASSERT(!by_id_.contains(id));
    node.interval = allocate_root_interval_.allocate_root();
    by_id_.emplace(id, node);
    roots_.insert(id);
    leaves_.insert(id);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud<IMA, IMI, IARI, IACI>::add_axiom(size_t entry_idx, pud_db_node node) {
    const pud_rule_id* id = make_axiom_.make_axiom(entry_idx);
    insert_isolated(id, node);
    return id;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud<IMA, IMI, IARI, IACI>::add_inference(const pud_rule_id* caller,
                                                           size_t call_site,
                                                           const pud_rule_id* callee,
                                                           pud_db_node node) {
    const pud_rule_id* id = make_inference_.make_inference(caller, call_site, callee);
    insert_isolated(id, node);
    return id;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud<IMA, IMI, IARI, IACI>::insert(const pud_rule_id* id, pud_db_node node) {
    insert_isolated(id, node);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud<IMA, IMI, IARI, IACI>::link(const pud_rule_id* parent, child_set_t children) {
    DEBUG_ASSERT(leaves_.contains(parent));
    DEBUG_ASSERT(!children.empty());
    for (const pud_rule_id* child : children) {
        DEBUG_ASSERT(roots_.contains(child));
        DEBUG_ASSERT(child != parent);
    }

    const om_interval parent_interval = by_id_.at(parent).interval;
    child_set_t& child_set = children_[parent];
    for (const pud_rule_id* child : children) {
        by_id_.at(child).interval =
            allocate_child_interval_.allocate_child_of(parent_interval);
        child_set.insert(child);
        parents_.emplace(child, parent);
        roots_.erase(child);
    }
    leaves_.erase(parent);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud<IMA, IMI, IARI, IACI>::link_child(const pud_rule_id* parent,
                                          const pud_rule_id* child) {
    link(parent, child_set_t{child});
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud<IMA, IMI, IARI, IACI>::unlink(const pud_rule_id* child) {
    auto parent_it = parents_.find(child);
    DEBUG_ASSERT(parent_it != parents_.end());
    const pud_rule_id* parent = parent_it->second;
    parents_.erase(parent_it);

    auto& child_set = children_.at(parent);
    child_set.erase(child);
    if (child_set.empty()) {
        children_.erase(parent);
        leaves_.insert(parent);
    }

    roots_.insert(child);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud<IMA, IMI, IARI, IACI>::erase(const pud_rule_id* id) {
    DEBUG_ASSERT(roots_.contains(id));
    DEBUG_ASSERT(leaves_.contains(id));
    by_id_.erase(id);
    roots_.erase(id);
    leaves_.erase(id);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const std::unordered_set<const pud_rule_id*>&
pud<IMA, IMI, IARI, IACI>::roots() const {
    return roots_;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const std::unordered_set<const pud_rule_id*>&
pud<IMA, IMI, IARI, IACI>::leaves() const {
    return leaves_;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
std::vector<const pud_rule_id*> pud<IMA, IMI, IARI, IACI>::ordered_ids(
        const set_t& ids) const {
    std::vector<const pud_rule_id*> out(ids.begin(), ids.end());
    std::sort(out.begin(), out.end(), rule_id_less{});
    return out;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
std::vector<const pud_rule_id*> pud<IMA, IMI, IARI, IACI>::ordered_roots() const {
    return ordered_ids(roots_);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
std::vector<const pud_rule_id*> pud<IMA, IMI, IARI, IACI>::ordered_leaves() const {
    return ordered_ids(leaves_);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const typename pud<IMA, IMI, IARI, IACI>::child_set_t&
pud<IMA, IMI, IARI, IACI>::children(const pud_rule_id* parent) const {
    return children_.at(parent);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
std::vector<const pud_rule_id*>
pud<IMA, IMI, IARI, IACI>::ordered_children(const pud_rule_id* parent) const {
    auto it = children_.find(parent);
    if (it == children_.end())
        return {};
    return std::vector<const pud_rule_id*>(it->second.begin(), it->second.end());
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud<IMA, IMI, IARI, IACI>::parent(const pud_rule_id* child) const {
    return parents_.at(child);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud<IMA, IMI, IARI, IACI>::try_parent(const pud_rule_id* child) const {
    auto it = parents_.find(child);
    if (it == parents_.end())
        return nullptr;
    return it->second;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_db_node& pud<IMA, IMI, IARI, IACI>::get_node(const pud_rule_id* id) const {
    return by_id_.at(id);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
bool pud<IMA, IMI, IARI, IACI>::is_leaf(const pud_rule_id* id) const {
    return leaves_.contains(id);
}

#endif
