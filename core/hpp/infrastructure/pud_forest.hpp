#ifndef PUD_FOREST_HPP
#define PUD_FOREST_HPP

#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/om_label.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IMakeAxiom,
         typename IMakeInference,
         typename IAllocateRootInterval,
         typename IAllocateChildInterval>
struct pud_forest {
    pud_forest(IMakeAxiom& make_axiom,
               IMakeInference& make_inference,
               IAllocateRootInterval& allocate_root_interval,
               IAllocateChildInterval& allocate_child_interval);

    const pud_rule_id* add_axiom(size_t entry_idx,
                                 std::vector<pud_added_unification> added_unifications,
                                 std::vector<const expr*> added_body_goals,
                                 uint32_t lvc);
    const pud_rule_id* add_inference(const pud_rule_id* caller,
                                     size_t call_site,
                                     const pud_rule_id* callee,
                                     std::vector<pud_added_unification> added_unifications,
                                     std::vector<const expr*> added_body_goals,
                                     uint32_t lvc);
    void link_children(const pud_rule_id* parent,
                       const std::vector<const pud_rule_id*>& children);

    std::vector<const pud_rule_id*> ordered_children(const pud_rule_id* parent) const;
    const pud_rule_id* try_parent(const pud_rule_id* child) const;
    const pud_db_node& get_node(const pud_rule_id* id) const;
    bool is_leaf(const pud_rule_id* id) const;
    std::vector<const expr*> effective_body(const pud_rule_id* node) const;
private:
    struct rule_id_less {
        bool operator()(const pud_rule_id* a, const pud_rule_id* b) const;
    };
    using child_set_t = std::set<const pud_rule_id*, rule_id_less>;
    using map_t = std::unordered_map<const pud_rule_id*, pud_db_node>;
    using children_map_t = std::unordered_map<const pud_rule_id*, child_set_t>;
    using parent_map_t = std::unordered_map<const pud_rule_id*, const pud_rule_id*>;
    using set_t = std::unordered_set<const pud_rule_id*>;

    void store_node(const pud_rule_id* id,
                    om_interval interval,
                    std::vector<pud_added_unification> added_unifications,
                    std::vector<const expr*> added_body_goals,
                    uint32_t lvc);
    void link(const pud_rule_id* parent, child_set_t children);

    IMakeAxiom& make_axiom_;
    IMakeInference& make_inference_;
    IAllocateRootInterval& allocate_root_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    map_t by_id_;
    children_map_t children_;
    parent_map_t parents_;
    set_t leaves_;
};

template<typename IMakeAxiom, typename IMakeInference,
         typename IAllocateRootInterval, typename IAllocateChildInterval>
bool pud_forest<IMakeAxiom, IMakeInference, IAllocateRootInterval, IAllocateChildInterval>::
rule_id_less::operator()(const pud_rule_id* a, const pud_rule_id* b) const {
    return *a < *b;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
pud_forest<IMA, IMI, IARI, IACI>::pud_forest(IMA& make_axiom,
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
    , leaves_() {}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud_forest<IMA, IMI, IARI, IACI>::store_node(
        const pud_rule_id* id,
        om_interval interval,
        std::vector<pud_added_unification> added_unifications,
        std::vector<const expr*> added_body_goals,
        uint32_t lvc) {
    DEBUG_ASSERT(!by_id_.contains(id));
    by_id_.emplace(id, pud_db_node{
        interval,
        std::move(added_unifications),
        std::move(added_body_goals),
        lvc});
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud_forest<IMA, IMI, IARI, IACI>::add_axiom(
        size_t entry_idx,
        std::vector<pud_added_unification> added_unifications,
        std::vector<const expr*> added_body_goals,
        uint32_t lvc) {
    const pud_rule_id* id = make_axiom_.make_axiom(entry_idx);
    store_node(id,
               allocate_root_interval_.allocate_root(),
               std::move(added_unifications),
               std::move(added_body_goals),
               lvc);
    leaves_.insert(id);
    return id;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud_forest<IMA, IMI, IARI, IACI>::add_inference(
        const pud_rule_id* caller,
        size_t call_site,
        const pud_rule_id* callee,
        std::vector<pud_added_unification> added_unifications,
        std::vector<const expr*> added_body_goals,
        uint32_t lvc) {
    const pud_rule_id* id = make_inference_.make_inference(caller, call_site, callee);
    store_node(id,
               om_interval{om_label(nullptr), om_label(nullptr)},
               std::move(added_unifications),
               std::move(added_body_goals),
               lvc);
    return id;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud_forest<IMA, IMI, IARI, IACI>::link(const pud_rule_id* parent,
                                           child_set_t children) {
    DEBUG_ASSERT(leaves_.contains(parent));
    DEBUG_ASSERT(!children.empty());
    for (const pud_rule_id* child : children) {
        DEBUG_ASSERT(by_id_.contains(child));
        DEBUG_ASSERT(!parents_.contains(child));
        DEBUG_ASSERT(child != parent);
    }

    const om_interval parent_interval = by_id_.at(parent).interval;
    child_set_t& child_set = children_[parent];
    for (const pud_rule_id* child : children) {
        by_id_.at(child).interval =
            allocate_child_interval_.allocate_child_of(parent_interval);
        child_set.insert(child);
        parents_.emplace(child, parent);
        leaves_.insert(child);
    }
    leaves_.erase(parent);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
void pud_forest<IMA, IMI, IARI, IACI>::link_children(
        const pud_rule_id* parent,
        const std::vector<const pud_rule_id*>& children) {
    child_set_t child_set;
    for (const pud_rule_id* child : children)
        child_set.insert(child);
    link(parent, std::move(child_set));
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
std::vector<const pud_rule_id*>
pud_forest<IMA, IMI, IARI, IACI>::ordered_children(const pud_rule_id* parent) const {
    auto it = children_.find(parent);
    if (it == children_.end())
        return {};
    return std::vector<const pud_rule_id*>(it->second.begin(), it->second.end());
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_rule_id* pud_forest<IMA, IMI, IARI, IACI>::try_parent(
        const pud_rule_id* child) const {
    auto it = parents_.find(child);
    if (it == parents_.end())
        return nullptr;
    return it->second;
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
const pud_db_node& pud_forest<IMA, IMI, IARI, IACI>::get_node(const pud_rule_id* id) const {
    return by_id_.at(id);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
bool pud_forest<IMA, IMI, IARI, IACI>::is_leaf(const pud_rule_id* id) const {
    return leaves_.contains(id);
}

template<typename IMA, typename IMI, typename IARI, typename IACI>
std::vector<const expr*> pud_forest<IMA, IMI, IARI, IACI>::effective_body(
        const pud_rule_id* node) const {
    std::vector<const pud_rule_id*> path;
    const pud_rule_id* walk = node;
    while (walk != nullptr) {
        path.push_back(walk);
        walk = try_parent(walk);
    }
    std::vector<const expr*> body;
    for (size_t step_idx = path.size(); step_idx > 0; --step_idx) {
        const pud_rule_id* step = path[step_idx - 1];
        const bool has_parent = (step_idx != path.size());
        if (has_parent) {
            const pud_rule_id::inference& inf =
                std::get<pud_rule_id::inference>(step->content);
            DEBUG_ASSERT(inf.call_site < body.size());
            body.erase(body.begin() + static_cast<std::ptrdiff_t>(inf.call_site));
        }
        const std::vector<const expr*>& added = get_node(step).added_body_goals;
        body.insert(body.end(), added.begin(), added.end());
    }
    return body;
}

#endif
