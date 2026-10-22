#ifndef PUD_CYCLE_DETECTOR_HPP
#define PUD_CYCLE_DETECTOR_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IGlobalize, typename IFind, typename IFindValue>
struct pud_cycle_detector {
    pud_cycle_detector(IGlobalize& globalize, IFind& find, IFindValue& find_value);
    bool detect_cycle(const std::vector<uint32_t>& vars);
private:
    struct frame {
        uint32_t root;
        std::vector<framed_expr> pending;
    };

    void enter(uint32_t root, std::unordered_map<uint32_t, bool>& finished, std::vector<frame>& path);

    IGlobalize& globalize_;
    IFind& find_;
    IFindValue& find_value_;
};

template<typename IG, typename IF, typename IFV>
pud_cycle_detector<IG, IF, IFV>::pud_cycle_detector(IG& globalize, IF& find, IFV& find_value)
    : globalize_(globalize)
    , find_(find)
    , find_value_(find_value) {}

template<typename IG, typename IF, typename IFV>
bool pud_cycle_detector<IG, IF, IFV>::detect_cycle(const std::vector<uint32_t>& vars) {
    std::unordered_map<uint32_t, bool> finished;
    for (uint32_t var : vars) {
        const uint32_t start = find_.find(var);
        if (finished.contains(start))
            continue;
        std::vector<frame> path;
        enter(start, finished, path);
        while (!path.empty()) {
            frame& top = path.back();
            if (top.pending.empty()) {
                finished[top.root] = true;
                path.pop_back();
                continue;
            }
            const framed_expr next = top.pending.back();
            top.pending.pop_back();
            const expr::var* next_var = std::get_if<expr::var>(&next.skeleton->content);
            if (!next_var) {
                for (const expr* arg : std::get<expr::functor>(next.skeleton->content).args)
                    top.pending.push_back(framed_expr{arg, next.frame_offset});
                continue;
            }
            const uint32_t root = find_.find(globalize_.globalize(next.frame_offset, next_var->index));
            const auto visited = finished.find(root);
            if (visited == finished.end()) {
                enter(root, finished, path);
                continue;
            }
            if (!visited->second)
                return true;
        }
    }
    return false;
}

template<typename IG, typename IF, typename IFV>
void pud_cycle_detector<IG, IF, IFV>::enter(
    uint32_t root, std::unordered_map<uint32_t, bool>& finished, std::vector<frame>& path) {
    finished[root] = false;
    frame entered{root, {}};
    if (const std::optional<framed_expr> value = find_value_.find_value(root))
        entered.pending.push_back(value.value());
    path.push_back(entered);
}

#endif
