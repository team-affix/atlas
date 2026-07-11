#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize>
struct dbuct_bind_map {
    dbuct_bind_map(IGlobalize& g);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

    void push_frame();
    void pop_frame();

private:
    using map_t = std::unordered_map<uint32_t, framed_expr>;
    using preimage_t = std::optional<framed_expr>;

    struct frame {
        std::unordered_map<uint32_t, preimage_t> preimages;
    };

    void record(uint32_t key, preimage_t prev);
    void undo_frame(frame& f);

    IGlobalize& globalizer_;
    map_t bindings_;
    std::vector<frame> frames_{frame{}};
};

template<typename IGlobalize>
dbuct_bind_map<IGlobalize>::dbuct_bind_map(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    bindings_.insert({global_key, std::move(value)});
    record(global_key, std::nullopt);
}

template<typename IGlobalize>
framed_expr dbuct_bind_map<IGlobalize>::whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    auto it = bindings_.find(global_key);
    if (it == bindings_.end())
        return fe;
    const framed_expr resolved = whnf(it->second);
    if (!(it->second == resolved)) {
        record(global_key, it->second);
        it->second = resolved;
    }
    return resolved;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::push_frame() {
    frames_.emplace_back();
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::pop_frame() {
    undo_frame(frames_.back());
    frames_.pop_back();
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::record(uint32_t key, preimage_t prev) {
    frames_.back().preimages.emplace(key, std::move(prev));
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::undo_frame(frame& f) {
    for (auto& [key, prev] : f.preimages) {
        if (prev)
            bindings_[key] = std::move(*prev);
        else
            bindings_.erase(key);
    }
}

#endif
