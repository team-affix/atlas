#ifndef PUD_PAGED_MODULE_HPP
#define PUD_PAGED_MODULE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_node_record.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

// A module whose stored tree lives in files, with pud_module's interface. A node's parent, child range and refuted flag
// are its record; its inference is encoded in the payload, decoded on demand through the cache. Every change is
// written to the files as it is made, so the files are always the whole module. Its header (id, dependencies, axioms)
// and functor names are held in memory, the names also appended to their log as they are learned.
template<
    typename IRecords,
    typename IPayload,
    typename INameLog,
    typename INodeCache,
    typename IInsertInference>
struct pud_paged_module {
    pud_paged_module(
        IRecords& records,
        IPayload& payload,
        INameLog& name_log,
        INodeCache& node_cache,
        IInsertInference& insert_inference,
        pud_module_id module_id,
        std::vector<pud_module_id> dependencies,
        std::vector<rule> axioms,
        std::vector<std::string> functor_names);
    pud_module_id get_module_id() const;
    const std::vector<pud_module_id>& get_dependencies() const;
    size_t count_axioms() const;
    size_t count_nodes() const;
    pud_node_id store_inference(pud_inference inference);
    void mark_refuted(pud_node_id node);
    const rule& get_axiom(pud_node_id axiom) const;
    const pud_inference& get_inference(pud_node_id inference) const;
    std::optional<pud_node_id> get_parent(pud_node_id node) const;
    const std::vector<pud_node_id>& get_children(pud_node_id node) const;
    bool is_leaf(pud_node_id node) const;
    bool is_refuted(pud_node_id node) const;
    const std::vector<std::string>& functor_names() const;
    uint32_t append_functor_name(const std::string& name);
    void trim();
private:
    IRecords& records_;
    IPayload& payload_;
    INameLog& name_log_;
    INodeCache& node_cache_;
    IInsertInference& insert_inference_;
    pud_module_id module_id_;
    std::vector<pud_module_id> dependencies_;
    std::vector<rule> axioms_;
    std::vector<std::string> functor_names_;
};

#define PUD_PAGED_MODULE_TEMPLATE                                                                        \
    template<typename IRecords, typename IPayload, typename INameLog, typename INodeCache,               \
             typename IInsertInference>
#define PUD_PAGED_MODULE pud_paged_module<IRecords, IPayload, INameLog, INodeCache, IInsertInference>

PUD_PAGED_MODULE_TEMPLATE
PUD_PAGED_MODULE::pud_paged_module(
    IRecords& records,
    IPayload& payload,
    INameLog& name_log,
    INodeCache& node_cache,
    IInsertInference& insert_inference,
    pud_module_id module_id,
    std::vector<pud_module_id> dependencies,
    std::vector<rule> axioms,
    std::vector<std::string> functor_names)
    : records_(records)
    , payload_(payload)
    , name_log_(name_log)
    , node_cache_(node_cache)
    , insert_inference_(insert_inference)
    , module_id_(module_id)
    , dependencies_(std::move(dependencies))
    , axioms_(std::move(axioms))
    , functor_names_(std::move(functor_names)) {}

PUD_PAGED_MODULE_TEMPLATE
pud_module_id PUD_PAGED_MODULE::get_module_id() const {
    return module_id_;
}

PUD_PAGED_MODULE_TEMPLATE
const std::vector<pud_module_id>& PUD_PAGED_MODULE::get_dependencies() const {
    return dependencies_;
}

PUD_PAGED_MODULE_TEMPLATE
size_t PUD_PAGED_MODULE::count_axioms() const {
    return axioms_.size();
}

PUD_PAGED_MODULE_TEMPLATE
size_t PUD_PAGED_MODULE::count_nodes() const {
    return records_.count();
}

// The new node's children come right after its siblings: a leaf's children are all stored by one unfold.
PUD_PAGED_MODULE_TEMPLATE
pud_node_id PUD_PAGED_MODULE::store_inference(pud_inference inference) {
    DEBUG_ASSERT(inference.parent < records_.count());
    const pud_node_id node = records_.count();
    std::ostringstream encoded;
    insert_inference_.insert_inference(encoded, inference);
    const std::string bytes = encoded.str();
    const uint64_t offset = payload_.append(bytes);
    pud_node_record parent = records_.get(inference.parent);
    DEBUG_ASSERT(parent.child_count == 0 || parent.first_child + parent.child_count == node);
    if (parent.child_count == 0)
        parent.first_child = node;
    ++parent.child_count;
    records_.append(pud_node_record{
        .payload_offset = offset,
        .payload_size = bytes.size(),
        .parent = inference.parent,
        .first_child = 0,
        .child_count = 0,
        .refuted = 0});
    records_.set(inference.parent, parent);
    node_cache_.forget_children(inference.parent);
    node_cache_.keep_inference(node, std::move(inference));
    return node;
}

PUD_PAGED_MODULE_TEMPLATE
void PUD_PAGED_MODULE::mark_refuted(pud_node_id node) {
    pud_node_record record = records_.get(node);
    record.refuted = 1;
    records_.set(node, record);
}

PUD_PAGED_MODULE_TEMPLATE
const rule& PUD_PAGED_MODULE::get_axiom(pud_node_id axiom) const {
    return axioms_.at(axiom);
}

PUD_PAGED_MODULE_TEMPLATE
const pud_inference& PUD_PAGED_MODULE::get_inference(pud_node_id inference) const {
    DEBUG_ASSERT(inference >= axioms_.size());
    return node_cache_.get_inference(inference);
}

PUD_PAGED_MODULE_TEMPLATE
std::optional<pud_node_id> PUD_PAGED_MODULE::get_parent(pud_node_id node) const {
    const uint64_t parent = records_.get(node).parent;
    if (parent == pud_node_record::k_no_parent)
        return std::nullopt;
    return parent;
}

PUD_PAGED_MODULE_TEMPLATE
const std::vector<pud_node_id>& PUD_PAGED_MODULE::get_children(pud_node_id node) const {
    return node_cache_.get_children(node);
}

PUD_PAGED_MODULE_TEMPLATE
bool PUD_PAGED_MODULE::is_leaf(pud_node_id node) const {
    return records_.get(node).child_count == 0;
}

PUD_PAGED_MODULE_TEMPLATE
bool PUD_PAGED_MODULE::is_refuted(pud_node_id node) const {
    return records_.get(node).refuted != 0;
}

PUD_PAGED_MODULE_TEMPLATE
const std::vector<std::string>& PUD_PAGED_MODULE::functor_names() const {
    return functor_names_;
}

PUD_PAGED_MODULE_TEMPLATE
uint32_t PUD_PAGED_MODULE::append_functor_name(const std::string& name) {
    name_log_.append(name);
    functor_names_.push_back(name);
    return static_cast<uint32_t>(functor_names_.size() - 1);
}

PUD_PAGED_MODULE_TEMPLATE
void PUD_PAGED_MODULE::trim() {
    node_cache_.trim();
}

#undef PUD_PAGED_MODULE
#undef PUD_PAGED_MODULE_TEMPLATE

#endif
