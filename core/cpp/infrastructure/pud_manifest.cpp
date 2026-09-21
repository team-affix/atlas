#include "infrastructure/pud_manifest.hpp"

pud_manifest::pud_manifest()
    : pool_()
    , om_()
    , fpa_()
    , globalizer_()
    , exprs_()
    , added_unifications_()
    , added_body_goals_()
    , lvc_()
    , children_()
    , parent_()
    , node_interval_()
    , added_caller_reps_()
    , pud_normalizer_(globalizer_, fpa_, fpa_, exprs_, exprs_)
    , witness_search_(children_, parent_, pool_,
                      node_interval_, node_interval_, node_interval_,
                      om_, added_unifications_,
                      fpa_, fpa_,
                      globalizer_, exprs_, added_caller_reps_)
    , candidate_search_(witness_search_, children_, parent_,
                        added_body_goals_)
    , queries_(added_body_goals_, lvc_,
               candidate_search_, witness_search_)
    , unfolder_(queries_, pud_normalizer_, pud_normalizer_, pud_normalizer_,
                exprs_, lvc_, pool_,
                added_unifications_, added_unifications_, added_body_goals_, lvc_,
                children_, parent_, parent_, node_interval_, om_,
                node_interval_, fpa_, added_caller_reps_, queries_)
    , axiom_adder_(pool_, added_unifications_, added_body_goals_, lvc_,
                   parent_, om_, node_interval_, queries_) {}
