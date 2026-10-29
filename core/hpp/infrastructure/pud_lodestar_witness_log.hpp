#ifndef PUD_LODESTAR_WITNESS_LOG_HPP
#define PUD_LODESTAR_WITNESS_LOG_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_advance.hpp"

template<typename IMhws, typename IModules>
struct pud_lodestar_witness_log {
    pud_lodestar_witness_log(IMhws& inner, IModules& modules);
    std::optional<pud_mhws_head_id> try_add(pud_replay search_root);
    std::optional<pud_mhws_head_id> try_fork(pud_mhws_head_id head_id, pud_replay search_root);
    std::optional<pud_witness_advance> advance(pud_mhws_head_id head_id);
    void remove(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> invalidate_leaf(pud_global_node_id leaf);
    void arm();
    std::vector<pud_mhws_head_id> drain();
    std::optional<pud_global_node_id> node_of(pud_mhws_head_id head_id) const;
private:
    void remember(pud_mhws_head_id head_id, pud_global_node_id node);
    pud_global_node_id settle(pud_global_node_id node) const;

    IMhws& inner_;
    IModules& modules_;
    std::unordered_map<pud_mhws_head_id, pud_global_node_id> nodes_;
    std::vector<pud_mhws_head_id> armed_adds_;
    bool armed_;
};

template<typename IMhws, typename IModules>
pud_lodestar_witness_log<IMhws, IModules>::pud_lodestar_witness_log(IMhws& inner, IModules& modules)
    : inner_(inner)
    , modules_(modules)
    , nodes_()
    , armed_adds_()
    , armed_(false) {}

template<typename IMhws, typename IModules>
void pud_lodestar_witness_log<IMhws, IModules>::remember(pud_mhws_head_id head_id, pud_global_node_id node) {
    nodes_[head_id] = settle(node);
    if (!armed_)
        return;
    armed_adds_.push_back(head_id);
}

template<typename IMhws, typename IModules>
pud_global_node_id pud_lodestar_witness_log<IMhws, IModules>::settle(pud_global_node_id node) const {
    if (modules_.check_refuted(node))
        return node;
    if (modules_.check_leaf(node))
        return node;
    for (const pud_node_id child_id : modules_.get_children(node)) {
        const pud_global_node_id child{.module = node.module, .node = child_id};
        if (modules_.check_refuted(child))
            continue;
        return settle(child);
    }
    return node;
}

template<typename IMhws, typename IModules>
std::optional<pud_mhws_head_id> pud_lodestar_witness_log<IMhws, IModules>::try_add(pud_replay search_root) {
    const pud_global_node_id root = search_root.node;
    const std::optional<pud_mhws_head_id> head_id = inner_.try_add(std::move(search_root));
    if (!head_id.has_value())
        return std::nullopt;
    remember(head_id.value(), root);
    return head_id;
}

template<typename IMhws, typename IModules>
std::optional<pud_mhws_head_id> pud_lodestar_witness_log<IMhws, IModules>::try_fork(
    pud_mhws_head_id head_id, pud_replay search_root) {
    const pud_global_node_id root = search_root.node;
    const std::optional<pud_mhws_head_id> forked = inner_.try_fork(head_id, std::move(search_root));
    if (!forked.has_value())
        return std::nullopt;
    remember(forked.value(), root);
    return forked;
}

template<typename IMhws, typename IModules>
std::optional<pud_witness_advance> pud_lodestar_witness_log<IMhws, IModules>::advance(pud_mhws_head_id head_id) {
    std::optional<pud_witness_advance> advanced = inner_.advance(head_id);
    if (!advanced.has_value()) {
        nodes_.erase(head_id);
        return std::nullopt;
    }
    nodes_[head_id] = settle(advanced->search_root.node);
    return advanced;
}

template<typename IMhws, typename IModules>
void pud_lodestar_witness_log<IMhws, IModules>::remove(pud_mhws_head_id head_id) {
    nodes_.erase(head_id);
    inner_.remove(head_id);
}

template<typename IMhws, typename IModules>
std::vector<pud_mhws_head_id> pud_lodestar_witness_log<IMhws, IModules>::invalidate_leaf(pud_global_node_id leaf) {
    const std::vector<pud_mhws_head_id> exhausted = inner_.invalidate_leaf(leaf);
    for (const pud_mhws_head_id head_id : exhausted)
        nodes_.erase(head_id);
    for (auto& [head_id, node] : nodes_) {
        if (node != leaf)
            continue;
        node = settle(leaf);
    }
    return exhausted;
}

template<typename IMhws, typename IModules>
void pud_lodestar_witness_log<IMhws, IModules>::arm() {
    armed_ = true;
    armed_adds_.clear();
}

template<typename IMhws, typename IModules>
std::vector<pud_mhws_head_id> pud_lodestar_witness_log<IMhws, IModules>::drain() {
    armed_ = false;
    std::vector<pud_mhws_head_id> added = std::move(armed_adds_);
    armed_adds_.clear();
    return added;
}

template<typename IMhws, typename IModules>
std::optional<pud_global_node_id> pud_lodestar_witness_log<IMhws, IModules>::node_of(pud_mhws_head_id head_id) const {
    const auto found = nodes_.find(head_id);
    if (found == nodes_.end())
        return std::nullopt;
    return found->second;
}

#endif
