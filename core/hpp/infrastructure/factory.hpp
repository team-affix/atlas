#ifndef FACTORY_HPP
#define FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"

template<typename T, typename Impl>
struct factory : i_factory<T> {
    std::unique_ptr<T> make() override {
        return std::make_unique<Impl>();
    }
};

#endif
