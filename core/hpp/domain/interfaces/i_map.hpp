#ifndef I_MAP_HPP
#define I_MAP_HPP

template<typename K, typename V>
struct i_map {
    virtual ~i_map() = default;
    virtual void insert(K, V) = 0;
    virtual bool contains(K) const = 0;
    virtual V& at(K) = 0;
    virtual const V& at(K) const = 0;
    virtual void erase(K) = 0;
    virtual void clear() = 0;
};

#endif
