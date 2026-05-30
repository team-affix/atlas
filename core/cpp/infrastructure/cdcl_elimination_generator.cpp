#include <memory>
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_at_insert.hpp"
#include "infrastructure/backtrackable_map_at_erase.hpp"

cdcl_elimination_generator::cdcl_elimination_generator(locator& loc) :
    avoidances(loc.locate<i_log_to_current_trail_frame>(), {}),
    watched_goals(loc.locate<i_log_to_current_trail_frame>(), {}),
    cdcl_sequencer(loc.locate<i_cdcl_sequencer>()) {
}

std::optional<const resolution_lineage*> cdcl_elimination_generator::learn(const lemma& l) {
    const auto& resolutions = l.get_resolutions();

    avoidance_type av{resolutions.begin(), resolutions.end()};

    return insert(std::move(av));
}

coroutine<const resolution_lineage*, void> cdcl_elimination_generator::constrain(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;

    auto it = watched_goals.get().find(gl);

    if (it == watched_goals.get().end())
        co_return;

    auto av_ids = it->second;

    for (avoidance_id id : av_ids) {
        const avoidance_type& av = avoidances.get().at(id);
        if (!av.contains(rl)) {
            erase(id);
            continue;
        }

        // reduce avoidance
        avoidance_type reduced = av;
        reduced.erase(rl);

        // erase old avoidance
        erase(id);

        // insert new avoidance
        if (auto elim = insert(std::move(reduced)))
            co_yield *elim;
    }

    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        watched_goals_type>>(
            gl);
    watched_goals.mutate(std::move(erase_mut));
}

std::optional<const resolution_lineage*> cdcl_elimination_generator::insert(avoidance_type av) {
    if (av.size() == 1)
        return *av.begin();

    avoidance_id id = cdcl_sequencer.next();

    auto insert_mut = std::make_unique<
        backtrackable_map_insert<
        avoidances_type>>(
            id, av);
    avoidances.mutate(std::move(insert_mut));

    for (const resolution_lineage* rl : av)
        link(rl->parent, id);

    return std::nullopt;
}

void cdcl_elimination_generator::link(const goal_lineage* gl, avoidance_id id) {
    if (!watched_goals.get().contains(gl)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_insert<
            watched_goals_type>>(
                gl, std::unordered_set<avoidance_id>{});
        watched_goals.mutate(std::move(insert_mut));
    }

    if (!watched_goals.get().at(gl).contains(id)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_at_insert<
            watched_goals_type>>(
                gl, id);
        watched_goals.mutate(std::move(insert_mut));
    }
}

void cdcl_elimination_generator::erase(avoidance_id id) {
    const avoidance_type& av = avoidances.get().at(id);

    for (const resolution_lineage* rl : av) {
        auto erase_mut = std::make_unique<
            backtrackable_map_at_erase<
            watched_goals_type>>(
                rl->parent, id);
        watched_goals.mutate(std::move(erase_mut));
    }

    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        avoidances_type>>(
            id);
    avoidances.mutate(std::move(erase_mut));
}
