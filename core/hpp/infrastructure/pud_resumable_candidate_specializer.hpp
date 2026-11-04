#ifndef PUD_RESUMABLE_CANDIDATE_SPECIALIZER_HPP
#define PUD_RESUMABLE_CANDIDATE_SPECIALIZER_HPP

#include <optional>
#include <utility>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_specialization_state.hpp"
#include "value_objects/pud_witness_advance.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

// The MHCS candidate specialization without its stored candidates, on a state the caller owns. A candidate stands
// on a node of its axiom's stored tree and needs witnesses in two different child subtrees to stay there. While
// only one child subtree has a witness, it moves down into it; a leaf is its own witness. Specializing a state
// continues from it, filling the witnesses it lacks: a new state starts at the axiom root with none.
template<
    typename IDescendReplay,
    typename IResumeWitnessSearch,
    typename IAdvanceWitnessSearch,
    typename IIsNodeLeaf,
    typename IIsNodeRefuted,
    typename IGetChildren>
struct pud_resumable_candidate_specializer {
    pud_resumable_candidate_specializer(
        IDescendReplay& descend_replay,
        IResumeWitnessSearch& resume_witness_search,
        IAdvanceWitnessSearch& advance_witness_search,
        IIsNodeLeaf& is_node_leaf,
        IIsNodeRefuted& is_node_refuted,
        IGetChildren& get_children);
    bool specialize(pud_specialization_state& state) const;
private:
    std::optional<pud_specialization_witness> try_add_witness(pud_specialization_state& state) const;
    void advance(pud_specialization_state& state, pud_specialization_witness& survivor) const;

    IDescendReplay& descend_replay_;
    IResumeWitnessSearch& resume_witness_search_;
    IAdvanceWitnessSearch& advance_witness_search_;
    IIsNodeLeaf& is_node_leaf_;
    IIsNodeRefuted& is_node_refuted_;
    IGetChildren& get_children_;
};

#define PUD_RESUMABLE_CANDIDATE_SPECIALIZER_TEMPLATE                                                    \
    template<typename IDescendReplay, typename IResumeWitnessSearch, typename IAdvanceWitnessSearch,      \
             typename IIsNodeLeaf, typename IIsNodeRefuted, typename IGetChildren>
#define PUD_RESUMABLE_CANDIDATE_SPECIALIZER                                                             \
    pud_resumable_candidate_specializer<IDescendReplay, IResumeWitnessSearch, IAdvanceWitnessSearch,      \
        IIsNodeLeaf, IIsNodeRefuted, IGetChildren>

PUD_RESUMABLE_CANDIDATE_SPECIALIZER_TEMPLATE
PUD_RESUMABLE_CANDIDATE_SPECIALIZER::pud_resumable_candidate_specializer(
    IDescendReplay& descend_replay,
    IResumeWitnessSearch& resume_witness_search,
    IAdvanceWitnessSearch& advance_witness_search,
    IIsNodeLeaf& is_node_leaf,
    IIsNodeRefuted& is_node_refuted,
    IGetChildren& get_children)
    : descend_replay_(descend_replay)
    , resume_witness_search_(resume_witness_search)
    , advance_witness_search_(advance_witness_search)
    , is_node_leaf_(is_node_leaf)
    , is_node_refuted_(is_node_refuted)
    , get_children_(get_children) {}

PUD_RESUMABLE_CANDIDATE_SPECIALIZER_TEMPLATE
bool PUD_RESUMABLE_CANDIDATE_SPECIALIZER::specialize(pud_specialization_state& state) const {
    while (true) {
        const pud_global_node_id node = state.current.node;
        if (is_node_refuted_.is_refuted(node)) {
            state.witnesses = std::nullopt;
            return false;
        }
        if (is_node_leaf_.is_leaf(node)) {
            state.witnesses = std::nullopt;
            return true;
        }
        if (!state.witnesses.has_value())
            state.witnesses = pud_specialization_witnesses{.a = std::nullopt, .b = std::nullopt, .next_child = 0};
        if (!state.witnesses->a.has_value())
            state.witnesses->a = try_add_witness(state);
        if (!state.witnesses->b.has_value())
            state.witnesses->b = try_add_witness(state);
        const bool has_a = state.witnesses->a.has_value();
        const bool has_b = state.witnesses->b.has_value();
        if (has_a && has_b)
            return true;
        if (!has_a && !has_b)
            return false;
        advance(state, has_a ? state.witnesses->a.value() : state.witnesses->b.value());
    }
}

PUD_RESUMABLE_CANDIDATE_SPECIALIZER_TEMPLATE
std::optional<pud_specialization_witness> PUD_RESUMABLE_CANDIDATE_SPECIALIZER::try_add_witness(
    pud_specialization_state& state) const {
    const std::vector<pud_node_id>& children = get_children_.get_children(state.current.node);
    size_t& next_child = state.witnesses->next_child;
    while (next_child < children.size()) {
        const pud_global_node_id child{.module = state.current.node.module, .node = children.at(next_child++)};
        std::optional<pud_replay> search_root = descend_replay_.descend(state.current, child);
        if (!search_root.has_value())
            continue;
        pud_witness_search_stack search{.frames = {}};
        const std::optional<pud_global_node_id> leaf = resume_witness_search_.resume(search_root.value(), search);
        if (!leaf.has_value())
            continue;
        return pud_specialization_witness{
            .search_root = std::move(search_root.value()), .search = std::move(search), .leaf = leaf.value()};
    }
    return std::nullopt;
}

// The candidate moves down to the root of the one witness it kept, which then searches from one level lower.
PUD_RESUMABLE_CANDIDATE_SPECIALIZER_TEMPLATE
void PUD_RESUMABLE_CANDIDATE_SPECIALIZER::advance(
    pud_specialization_state& state, pud_specialization_witness& survivor) const {
    std::optional<pud_witness_advance> advanced = advance_witness_search_.advance(survivor.search);
    state.path.push_back(survivor.search_root.node);
    state.current = survivor.search_root;
    if (!advanced.has_value()) {
        state.witnesses = std::nullopt;
        return;
    }
    survivor.search_root = std::move(advanced->search_root);
    state.witnesses->next_child = advanced->next_sibling;
}

#undef PUD_RESUMABLE_CANDIDATE_SPECIALIZER
#undef PUD_RESUMABLE_CANDIDATE_SPECIALIZER_TEMPLATE

#endif
