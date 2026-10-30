#ifndef PUD_CYCLE_DETECTOR_HPP
#define PUD_CYCLE_DETECTOR_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/framed_expr_hash.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/ubs_class.hpp"

template<typename IGlobalize, typename IFind, typename IDemoduleFunctor>
struct pud_cycle_detector {
    struct visitation {
        visitation();
    private:
        using class_marks_t = std::unordered_map<uint32_t, bool>;
        using term_marks_t = std::unordered_map<framed_expr, bool, framed_expr_hash>;
        class_marks_t class_marks_;
        term_marks_t term_marks_;
        friend struct pud_cycle_detector;
    };

    pud_cycle_detector(IGlobalize& globalize, IFind& find, IDemoduleFunctor& demodule_functor);
    bool find_cycle_from_class(uint32_t instantiated_class_root, visitation& visitation);
private:
    struct frame {
        bool* finished;
        std::vector<framed_expr> pending;
    };

    bool enter_term(framed_expr term, visitation& visitation, std::vector<frame>& path);
    bool enter_class(const ubs_class& entered, visitation& visitation, std::vector<frame>& path);

    IGlobalize& globalize_;
    IFind& find_;
    IDemoduleFunctor& demodule_functor_;
};

template<typename IG, typename IF, typename IDF>
pud_cycle_detector<IG, IF, IDF>::visitation::visitation()
    : class_marks_()
    , term_marks_() {}

template<typename IG, typename IF, typename IDF>
pud_cycle_detector<IG, IF, IDF>::pud_cycle_detector(IG& globalize, IF& find, IDF& demodule_functor)
    : globalize_(globalize)
    , find_(find)
    , demodule_functor_(demodule_functor) {}

template<typename IG, typename IF, typename IDF>
bool pud_cycle_detector<IG, IF, IDF>::find_cycle_from_class(
    uint32_t instantiated_class_root, visitation& visitation) {
    std::vector<frame> path;
    if (enter_class(find_.find(instantiated_class_root), visitation, path))
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
        if (enter_term(next, visitation, path))
            return true;
    }
    return false;
}

template<typename IG, typename IF, typename IDF>
bool pud_cycle_detector<IG, IF, IDF>::enter_term(
    framed_expr term, visitation& visitation, std::vector<frame>& path) {
    if (const expr::var* var = std::get_if<expr::var>(&term.skeleton->content))
        return enter_class(
            find_.find(globalize_.globalize(term.frame_offset, var->index)), visitation, path);
    const std::vector<const expr*>& args = std::get<expr::functor>(term.skeleton->content).args;
    if (args.empty())
        return false;
    const auto [mark, unseen] = visitation.term_marks_.try_emplace(term, false);
    if (!unseen)
        return !mark->second;
    frame entered{&mark->second, {}};
    for (const expr* arg : args)
        entered.pending.push_back(framed_expr{arg, term.frame_offset});
    path.push_back(std::move(entered));
    return false;
}

template<typename IG, typename IF, typename IDF>
bool pud_cycle_detector<IG, IF, IDF>::enter_class(
    const ubs_class& entered, visitation& visitation, std::vector<frame>& path) {
    const auto [mark, unseen] = visitation.class_marks_.try_emplace(entered.root, false);
    if (!unseen)
        return !mark->second;
    frame class_frame{&mark->second, {}};
    if (entered.value.has_value()) {
        const expr::functor& functor = *entered.value->functor;
        const expr* reconstructed = demodule_functor_.demodule(
            moduled_functor{&functor, entered.value->frame_offset, entered.value->module});
        class_frame.pending.push_back(framed_expr{reconstructed, entered.value->frame_offset});
    }
    path.push_back(std::move(class_frame));
    return false;
}

#endif
