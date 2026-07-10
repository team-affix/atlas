#ifndef BIND_MAP_TRAIL_HPP
#define BIND_MAP_TRAIL_HPP

#include <cstddef>
#include <vector>
#include "debug_assert.hpp"

// Shared undo coordinator for a family of bind maps that must roll back in
// lockstep with an owning frame stack (e.g. the local maps of the MHU
// elimination generator). A single depth counter is shared by every attached
// map, so advancing a frame is O(1) regardless of how many maps are alive.
// Each map registers itself only when it first opens a frame at a new depth,
// so pop/squash cost is proportional to the maps actually mutated in a frame,
// never to the total number of live maps.
template<typename IBindMap>
struct bind_map_trail {
    size_t depth() const { return depth_; }

    void push_frame() {
        ++depth_;
        touched_.emplace_back();
    }

    void pop_frame() {
        DEBUG_ASSERT(!touched_.empty());
        for (IBindMap* m : touched_.back())
            m->pop_top_frame();
        touched_.pop_back();
        --depth_;
    }

    void squash_frame() {
        DEBUG_ASSERT(!touched_.empty());
        std::vector<IBindMap*> top = std::move(touched_.back());
        touched_.pop_back();
        --depth_;
        const bool has_parent = !touched_.empty();
        for (IBindMap* m : top)
            if (m->squash_top_frame(depth_) && has_parent)
                touched_.back().push_back(m);
    }

    void register_touched(IBindMap* m) {
        DEBUG_ASSERT(!touched_.empty());
        touched_.back().push_back(m);
    }

private:
    size_t depth_ = 0;
    std::vector<std::vector<IBindMap*>> touched_;
};

#endif
