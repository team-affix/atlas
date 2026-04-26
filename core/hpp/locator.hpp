#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include <unordered_map>
#include <stdexcept>

enum class locator_keys {
    inst_database,
    inst_goals,
    inst_trail,
    inst_var_sequencer,
    inst_bind_map,
    inst_normalizer,
    inst_expr_printer,
    inst_expr_pool,
    inst_goal_store,
    inst_candidate_store,
    inst_copier,
    inst_cdcl,
    inst_cdcl_eliminator,
    inst_head_eliminator,
    inst_frontier_watch,
    inst_resolution_lineage,
    inst_goal_lineage,
    inst_lineage_pool,
};

struct locator {
    template<typename T>
    T& operator()(locator_keys);
    template<typename T>
    void bind(locator_keys, T& value);
    void unbind(locator_keys);
    void purge();
#ifndef DEBUG
private:
#endif
    std::unordered_map<locator_keys, void*> entries;
};

template<typename T>
T& locator::operator()(locator_keys key) {
    return *reinterpret_cast<T*>(entries.at(key));
}

template<typename T>
void locator::bind(locator_keys key, T& value) {
    auto [it, success] = entries.insert({key, &value});

    if (!success)
        throw std::runtime_error("Key already bound");
}

#endif
