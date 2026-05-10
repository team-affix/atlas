#ifndef RESOLUTION_BIND_MAP_STORE_HPP
#define RESOLUTION_BIND_MAP_STORE_HPP

#include <unordered_map>
#include <memory>
#include "../domain/interfaces/i_resolution_bind_map_store.hpp"

struct resolution_bind_map_store : i_resolution_bind_map_store {
    void insert(const resolution_lineage*, std::unique_ptr<i_bind_map>) override;
    std::unique_ptr<i_bind_map>& get(const resolution_lineage*) override;
    void erase(const resolution_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const resolution_lineage*, std::unique_ptr<i_bind_map>> maps;
};

#endif
