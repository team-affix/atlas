#ifndef DELTA_MAP_HPP
#define DELTA_MAP_HPP

#include <map>
#include <memory>
#include <optional>
#include <utility>
#include "trail.hpp"
#include "delta.hpp"
#include "tracked.hpp"

template<typename K, typename V>
struct delta<std::map<K, V>> {
    virtual ~delta<std::map<K, V>>() = default;
    delta(trail& t, const std::map<K, V>& value) : underlying(t, value) {}
    void insert(const K& key, const V& value) {
        // create the key register
        std::shared_ptr<std::optional<K>> key_reg =
            std::make_shared<std::optional<K>>(key);

        // create the value register
        std::shared_ptr<std::optional<V>> value_reg =
            std::make_shared<std::optional<V>>(value);

        underlying.mutate(
            [this, key, value, key_reg, value_reg](std::map<K, V>& m) {
                // take the value from the register
                m.emplace(std::move(*key_reg), std::move(*value_reg));
                key_reg.reset();
                value_reg.reset();
            },
            [this, key, value, key_reg, value_reg](std::map<K, V>& m) {
                // take the value from the map
                auto node = m.extract(key);
                key_reg->value() = std::move(node.key());
                value_reg->value() = std::move(node.value());
            }
        );
    }
    void assign(const K& key, const V& value) {
        // create the register for the value
        std::shared_ptr<V> reg =
            std::make_shared<V>(value);

        // create the swapper function
        auto swapper = [this, key, reg](std::map<K, V>& m) {
            std::swap(m.at(key), *reg);
        };
        
        // the swapper is an involution so fwd and bwd are same
        underlying.mutate(
            swapper,
            swapper
        );
    }
    void erase(const K& key) {
        // create the key register
        std::shared_ptr<std::optional<K>> key_reg =
            std::make_shared<std::optional<K>>(std::nullopt);

        // create the value register
        std::shared_ptr<std::optional<V>> value_reg =
            std::make_shared<std::optional<V>>(std::nullopt);

        underlying.mutate(
            [this, key, key_reg, value_reg](std::map<K, V>& m) {
                // take the value from the map
                auto node = m.extract(key);
                key_reg->value() = std::move(node.key());
                value_reg->value() = std::move(node.value());
            },
            [this, key, key_reg, value_reg](std::map<K, V>& m) {
                // take the value from the register
                m.emplace(std::move(*key_reg), std::move(*value_reg));
                key_reg.reset();
                value_reg.reset();
            }
        );
    }
    const std::map<K, V>& get() const {
        return underlying.get();
    }
#ifndef DEBUG
private:
#endif
    tracked<std::map<K, V>> underlying;
};

#endif
