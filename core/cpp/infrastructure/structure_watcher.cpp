#include "infrastructure/structure_watcher.hpp"
#include <algorithm>

structure_watcher::structure_watcher()
    : reps_to_heads_(), heads_to_reps_() {}

structure_watcher::head_set_t
structure_watcher::query_heads(om_label open, uint32_t rep) const {
    const auto result = reps_to_heads_.query(open, rep);
    if (!result)
        return head_set_t{};
    return *result;
}

structure_watcher::rep_set_t
structure_watcher::query_reps(om_label open, watcher_head_id head) const {
    const auto result = heads_to_reps_.query(open, head);
    if (!result)
        return rep_set_t{};
    return *result;
}

void structure_watcher::watch(om_interval interval,
                              watcher_head_id head,
                              const std::vector<uint32_t>& reps) {
    rep_set_t rep_set = query_reps(interval.open, head);
    for (uint32_t rep : reps)
        rep_set = rep_set.insert(rep);
    heads_to_reps_.record(interval, head, rep_set);

    for (uint32_t rep : reps) {
        head_set_t head_set = query_heads(interval.open, rep);
        head_set = head_set.insert(head);
        reps_to_heads_.record(interval, rep, head_set);
    }
}

std::vector<watcher_head_id>
structure_watcher::note_var_bind(om_interval interval,
                                 uint32_t bound, uint32_t target) {
    const head_set_t bound_heads = query_heads(interval.open, bound);

    std::vector<watcher_head_id> changed;
    head_set_t new_bound_heads = bound_heads;
    head_set_t new_target_heads = query_heads(interval.open, target);

    for (watcher_head_id head : bound_heads) {
        rep_set_t head_reps = query_reps(interval.open, head);
        const bool collapse = head_reps.contains(target);

        head_reps = head_reps.erase(bound);
        if (!collapse)
            head_reps = head_reps.insert(target);
        heads_to_reps_.record(interval, head, head_reps);

        new_bound_heads = new_bound_heads.erase(head);
        if (!collapse)
            new_target_heads = new_target_heads.insert(head);

        if (collapse)
            changed.push_back(head);
    }

    reps_to_heads_.record(interval, bound, new_bound_heads);
    reps_to_heads_.record(interval, target, new_target_heads);

    std::sort(changed.begin(), changed.end());
    return changed;
}

std::vector<watcher_head_id>
structure_watcher::note_functor_bind(om_interval interval,
                                     uint32_t bound,
                                     const std::vector<uint32_t>& introduced_reps) {
    const head_set_t bound_heads = query_heads(interval.open, bound);

    std::vector<watcher_head_id> changed;
    for (watcher_head_id head : bound_heads)
        changed.push_back(head);

    for (watcher_head_id head : bound_heads) {
        rep_set_t head_reps = query_reps(interval.open, head);
        head_reps = head_reps.erase(bound);
        for (uint32_t rep : introduced_reps)
            head_reps = head_reps.insert(rep);
        heads_to_reps_.record(interval, head, head_reps);
    }

    head_set_t empty_heads;
    reps_to_heads_.record(interval, bound, empty_heads);

    for (uint32_t rep : introduced_reps) {
        head_set_t rep_heads = query_heads(interval.open, rep);
        for (watcher_head_id head : bound_heads)
            rep_heads = rep_heads.insert(head);
        reps_to_heads_.record(interval, rep, rep_heads);
    }

    std::sort(changed.begin(), changed.end());
    return changed;
}

std::vector<watcher_head_id>
structure_watcher::heads_of(om_label open, uint32_t rep) const {
    const head_set_t head_set = query_heads(open, rep);
    std::vector<watcher_head_id> result;
    for (watcher_head_id h : head_set)
        result.push_back(h);
    return result;
}

std::vector<uint32_t>
structure_watcher::reps_of(om_label open, watcher_head_id head) const {
    const rep_set_t rep_set = query_reps(open, head);
    std::vector<uint32_t> result;
    for (uint32_t r : rep_set)
        result.push_back(r);
    return result;
}
