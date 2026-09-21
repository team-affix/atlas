#include "infrastructure/pud_manifest.hpp"

pud_manifest::pud_manifest()
    : pool_()
    , om_()
    , fpa_()
    , globalizer_()
    , exprs_()
    , forest_(pool_, pool_, om_, om_)
    , unify_head_(om_, forest_, forest_, fpa_, fpa_, globalizer_, exprs_, exprs_)
    , witness_search_(forest_, forest_, forest_, unify_head_)
    , candidate_search_(witness_search_, forest_, forest_, forest_, unify_head_, forest_)
    , queries_(forest_, forest_, om_, unify_head_, unify_head_,
               candidate_search_, witness_search_)
    , unfolder_(forest_, unify_head_, unify_head_, exprs_,
                forest_, forest_,
                queries_, queries_, queries_, queries_)
    , axiom_adder_(forest_, queries_) {}
