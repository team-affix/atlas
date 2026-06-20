#ifndef UNIFIER_FACTORY_HPP
#define UNIFIER_FACTORY_HPP

#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier.hpp"

template<typename IBindMap>
struct unifier_factory {
    explicit unifier_factory(globalizer&);
    unifier<IBindMap> make(IBindMap* bm) const;
private:
    globalizer& globalizer_;
};

template<typename IBindMap>
unifier_factory<IBindMap>::unifier_factory(globalizer& g) : globalizer_(g) {}

template<typename IBindMap>
unifier<IBindMap> unifier_factory<IBindMap>::make(IBindMap* bm) const {
    return unifier<IBindMap>{globalizer_, bm};
}

#endif
