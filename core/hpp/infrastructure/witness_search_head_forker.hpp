#ifndef WITNESS_SEARCH_HEAD_FORKER_HPP
#define WITNESS_SEARCH_HEAD_FORKER_HPP

template<typename QueryHandle, typename Head>
struct witness_search_head_forker {
    Head fork(const Head& other, QueryHandle new_query_handle) const;
};

template<typename QueryHandle, typename Head>
Head witness_search_head_forker<QueryHandle, Head>::fork(const Head& other, QueryHandle new_query_handle) const {
    return Head{other, new_query_handle};
}

#endif
