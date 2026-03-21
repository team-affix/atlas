#include "../hpp/avoidance_trimmer.hpp"

avoidance_trimmer::avoidance_trimmer(
    avoidance_store& as,
    avoidance_map& am) : as(as), am(am) {
}

void avoidance_trimmer::operator()(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    
    // get the range of avoidances concerning the parent goal
    auto rng = am.equal_range(gl);

    for (auto it = rng.first; it != rng.second; ++it) {
        // get the iterator to the avoidance
        const auto& av = it->second;

        // if the avoidance doesn't contain resolution, it is mutually
        // exclusive with this context, thus can be discarded
        if (!av.contains(rl))
            as.erase(av);

        // otherwise, the avoidance is still relevant, thus can be kept
        // but the rl must be removed to indicate closer to conflict
        // (need to keep memory address of avoidance valid so use extract and move)
        auto node = as.extract(av);
        node.value().erase(rl);
        as.insert(std::move(node));

    }

    // this goal is resolved, thus its concerned avoidances are no longer needed
    am.erase(gl);
}
