#include "infrastructure/pud_manifest.hpp"

pud_manifest::pud_manifest()
    : pool_()
    , om_()
    , fpa_()
    , globalizer_()
    , exprs_()
    , forest_(pool_, pool_, om_, om_)
    , leaf_queries_()
    , watchers_()
    , unify_head_(om_, forest_, forest_, fpa_, fpa_, globalizer_, exprs_, exprs_)
    , witness_search_(forest_, forest_, forest_, unify_head_)
    , candidate_search_(witness_search_, forest_, forest_, forest_, unify_head_)
    , reinit_(forest_, forest_, fpa_, unify_head_, unify_head_)
    , router_(watchers_, witness_search_, candidate_search_, unify_head_)
    , unfolder_(forest_, unify_head_, unify_head_, unify_head_, exprs_,
                forest_, forest_, om_,
                leaf_queries_, leaf_queries_, leaf_queries_,
                reinit_, candidate_search_,
                forest_, forest_, forest_,
                router_, watchers_, watchers_) {}
