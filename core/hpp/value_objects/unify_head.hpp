#ifndef UNIFY_HEAD_HPP
#define UNIFY_HEAD_HPP

#include <memory>

template<typename IBindMap, typename IUnifier>
struct unify_head {
    unify_head(std::unique_ptr<IBindMap> local_bind_map, IUnifier unifier);

    IBindMap& local_bind_map();
    const IBindMap& local_bind_map() const;
    IBindMap* local_bind_map_ptr();
    IUnifier& unifier();
    const IUnifier& unifier() const;

private:
    std::unique_ptr<IBindMap> local_bind_map_;
    IUnifier unifier_;
};

template<typename IBindMap, typename IUnifier>
unify_head<IBindMap, IUnifier>::unify_head(std::unique_ptr<IBindMap> local_bind_map, IUnifier unifier)
    : local_bind_map_(std::move(local_bind_map)), unifier_(std::move(unifier)) {}

template<typename IBindMap, typename IUnifier>
IBindMap& unify_head<IBindMap, IUnifier>::local_bind_map() { return *local_bind_map_; }

template<typename IBindMap, typename IUnifier>
const IBindMap& unify_head<IBindMap, IUnifier>::local_bind_map() const { return *local_bind_map_; }

template<typename IBindMap, typename IUnifier>
IBindMap* unify_head<IBindMap, IUnifier>::local_bind_map_ptr() { return local_bind_map_.get(); }

template<typename IBindMap, typename IUnifier>
IUnifier& unify_head<IBindMap, IUnifier>::unifier() { return unifier_; }

template<typename IBindMap, typename IUnifier>
const IUnifier& unify_head<IBindMap, IUnifier>::unifier() const { return unifier_; }

#endif
