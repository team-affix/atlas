#ifndef DBUCT_BIND_MAP_HPP
#define DBUCT_BIND_MAP_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include "infrastructure/bind_map_trail.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize>
struct dbuct_bind_map {
    dbuct_bind_map(IGlobalize& g);

    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);

    // Standalone frame control: the map owns its depth and lazily materializes
    // a frame only when it is actually mutated at that depth.
    void push_frame();
    void pop_frame();
    void squash_frame();

    // Trail-driven control: the map defers its depth to a shared trail and is
    // rolled back by that trail. push/pop/squash are not called directly.
    void attach_trail(bind_map_trail<dbuct_bind_map>* trail);
    void pop_top_frame();
    bool squash_top_frame(size_t parent_depth);

private:
    using map_t = std::unordered_map<uint32_t, framed_expr>;
    using preimage_t = std::optional<framed_expr>;

    struct frame {
        size_t depth;
        std::unordered_map<uint32_t, preimage_t> preimages;
    };

    size_t current_depth() const { return trail_ ? trail_->depth() : depth_; }
    void touch(uint32_t key, preimage_t prev);
    void undo_frame(frame& f);

    IGlobalize& globalizer_;
    map_t bindings_;
    std::vector<frame> frames_;
    size_t depth_ = 0;
    bind_map_trail<dbuct_bind_map>* trail_ = nullptr;
};

template<typename IGlobalize>
dbuct_bind_map<IGlobalize>::dbuct_bind_map(IGlobalize& g) : globalizer_(g) {}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::attach_trail(bind_map_trail<dbuct_bind_map>* trail) {
    trail_ = trail;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    bindings_.insert({global_key, std::move(value)});
    touch(global_key, std::nullopt);
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
        touch(global_key, it->second);
        it->second = resolved;
    }
    return resolved;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::push_frame() {
    ++depth_;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::pop_frame() {
    if (!frames_.empty() && frames_.back().depth == depth_) {
        undo_frame(frames_.back());
        frames_.pop_back();
    }
    DEBUG_ASSERT(depth_ > 0);
    --depth_;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::squash_frame() {
    DEBUG_ASSERT(depth_ > 0);
    if (!frames_.empty() && frames_.back().depth == depth_)
        (void)squash_top_frame(depth_ - 1);
    --depth_;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::pop_top_frame() {
    DEBUG_ASSERT(!frames_.empty());
    undo_frame(frames_.back());
    frames_.pop_back();
}

template<typename IGlobalize>
bool dbuct_bind_map<IGlobalize>::squash_top_frame(size_t parent_depth) {
    DEBUG_ASSERT(!frames_.empty());
    if (frames_.size() >= 2 && frames_[frames_.size() - 2].depth == parent_depth) {
        frame top = std::move(frames_.back());
        frames_.pop_back();
        auto& parent = frames_.back().preimages;
        for (auto& [key, prev] : top.preimages)
            parent.emplace(key, std::move(prev));
        return false;
    }
    frames_.back().depth = parent_depth;
    return true;
}

template<typename IGlobalize>
void dbuct_bind_map<IGlobalize>::touch(uint32_t key, preimage_t prev) {
    const size_t d = current_depth();
    DEBUG_ASSERT(trail_ != nullptr || d > 0);
    if (frames_.empty() || frames_.back().depth != d) {
        frames_.push_back(frame{d, {}});
        if (trail_)
            trail_->register_touched(this);
    }
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
