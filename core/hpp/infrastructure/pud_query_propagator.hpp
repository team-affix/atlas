#ifndef PUD_QUERY_PROPAGATOR_HPP
#define PUD_QUERY_PROPAGATOR_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <immer/map_transient.hpp>
#include "value_objects/pud_query_node.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<
    typename BindMap,
    typename Unifier,
    typename Specializer,
    typename Normalizer,
    typename IGetNodeParent,
    typename IMakeNode,
    typename IMakeVar,
    typename IGlobalize,
    typename ICheckNodeRefuted>
struct pud_query_propagator {
    struct query_node_handle {
        const pud_node* node() const;
    private:
        query_node_handle(std::shared_ptr<pud_query_node> node);
        std::shared_ptr<pud_query_node> query_node;
        friend struct pud_query_propagator;
    };
    pud_query_propagator(
        IGetNodeParent& get_node_parent,
        IMakeNode& make_node,
        IMakeVar& make_var,
        IGlobalize& globalize,
        ICheckNodeRefuted& check_node_refuted);
    query_node_handle root();
    std::optional<query_node_handle> propagate(query_node_handle current, const pud_node* child_node);
    query_node_handle open_query(query_node_handle caller, const expr* query);
    const pud_node* close_query(query_node_handle query);
private:
    IGetNodeParent& get_node_parent_;
    IMakeNode& make_node_;
    IMakeVar& make_var_;
    IGlobalize& globalize_;
    ICheckNodeRefuted& check_node_refuted_;
};

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::query_node_handle::query_node_handle(
    std::shared_ptr<pud_query_node> node)
    : query_node(std::move(node)) {}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
const pud_node* pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::query_node_handle::node() const {
    return query_node->node;
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::pud_query_propagator(
    IGNP& get_node_parent,
    IMN& make_node,
    IMV& make_var,
    IG& globalize,
    IGNR& check_node_refuted)
    : get_node_parent_(get_node_parent)
    , make_node_(make_node)
    , make_var_(make_var)
    , globalize_(globalize)
    , check_node_refuted_(check_node_refuted) {}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
typename pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::query_node_handle
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::root() {
    return query_node_handle{
        std::make_shared<pud_query_node>(
            pud_query_node{
                .frame_offset = 0,
                .node = nullptr,
                .touched_caller_reps = {},
                .parent = std::shared_ptr<pud_query_node>{},
                .bindings = {},
                .lvc = 1
            })};
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
std::optional<typename pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::query_node_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::propagate(query_node_handle current, const pud_node* child_node) {
    const pud_node* current_node = current.query_node->node;

    if (check_node_refuted_.check_refuted(child_node))
        return std::nullopt;

    DEBUG_ASSERT(get_node_parent_.get(child_node) == current_node);

    auto transient = current.query_node->bindings.transient();
    BM bind_map{globalize_, transient};
    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    uint32_t frame_offset = current.query_node->frame_offset;

    std::vector<uint32_t> touched_caller_reps;

    for (pud_specialization spec : child_node->added_specializations) {
        auto sm = specializer.specialize(frame_offset, spec);
        while (auto touched_rep = sm.next())
            touched_caller_reps.push_back(*touched_rep);
        if (!sm.result())
            return std::nullopt;
    }

    immer::map<uint32_t, framed_expr> child_bindings = std::move(transient).persistent();

    uint32_t lvc = current.query_node->lvc;

    auto new_query_node = std::make_shared<pud_query_node>(
        pud_query_node{
            .frame_offset = frame_offset,
            .node = child_node,
            .touched_caller_reps = touched_caller_reps,
            .parent = current.query_node,
            .bindings = std::move(child_bindings),
            .lvc = lvc + child_node->added_var_count
        }
    );

    return query_node_handle{new_query_node};
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
typename pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::query_node_handle
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::open_query(query_node_handle caller, const expr* query_expr) {
    uint32_t caller_frame_offset = caller.query_node->frame_offset;
    uint32_t caller_lvc = caller.query_node->lvc;

    uint32_t query_frame_offset = caller_frame_offset + caller_lvc;

    auto transient = caller.query_node->bindings.transient();
    BM bind_map{globalize_, transient};

    uint32_t query_head_var = globalize_.globalize(query_frame_offset, 0);

    bind_map.bind(
        query_head_var,
        framed_expr{
            query_expr,
            caller_frame_offset});

    immer::map<uint32_t, framed_expr> query_bindings = std::move(transient).persistent();

    auto new_query_node = std::make_shared<pud_query_node>(
        pud_query_node{
            .frame_offset = query_frame_offset,
            .node = nullptr,
            .touched_caller_reps = {},
            .parent = caller.query_node,
            .bindings = std::move(query_bindings),
            .lvc = 0
        });

    return query_node_handle{new_query_node};
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR>
const pud_node* pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR>::close_query(query_node_handle current) {
    uint32_t frame_offset = current.query_node->frame_offset;

    auto transient = current.query_node->bindings.transient();
    BM bind_map{globalize_, transient};
    N normalizer{globalize_, make_var_, make_var_, bind_map};

    std::unordered_map<uint32_t, uint32_t> translation_map;

    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;

    for (const pud_query_node* node = current.query_node.get(); node->node != nullptr; node = node->parent.get()) {
        for (uint32_t touched_caller_rep : node->touched_caller_reps) {
            auto var = make_var_.make_var(touched_caller_rep);
            framed_expr var_framed{var, 0};
            framed_expr value = bind_map.whnf(var_framed);
            auto normalized = normalizer.normalize(value, frame_offset, translation_map);
            added_specializations.push_back(pud_specialization{
                .var_idx = touched_caller_rep,
                .value = normalized
            });
        }

        for (const expr* body_goal : node->node->added_body_goals) {
            framed_expr body_goal_framed{body_goal, frame_offset};
            auto normalized = normalizer.normalize(body_goal_framed, frame_offset, translation_map);
            added_body_goals.push_back(normalized);
        }
    }

    uint32_t added_var_count = translation_map.size();

    return make_node_.make(
        added_specializations,
        added_body_goals,
        added_var_count
    );
}

#endif
