#ifndef I_RESTORABLE_MAP_HPP
#define I_RESTORABLE_MAP_HPP

template<typename M>
struct i_restorable_map {
    virtual ~i_restorable_map() = default;
    virtual void restore() = 0;
    virtual void insert(const M::key_type&, const M::value_type&) = 0;
    virtual void erase(const M::key_type&) = 0;
    virtual void assign(const M::key_type&, const M::value_type&) = 0;
    virtual const M& get() const = 0;
};

#endif
