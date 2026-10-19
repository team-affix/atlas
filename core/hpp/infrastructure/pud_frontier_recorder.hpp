#ifndef PUD_FRONTIER_RECORDER_HPP
#define PUD_FRONTIER_RECORDER_HPP

#include <vector>
#include "value_objects/pud_node_id.hpp"

template<typename IMarkLeafOpen, typename IMarkLeafClosed, typename IMarkLeafRefuted>
struct pud_frontier_recorder {
    pud_frontier_recorder(
        IMarkLeafOpen& mark_leaf_open,
        IMarkLeafClosed& mark_leaf_closed,
        IMarkLeafRefuted& mark_leaf_refuted);
    void mark_leaf_open(pud_node_id leaf);
    void mark_leaf_closed(pud_node_id leaf);
    void mark_refuted(pud_node_id leaf);
    const std::vector<pud_node_id>& get_opened_leaves() const;
    const std::vector<pud_node_id>& get_refuted_leaves() const;
    void clear();
private:
    IMarkLeafOpen& mark_leaf_open_;
    IMarkLeafClosed& mark_leaf_closed_;
    IMarkLeafRefuted& mark_leaf_refuted_;
    std::vector<pud_node_id> opened_leaves_;
    std::vector<pud_node_id> refuted_leaves_;
};

template<typename IMLO, typename IMLC, typename IMLR>
pud_frontier_recorder<IMLO, IMLC, IMLR>::pud_frontier_recorder(
    IMLO& mark_leaf_open,
    IMLC& mark_leaf_closed,
    IMLR& mark_leaf_refuted)
    : mark_leaf_open_(mark_leaf_open)
    , mark_leaf_closed_(mark_leaf_closed)
    , mark_leaf_refuted_(mark_leaf_refuted)
    , opened_leaves_()
    , refuted_leaves_() {}

template<typename IMLO, typename IMLC, typename IMLR>
void pud_frontier_recorder<IMLO, IMLC, IMLR>::mark_leaf_open(pud_node_id leaf) {
    mark_leaf_open_.mark_leaf_open(leaf);
    opened_leaves_.push_back(leaf);
}

template<typename IMLO, typename IMLC, typename IMLR>
void pud_frontier_recorder<IMLO, IMLC, IMLR>::mark_leaf_closed(pud_node_id leaf) {
    mark_leaf_closed_.mark_leaf_closed(leaf);
}

template<typename IMLO, typename IMLC, typename IMLR>
void pud_frontier_recorder<IMLO, IMLC, IMLR>::mark_refuted(pud_node_id leaf) {
    mark_leaf_refuted_.mark_refuted(leaf);
    refuted_leaves_.push_back(leaf);
}

template<typename IMLO, typename IMLC, typename IMLR>
const std::vector<pud_node_id>& pud_frontier_recorder<IMLO, IMLC, IMLR>::get_opened_leaves() const {
    return opened_leaves_;
}

template<typename IMLO, typename IMLC, typename IMLR>
const std::vector<pud_node_id>& pud_frontier_recorder<IMLO, IMLC, IMLR>::get_refuted_leaves() const {
    return refuted_leaves_;
}

template<typename IMLO, typename IMLC, typename IMLR>
void pud_frontier_recorder<IMLO, IMLC, IMLR>::clear() {
    opened_leaves_.clear();
    refuted_leaves_.clear();
}

#endif
