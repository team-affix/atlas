#ifndef I_RESTORABLE_SET_HPP
#define I_RESTORABLE_SET_HPP

template<typename S>
struct i_restorable_set {
    virtual ~i_restorable_set() = default;
    virtual void restore() = 0;
    virtual void insert(const S::value_type&) = 0;
    virtual void erase(const S::value_type&) = 0;
    virtual const S& get() const = 0;
};

#endif
