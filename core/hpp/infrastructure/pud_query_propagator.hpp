#ifndef PUD_QUERY_PROPAGATOR_HPP
#define PUD_QUERY_PROPAGATOR_HPP

#include <memory>
#include <optional>
#include "value_objects/pud_query_node.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/framed_expr.hpp"

template<
    typename BindMap,
    typename Unifier,
    typename Specializer,
    typename Normalizer,
    typename IGetNodeChildren,
    typename IAllocateRootInterval,
    typename IAllocateChildInterval,
    typename IMakeVar,
    typename IGlobalize,
    typename IRecordFPArrayBinding,
    typename IQueryFPArrayBinding>
struct pud_query_propagator {
    struct query_node_handle {
    private:
        std::shared_ptr<pud_query_node> query_node;
        friend struct pud_query_propagator;
    };
    pud_query_propagator(
        IGetNodeChildren& get_node_children,
        IAllocateRootInterval& allocate_root_interval,
        IAllocateChildInterval& allocate_child_interval,
        IMakeVar& make_var,
        IGlobalize& globalize,
        IRecordFPArrayBinding& record_fp,
        IQueryFPArrayBinding& query_fp);
    query_node_handle root();
    std::optional<query_node_handle> child(query_node_handle current, const pud_rule_id* child_callee);
    query_node_handle open_query(query_node_handle caller, const expr* query);
    pud_node close_query(query_node_handle query);
private:
    IGetNodeChildren& get_node_children_;
    IAllocateRootInterval& allocate_root_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IMakeVar& make_var_;
    IGlobalize& globalize_;
    IRecordFPArrayBinding& record_fp_;
    IQueryFPArrayBinding& query_fp_;
};

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IGNC,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
pud_query_propagator<BM, U, S, N, IGNC, IAR, IAC, IMV, IG, IRFAB, IQFAB>::pud_query_propagator(
    IGNC& get_node_children,
    IAR& allocate_root_interval,
    IAC& allocate_child_interval,
    IMV& make_var,
    IG& globalize,
    IRFAB& record_fp,
    IQFAB& query_fp) :
    get_node_children_(get_node_children),
    allocate_root_interval_(allocate_root_interval),
    allocate_child_interval_(allocate_child_interval),
    make_var_(make_var),
    globalize_(globalize),
    record_fp_(record_fp),
    query_fp_(query_fp)
    {}

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IGNC,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
pud_query_propagator<BM, U, S, N, IGNC, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle pud_query_propagator<BM, U, S, N, IGNC, IAR, IAC, IMV, IG, IRFAB, IQFAB>::root() {
    return query_node_handle{
        std::make_shared<pud_query_node>(
            pud_query_node{
                .frame_offset = 0,
                .node = nullptr,
                .touched_caller_reps = {},
                .parent = std::shared_ptr<pud_query_node>{},
                .interval = allocate_root_interval_.allocate_root(),
                .lvc = 0
            })};
}

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IGNC,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
std::optional<typename pud_query_propagator<BM, U, S, N, IGNC, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle> pud_query_propagator<BM, U, S, N, IGNC, IAR, IAC, IMV, IG, IRFAB, IQFAB>::child(query_node_handle current, const pud_rule_id* child_callee) {
    const pud_node* current_node = current.query_node->node;
    const auto& children = get_node_children_.get(current_node);
    const pud_node* child_node = children.at(child_callee);

    om_interval current_interval = current.query_node->interval;
    om_interval child_interval = allocate_child_interval_.allocate_child_of(current_interval);

    BM bind_map{globalize_, record_fp_, query_fp_, child_interval};
    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    uint32_t frame_offset = current.query_node->frame_offset;
    
    std::vector<uint32_t> touched_caller_reps;

    // try to traverse to child

    for (pud_specialization spec : child_node->added_specializations) {
        auto sm = specializer.specialize(frame_offset, spec);
        while (auto touched_rep = sm.next())
            touched_caller_reps.push_back(*touched_rep);
        if (!sm.result())
            return std::nullopt; // can't traverse to child
    }

    // at this point, traversal has succeeded

    uint32_t lvc = current.query_node->lvc;
    
    auto new_query_node = std::make_shared<pud_query_node>(
        pud_query_node{
            .frame_offset = frame_offset,
            .node = child_node,
            .touched_caller_reps = touched_caller_reps,
            .parent = current.query_node,
            .interval = child_interval,
            .lvc = lvc + child_node->added_var_count
        }
    );

    return query_node_handle{new_query_node};
}

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
typename pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::open_query(typename pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle caller, const expr* query_expr) {
    uint32_t caller_frame_offset = caller.query_node->frame_offset;
    uint32_t caller_lvc = caller.query_node->lvc;
    om_interval caller_interval = caller.query_node->interval;

    om_interval query_interval = allocate_child_interval_.allocate_child_of(caller_interval);

    uint32_t query_frame_offset = caller_frame_offset + caller_lvc;
    
    BM bind_map{globalize_, record_fp_, query_fp_, query_interval};

    // get the root head var (var0@cutoff)

    uint32_t query_head_var = globalize_.globalize(query_frame_offset, 0);

    // seed the bind map with the query expression
    bind_map.bind(
        query_head_var,
        framed_expr{
            query_expr,
            caller_frame_offset});

    auto new_query_node = std::make_shared<pud_query_node>(
        pud_query_node{
            .frame_offset = query_frame_offset,
            .node = nullptr,
            .touched_caller_reps = {},
            .parent = caller.query_node,
            .interval = query_interval,
            .lvc = 0
        });
    
    return query_node_handle{new_query_node};
}

template<
    typename BM,
    typename U,
    typename S,
    typename N,
    typename IAR,
    typename IAC,
    typename IMV,
    typename IG,
    typename IRFAB,
    typename IQFAB>
pud_node pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::close_query(typename pud_query_propagator<BM, U, S, N, IAR, IAC, IMV, IG, IRFAB, IQFAB>::query_node_handle current) {
    // 1. extract relevant fields
    uint32_t frame_offset = current.query_node->frame_offset;
    om_interval interval = current.query_node->interval;
    
    // 1.5. create bind map, and normalizer
    BM bind_map{globalize_, record_fp_, query_fp_, interval};
    N normalizer{make_var_, bind_map};

    // 1.75. initialize translation map
    std::unordered_map<uint32_t, uint32_t> translation_map;
    
    // 2. aggregate all added specializations, body goals from lineage
    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;

    // this iterates points in the lineage of the query
    for (const pud_query_node* node = current.query_node.get(); node != nullptr; node = node->parent.get()) {
        for (uint32_t touched_caller_rep : node->touched_caller_reps) {
            //     2a. for each touched caller rep, normalize it with cutoff == frame_offset of query.
            //         keep rolling map for translating additional vars to contiguous indices
            auto var = make_var_.make_var(touched_caller_rep);
            framed_expr var_framed{var, 0}; // always frame 0
                                            // since these were sourced from unify() yields
                                            // which are already globalized w.r.t.
                                            // the binding environment
            auto normalized = normalizer.normalize(var_framed, frame_offset, translation_map);
            added_specializations.push_back(pud_specialization{
                .var_idx = touched_caller_rep,
                .value = normalized
            });
        }
        for (const expr* body_goal : node->node->added_body_goals) {
            //     2b. for each added body goal, normalize it with same instructions as before (same map)
            framed_expr body_goal_framed{body_goal, frame_offset};
            auto normalized = normalizer.normalize(body_goal_framed, frame_offset, translation_map);
            added_body_goals.push_back(normalized);
        }
    }
    // 3. added_var_count is just the size of the translation map (one entry per new var)
    uint32_t added_var_count = translation_map.size();

    // 4. create new node
    return pud_node{
        .added_specializations = added_specializations,
        .added_body_goals = added_body_goals,
        .added_var_count = added_var_count,
        .children = {}
    };
}

#endif
