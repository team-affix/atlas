#ifndef FUNCTOR_FIXTURE_HPP
#define FUNCTOR_FIXTURE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include "infrastructure/functor_names.hpp"

struct test_functors {
    functor_names names;
    std::unordered_map<std::string, uint32_t> cache;
    uint32_t next_id = k_first_user_functor_id;

    uint32_t id(const char* s) {
        std::string key{s};
        if (key == "nil")
            return k_nil_functor_id;
        if (key == "cons")
            return k_cons_functor_id;
        if (auto it = cache.find(key); it != cache.end())
            return it->second;
        const uint32_t assigned = next_id++;
        names.set_name(assigned, key);
        cache.emplace(std::move(key), assigned);
        return assigned;
    }

    uint32_t id(const std::string& s) {
        return id(s.c_str());
    }
};

#endif
