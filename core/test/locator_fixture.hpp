#ifndef LOCATOR_FIXTURE_HPP
#define LOCATOR_FIXTURE_HPP

#include "infrastructure/locator.hpp"

template<typename Sut, typename... Ifaces, typename Dep>
Sut bind_and_make(locator& loc, Dep& dep) {
    loc.bind_as<Ifaces...>(dep);
    return Sut{loc};
}

#endif
