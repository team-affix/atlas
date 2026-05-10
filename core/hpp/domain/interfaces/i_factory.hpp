#ifndef I_FACTORY_HPP
#define I_FACTORY_HPP

#include <memory>

template<typename T, typename... Args>
struct i_factory {
    virtual ~i_factory() = default;
    virtual std::unique_ptr<T> make(Args...) = 0;
};

#endif
