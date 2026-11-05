#ifndef PUD_DECODED_NODE_CACHE_HPP
#define PUD_DECODED_NODE_CACHE_HPP

#include <cstddef>
#include <list>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_node_record.hpp"

// The decoded parts of one module's stored nodes, kept while they fit: inferences decoded from their payload bytes,
// and children lists spelled out from their records' ranges. Entries are dropped, least recently used first, only when
// trimmed: an inference or children list read in a search step stays valid until the step ends.
template<typename IGetRecord, typename IReadPayload, typename IExtractInference>
struct pud_decoded_node_cache {
    pud_decoded_node_cache(
        IGetRecord& get_record,
        IReadPayload& read_payload,
        IExtractInference& extract_inference,
        size_t budget_bytes);
    const pud_inference& get_inference(pud_node_id node);
    const std::vector<pud_node_id>& get_children(pud_node_id node);
    void keep_inference(pud_node_id node, pud_inference inference);
    void forget_children(pud_node_id node);
    void trim();
    size_t hits() const;
    size_t misses() const;
private:
    using recency_t = std::list<pud_node_id>;
    struct entry {
        std::optional<pud_inference> inference;
        std::optional<std::vector<pud_node_id>> children;
        size_t bytes;
        recency_t::iterator recency;
    };

    entry& find_entry(pud_node_id node);
    void count_bytes(entry& counted);
    void evict(pud_node_id node);

    IGetRecord& get_record_;
    IReadPayload& read_payload_;
    IExtractInference& extract_inference_;
    size_t budget_bytes_;
    size_t kept_bytes_;
    size_t hits_;
    size_t misses_;
    recency_t recency_;
    std::unordered_map<pud_node_id, entry> entries_;
};

#define PUD_DECODED_NODE_CACHE_TEMPLATE template<typename IGetRecord, typename IReadPayload, typename IExtractInference>
#define PUD_DECODED_NODE_CACHE          pud_decoded_node_cache<IGetRecord, IReadPayload, IExtractInference>

PUD_DECODED_NODE_CACHE_TEMPLATE
PUD_DECODED_NODE_CACHE::pud_decoded_node_cache(
    IGetRecord& get_record,
    IReadPayload& read_payload,
    IExtractInference& extract_inference,
    size_t budget_bytes)
    : get_record_(get_record)
    , read_payload_(read_payload)
    , extract_inference_(extract_inference)
    , budget_bytes_(budget_bytes)
    , kept_bytes_(0)
    , hits_(0)
    , misses_(0)
    , recency_()
    , entries_() {}

PUD_DECODED_NODE_CACHE_TEMPLATE
const pud_inference& PUD_DECODED_NODE_CACHE::get_inference(pud_node_id node) {
    entry& found = find_entry(node);
    if (found.inference.has_value()) {
        ++hits_;
        return found.inference.value();
    }
    ++misses_;
    const pud_node_record& record = get_record_.get(node);
    std::istringstream in(read_payload_.read(record.payload_offset, record.payload_size));
    found.inference = extract_inference_.extract_inference(in);
    count_bytes(found);
    return found.inference.value();
}

PUD_DECODED_NODE_CACHE_TEMPLATE
const std::vector<pud_node_id>& PUD_DECODED_NODE_CACHE::get_children(pud_node_id node) {
    entry& found = find_entry(node);
    if (found.children.has_value())
        return found.children.value();
    const pud_node_record& record = get_record_.get(node);
    std::vector<pud_node_id> children;
    children.reserve(record.child_count);
    for (uint64_t offset = 0; offset < record.child_count; ++offset)
        children.push_back(record.first_child + offset);
    found.children = std::move(children);
    count_bytes(found);
    return found.children.value();
}

PUD_DECODED_NODE_CACHE_TEMPLATE
void PUD_DECODED_NODE_CACHE::keep_inference(pud_node_id node, pud_inference inference) {
    entry& kept = find_entry(node);
    kept.inference = std::move(inference);
    count_bytes(kept);
}

// A node's children list is spelled out again the next time it is read: its range has grown.
PUD_DECODED_NODE_CACHE_TEMPLATE
void PUD_DECODED_NODE_CACHE::forget_children(pud_node_id node) {
    const auto found = entries_.find(node);
    if (found == entries_.end())
        return;
    found->second.children = std::nullopt;
    count_bytes(found->second);
}

PUD_DECODED_NODE_CACHE_TEMPLATE
void PUD_DECODED_NODE_CACHE::trim() {
    while (kept_bytes_ > budget_bytes_ && !recency_.empty())
        evict(recency_.back());
}

PUD_DECODED_NODE_CACHE_TEMPLATE
size_t PUD_DECODED_NODE_CACHE::hits() const {
    return hits_;
}

PUD_DECODED_NODE_CACHE_TEMPLATE
size_t PUD_DECODED_NODE_CACHE::misses() const {
    return misses_;
}

// The node's entry, made empty if it has none, and marked most recently used.
PUD_DECODED_NODE_CACHE_TEMPLATE
typename PUD_DECODED_NODE_CACHE::entry& PUD_DECODED_NODE_CACHE::find_entry(pud_node_id node) {
    const auto found = entries_.find(node);
    if (found != entries_.end()) {
        recency_.splice(recency_.begin(), recency_, found->second.recency);
        return found->second;
    }
    recency_.push_front(node);
    entry& made = entries_.emplace(
        node, entry{.inference = std::nullopt, .children = std::nullopt, .bytes = 0, .recency = recency_.begin()})
        .first->second;
    count_bytes(made);
    return made;
}

// An entry's size is estimated from what it holds: an inference costs its vectors and their terms' share.
PUD_DECODED_NODE_CACHE_TEMPLATE
void PUD_DECODED_NODE_CACHE::count_bytes(entry& counted) {
    size_t bytes = sizeof(entry) + 2 * sizeof(pud_node_id);
    if (counted.inference.has_value())
        bytes += 48 * counted.inference->substitutions.size() + 32 * counted.inference->body_goals.size();
    if (counted.children.has_value())
        bytes += sizeof(pud_node_id) * counted.children->size();
    kept_bytes_ = kept_bytes_ - counted.bytes + bytes;
    counted.bytes = bytes;
}

PUD_DECODED_NODE_CACHE_TEMPLATE
void PUD_DECODED_NODE_CACHE::evict(pud_node_id node) {
    const auto found = entries_.find(node);
    kept_bytes_ -= found->second.bytes;
    recency_.erase(found->second.recency);
    entries_.erase(found);
}

#undef PUD_DECODED_NODE_CACHE
#undef PUD_DECODED_NODE_CACHE_TEMPLATE

#endif
