#ifndef DELTA_SET_HPP
#define DELTA_SET_HPP

#include <set>
#include <memory>
#include <optional>
#include "delta.hpp"
#include "tracked.hpp"

template<typename T>
struct delta<std::set<T>> {
    delta(trail& t, const std::set<T>& value) : underlying(t, value) {}
    void insert(const T& value) {
        // if the value is already in the set, do nothing
        if (underlying.get().contains(value))
            return;

        // register the value for undo/redo
        std::shared_ptr<std::optional<T>> reg =
            std::make_shared<std::optional<T>>(value);

        // mutate the underlying set
        underlying.mutate(
            [this, value, reg](std::set<T>& s) {
                // take the value from the register
                s.insert(std::move(*reg));
                reg.reset();
            },
            [this, value, reg](std::set<T>& s) {
                // take the value from the set
                auto node = s.extract(value);
                reg->value() = std::move(node.key());
            }
        );
    }
    void erase(const T& value) {
        // if the value is not in the set, do nothing
        if (!underlying.get().contains(value))
            return;
        
        // register the value for undo/redo
        std::shared_ptr<std::optional<T>> reg =
            std::make_shared<std::optional<T>>(std::nullopt);
    
        // mutate the underlying set
        underlying.mutate(
            [this, value, reg](std::set<T>& s) {
                // take the value from the set
                auto node = s.extract(value);
                reg->value() = std::move(node.key());
            },
            [this, value, reg](std::set<T>& s) {
                // take the value from the register
                s.insert(std::move(*reg));
                reg.reset();
            }
        );
    }
    const std::set<T>& get() const {
        return underlying.get();
    }
#ifndef DEBUG
private:
#endif
    tracked<std::set<T>> underlying;
};

#endif
