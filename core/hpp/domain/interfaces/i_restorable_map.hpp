#ifndef I_RESTORABLE_MAP_HPP
#define I_RESTORABLE_MAP_HPP

template<typename K, typename V>
struct i_restorable_map {
    virtual ~i_restorable_map() = default;
    virtual void restore() = 0;
    virtual void insert(const K&, const V&) = 0;
    virtual void erase(const K&) = 0;
};

#endif
