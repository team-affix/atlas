#ifndef TRACKED_MAP_HPP
#define TRACKED_MAP_HPP

#include <map>
#include "tracked.hpp"
#include "trail.hpp"

template<typename K, typename V>
struct tracked<std::map<K, V>> {
    tracked<std::map<K, V>>(trail&);
    void upsert(const K&, const V&);
    void erase(const K&);
    const std::map<K, V>& get() const;
#ifndef DEBUG
private:
#endif
    trail& t;
    std::map<K, V> members;
};

template<typename K, typename V>
void tracked<std::map<K, V>>::upsert(const K& key, const V& value) {
    auto [it, inserted] = members.insert({key, value});
    if (inserted) {
        t.log([this, key, value]() { members.erase(key); });
    }
    else {
        V old_value = it->second;
        it->second = value;
        t.log([this, key, old_value]() { members.at(key) = old_value; });
    }
}

template<typename K, typename V>
void tracked<std::map<K, V>>::erase(const K& key) {
    auto it = members.find(key);
    if (it == members.end())
        return;
    V old_value = it->second;
    members.erase(it);
    t.log([this, key, old_value]() { members.insert({key, old_value}); });
}

#endif
