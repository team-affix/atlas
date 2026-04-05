#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <unordered_map>

template<typename T>
struct registry {
    void insert(size_t id, const T* value) {
        deref_map.insert({id, value});
        ref_map.insert({value, id});
    }
    void erase(size_t id) {
        ref_map.erase(deref_map.at(id));
        deref_map.erase(id);
    }
    const T* deref(size_t id) const {
        return deref_map.at(id);
    }
    const size_t ref(const T* value) const {
        return ref_map.at(value);
    }
    size_t size() const {
        return deref_map.size();
    }
    bool empty() const {
        return deref_map.empty();
    }
#ifndef DEBUG
private:
#endif
    std::unordered_map<size_t, const T*> deref_map;
    std::unordered_map<const T*, size_t> ref_map;
};

#endif
