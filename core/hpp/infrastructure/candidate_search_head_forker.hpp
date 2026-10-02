#ifndef CANDIDATE_SEARCH_HEAD_FORKER_HPP
#define CANDIDATE_SEARCH_HEAD_FORKER_HPP

template<typename QueryHandle, typename Head>
struct candidate_search_head_forker {
    Head fork(const Head& other, QueryHandle new_query_handle) const;
};

template<typename QueryHandle, typename Head>
Head candidate_search_head_forker<QueryHandle, Head>::fork(const Head& other, QueryHandle new_query_handle) const {
    return Head{other, new_query_handle};
}

#endif
