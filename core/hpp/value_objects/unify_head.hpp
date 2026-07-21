#ifndef UNIFY_HEAD_HPP
#define UNIFY_HEAD_HPP

template<typename IBindMap, typename IUnifier>
struct unify_head {
    IBindMap* local_bind_map;
    IUnifier unifier;
};

#endif
