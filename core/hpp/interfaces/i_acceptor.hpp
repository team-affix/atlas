#ifndef I_ACCEPTOR_HPP
#define I_ACCEPTOR_HPP

#include "i_visitor.hpp"

template<typename T>
struct i_acceptor {
    virtual ~i_acceptor() = default;
    virtual void accept(i_visitor<T>&) = 0;
};

#endif
