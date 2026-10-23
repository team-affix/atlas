#include "infrastructure/pud_changed_class_finder.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <immer/algorithm.hpp>

namespace {

struct changed_key_collector {
    std::vector<uint32_t>& keys;

    template<typename Entry>
    void added(const Entry& entry) {
        keys.push_back(entry.first);
    }

    template<typename Entry>
    void removed(const Entry& entry) {
        keys.push_back(entry.first);
    }

    template<typename Entry>
    void changed(const Entry& old_entry, const Entry&) {
        keys.push_back(old_entry.first);
    }
};

}

// Only roots of `before` are ever edited, so every changed key names a class of `before`.
std::vector<ubs_class> pud_changed_class_finder::find_changed_classes(
    const pud_ubs_state& before, const pud_ubs_state& after) const {
    std::vector<uint32_t> roots;
    immer::diff(before.parents, after.parents, changed_key_collector{roots});
    immer::diff(before.classes, after.classes, changed_key_collector{roots});
    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
    std::vector<ubs_class> classes;
    classes.reserve(roots.size());
    for (uint32_t root : roots) {
        const ubs_class* stored = before.classes.find(root);
        classes.push_back(stored ? *stored : ubs_class{.root = root, .size = 1, .rep = root, .value = std::nullopt});
    }
    return classes;
}
