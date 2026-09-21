#ifndef PUD_QUERY_STARTER_HPP
#define PUD_QUERY_STARTER_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"

template<typename IMakeInference,
         typename IGetInterval,
         typename IAllocateChildInterval,
         typename IStoreInterval,
         typename IGlobalize,
         typename IRecordBinding,
         typename IQueryBinding>
struct pud_query_starter {
    pud_query_starter(IMakeInference& make_inference,
                      IGetInterval& get_interval,
                      IAllocateChildInterval& allocate_child_interval,
                      IStoreInterval& store_interval,
                      IGlobalize& globalize,
                      IRecordBinding& record_binding,
                      IQueryBinding& query_binding);
    void start(const pud_rule_id* leaf,
               size_t body_goal_idx,
               const expr* body_goal,
               uint32_t frame_offset);
private:
    using bind_map_t = hierarchical_bind_map<IGlobalize, IRecordBinding, IQueryBinding>;

    IMakeInference& make_inference_;
    IGetInterval& get_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IStoreInterval& store_interval_;
    IGlobalize& globalize_;
    IRecordBinding& record_binding_;
    IQueryBinding& query_binding_;
};

template<typename IMI, typename IGI, typename IACI, typename ISI,
         typename IG, typename IRB, typename IQB>
pud_query_starter<IMI, IGI, IACI, ISI, IG, IRB, IQB>::pud_query_starter(
        IMI& make_inference,
        IGI& get_interval,
        IACI& allocate_child_interval,
        ISI& store_interval,
        IG& globalize,
        IRB& record_binding,
        IQB& query_binding)
    : make_inference_(make_inference)
    , get_interval_(get_interval)
    , allocate_child_interval_(allocate_child_interval)
    , store_interval_(store_interval)
    , globalize_(globalize)
    , record_binding_(record_binding)
    , query_binding_(query_binding) {}

template<typename IMI, typename IGI, typename IACI, typename ISI,
         typename IG, typename IRB, typename IQB>
void pud_query_starter<IMI, IGI, IACI, ISI, IG, IRB, IQB>::start(
        const pud_rule_id* leaf,
        size_t body_goal_idx,
        const expr* body_goal,
        uint32_t frame_offset) {
    const pud_rule_id* key = make_inference_.make_inference(
        leaf, body_goal_idx, nullptr);
    const om_interval interval = allocate_child_interval_.allocate_child_of(
        get_interval_.get(leaf));
    store_interval_.store(key, interval);
    bind_map_t bm(globalize_, record_binding_, query_binding_, interval);
    bm.bind(globalize_.globalize(frame_offset, 0), framed_expr{body_goal, 0});
}

#endif
