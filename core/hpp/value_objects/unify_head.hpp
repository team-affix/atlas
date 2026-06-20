#ifndef UNIFY_HEAD_HPP
#define UNIFY_HEAD_HPP

#include <memory>

template<typename IBindMap, typename IUnifier>
struct unify_head {
    std::unique_ptr<IBindMap> local_bind_map;
    IUnifier unifier;
};

#endif
