#ifndef I_VISITOR_HPP
#define I_VISITOR_HPP

template<typename T>
struct i_visitor {
    virtual ~i_visitor() = default;
    virtual void visit(T) = 0;
};

#endif
