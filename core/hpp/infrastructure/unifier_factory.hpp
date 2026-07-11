#ifndef UNIFIER_FACTORY_HPP
#define UNIFIER_FACTORY_HPP

#include "infrastructure/unifier.hpp"

template<typename IGlobalize, typename IBindMap>
struct unifier_factory {
    unifier_factory(IGlobalize&);
    unifier<IGlobalize, IBindMap> make(IBindMap* bm) const;
private:
    IGlobalize& globalizer_;
};

template<typename IGlobalize, typename IBindMap>
unifier_factory<IGlobalize, IBindMap>::unifier_factory(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize, typename IBindMap>
unifier<IGlobalize, IBindMap> unifier_factory<IGlobalize, IBindMap>::make(IBindMap* bm) const {
    return unifier<IGlobalize, IBindMap>{globalizer_, bm};
}

#endif
