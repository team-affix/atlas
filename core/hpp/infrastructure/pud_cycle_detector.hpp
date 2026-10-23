#ifndef PUD_CYCLE_DETECTOR_HPP
#define PUD_CYCLE_DETECTOR_HPP

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/framed_expr_hash.hpp"
#include "value_objects/ubs_class.hpp"

// Depth-first search over the graph whose nodes are classes and functor subterms. A node reached
// again while it is still on the search path closes a cycle; a finished node is never searched twice,
// so shared subterms cost nothing extra.
template<typename IGlobalize, typename IFind>
struct pud_cycle_detector {
    pud_cycle_detector(IGlobalize& globalize, IFind& find);
    bool detect_cycle(const std::vector<uint32_t>& vars);
private:
    using class_marks_t = std::unordered_map<uint32_t, bool>;
    using term_marks_t = std::unordered_map<framed_expr, bool, framed_expr_hash>;
    struct frame {
        bool* finished;
        std::vector<framed_expr> pending;
    };

    bool enter_term(framed_expr term, class_marks_t& class_marks, term_marks_t& term_marks, std::vector<frame>& path);
    bool enter_class(const ubs_class& entered, class_marks_t& class_marks, std::vector<frame>& path);

    IGlobalize& globalize_;
    IFind& find_;
};

template<typename IG, typename IF>
pud_cycle_detector<IG, IF>::pud_cycle_detector(IG& globalize, IF& find)
    : globalize_(globalize)
    , find_(find) {}

template<typename IG, typename IF>
bool pud_cycle_detector<IG, IF>::detect_cycle(const std::vector<uint32_t>& vars) {
    class_marks_t class_marks;
    term_marks_t term_marks;
    std::vector<frame> path;
    for (uint32_t var : vars) {
        if (enter_class(find_.find(var), class_marks, path))
            return true;
        while (!path.empty()) {
            frame& top = path.back();
            if (top.pending.empty()) {
                *top.finished = true;
                path.pop_back();
                continue;
            }
            const framed_expr next = top.pending.back();
            top.pending.pop_back();
            if (enter_term(next, class_marks, term_marks, path))
                return true;
        }
    }
    return false;
}

template<typename IG, typename IF>
bool pud_cycle_detector<IG, IF>::enter_term(
    framed_expr term, class_marks_t& class_marks, term_marks_t& term_marks, std::vector<frame>& path) {
    if (const expr::var* var = std::get_if<expr::var>(&term.skeleton->content))
        return enter_class(find_.find(globalize_.globalize(term.frame_offset, var->index)), class_marks, path);
    const std::vector<const expr*>& args = std::get<expr::functor>(term.skeleton->content).args;
    if (args.empty())
        return false;
    const auto [mark, unseen] = term_marks.try_emplace(term, false);
    if (!unseen)
        return !mark->second;
    frame entered{&mark->second, {}};
    for (const expr* arg : args)
        entered.pending.push_back(framed_expr{arg, term.frame_offset});
    path.push_back(std::move(entered));
    return false;
}

template<typename IG, typename IF>
bool pud_cycle_detector<IG, IF>::enter_class(
    const ubs_class& entered, class_marks_t& class_marks, std::vector<frame>& path) {
    const auto [mark, unseen] = class_marks.try_emplace(entered.root, false);
    if (!unseen)
        return !mark->second;
    frame class_frame{&mark->second, {}};
    if (entered.value.has_value())
        class_frame.pending.push_back(entered.value.value());
    path.push_back(std::move(class_frame));
    return false;
}

#endif
