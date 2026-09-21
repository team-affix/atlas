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
    , base_interval_()
    , unify_head_(om_, children_, added_unifications_, fpa_, fpa_,
                  globalizer_, exprs_, exprs_)
    , witness_search_(children_, children_, children_, unify_head_)
    , candidate_search_(witness_search_, children_, children_, children_,
                        unify_head_, added_body_goals_)
    , queries_(added_body_goals_, lvc_, base_interval_, om_, unify_head_,
               candidate_search_, witness_search_)
    , unfolder_(queries_, unify_head_, unify_head_, exprs_,
                lvc_, pool_,
                added_unifications_, added_body_goals_, lvc_,
                children_, base_interval_, om_,
                base_interval_, queries_)
    , axiom_adder_(pool_, added_unifications_, added_body_goals_, lvc_,
                   children_, om_, base_interval_, queries_) {}
