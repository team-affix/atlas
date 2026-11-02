#ifndef PUD_WITNESS_SEARCH_RECORDER_HPP
#define PUD_WITNESS_SEARCH_RECORDER_HPP

#include <optional>
#include <utility>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_advance.hpp"

// Forwards the witness search capabilities mhcs uses, recording which searches an add or fork created
// so the candidate watch can tell what a candidate rests on. A search's witness leaf is the one the
// witness search itself rests on.
template<typename IMhws>
struct pud_witness_search_recorder {
    pud_witness_search_recorder(IMhws& inner);
    std::optional<pud_mhws_head_id> try_add(pud_replay search_root);
    std::optional<pud_mhws_head_id> try_fork(pud_mhws_head_id head_id, pud_replay search_root);
    std::optional<pud_witness_advance> advance(pud_mhws_head_id head_id);
    void remove(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> invalidate_leaf(pud_global_node_id leaf);
    void begin_recording_adds();
    std::vector<pud_mhws_head_id> take_recorded_adds();
    std::optional<pud_global_node_id> node_of(pud_mhws_head_id head_id) const;
private:
    void record(std::optional<pud_mhws_head_id> head_id);

    IMhws& inner_;
    std::vector<pud_mhws_head_id> recorded_adds_;
    bool recording_;
};

template<typename IMhws>
pud_witness_search_recorder<IMhws>::pud_witness_search_recorder(IMhws& inner)
    : inner_(inner)
    , recorded_adds_()
    , recording_(false) {}

template<typename IMhws>
void pud_witness_search_recorder<IMhws>::record(std::optional<pud_mhws_head_id> head_id) {
    if (!head_id.has_value() || !recording_)
        return;
    recorded_adds_.push_back(head_id.value());
}

template<typename IMhws>
std::optional<pud_mhws_head_id> pud_witness_search_recorder<IMhws>::try_add(pud_replay search_root) {
    const std::optional<pud_mhws_head_id> head_id = inner_.try_add(std::move(search_root));
    record(head_id);
    return head_id;
}

template<typename IMhws>
std::optional<pud_mhws_head_id> pud_witness_search_recorder<IMhws>::try_fork(
    pud_mhws_head_id head_id, pud_replay search_root) {
    const std::optional<pud_mhws_head_id> forked = inner_.try_fork(head_id, std::move(search_root));
    record(forked);
    return forked;
}

template<typename IMhws>
std::optional<pud_witness_advance> pud_witness_search_recorder<IMhws>::advance(pud_mhws_head_id head_id) {
    return inner_.advance(head_id);
}

template<typename IMhws>
void pud_witness_search_recorder<IMhws>::remove(pud_mhws_head_id head_id) {
    inner_.remove(head_id);
}

template<typename IMhws>
std::vector<pud_mhws_head_id> pud_witness_search_recorder<IMhws>::invalidate_leaf(pud_global_node_id leaf) {
    return inner_.invalidate_leaf(leaf);
}

template<typename IMhws>
void pud_witness_search_recorder<IMhws>::begin_recording_adds() {
    recording_ = true;
    recorded_adds_.clear();
}

template<typename IMhws>
std::vector<pud_mhws_head_id> pud_witness_search_recorder<IMhws>::take_recorded_adds() {
    recording_ = false;
    return std::move(recorded_adds_);
}

template<typename IMhws>
std::optional<pud_global_node_id> pud_witness_search_recorder<IMhws>::node_of(pud_mhws_head_id head_id) const {
    return inner_.find_witness(head_id);
}

#endif
