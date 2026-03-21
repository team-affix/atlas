#include "../hpp/avoidance_trimmer.hpp"

avoidance_trimmer::avoidance_trimmer(
    avoidance_store& as,
    const avoidance_map& am) : as(as), am(am) {
}

void avoidance_trimmer::operator()(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;
    
    // get the range of avoidances concerning the parent goal
    auto [first, last] = am.equal_range(gl);

    // remove the resolution from the concerned avoidances
    for (auto it = first; it != last; ++it) {
        avoidance& av = as.at(it->second);
        if (av.contains(rl))
            av.erase(rl);
    }

}
