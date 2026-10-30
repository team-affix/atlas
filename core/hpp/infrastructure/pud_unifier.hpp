#ifndef PUD_UNIFIER_HPP
#define PUD_UNIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/ubs_class.hpp"

template<typename IGlobalize, typename IFind, typename IEquate, typename IInstantiate, typename IDetectCycle, typename IMakeFunctor, typename IDemodule>
struct pud_unifier {
    pud_unifier(
        IGlobalize& globalize,
        IFind& find,
        IEquate& equate,
        IInstantiate& instantiate,
        IDetectCycle& detect_cycle,
        IMakeFunctor& make_functor,
        IDemodule& demodule);
    std::optional<std::vector<ubs_class>> unify(moduled_expr lhs, moduled_expr rhs);
private:
    using pair_t = std::pair<moduled_expr, moduled_expr>;
    struct pair_hash {
        size_t operator()(const pair_t& pair) const noexcept;
    };

    ubs_class find_class(moduled_expr var);

    IGlobalize& globalize_;
    IFind& find_;
    IEquate& equate_;
    IInstantiate& instantiate_;
    IDetectCycle& detect_cycle_;
    IMakeFunctor& make_functor_;
    IDemodule& demodule_;
};

#define PUD_UNIFIER_TEMPLATE                                                                     \
    template<typename IGlobalize, typename IFind, typename IEquate, typename IInstantiate,      \
             typename IDetectCycle, typename IMakeFunctor, typename IDemodule>
#define PUD_UNIFIER                                                                                  \
    pud_unifier<IGlobalize, IFind, IEquate, IInstantiate, IDetectCycle, IMakeFunctor, IDemodule>

PUD_UNIFIER_TEMPLATE
PUD_UNIFIER::pud_unifier(
    IGlobalize& globalize,
    IFind& find,
    IEquate& equate,
    IInstantiate& instantiate,
    IDetectCycle& detect_cycle,
    IMakeFunctor& make_functor,
    IDemodule& demodule)
    : globalize_(globalize)
    , find_(find)
    , equate_(equate)
    , instantiate_(instantiate)
    , detect_cycle_(detect_cycle)
    , make_functor_(make_functor)
    , demodule_(demodule) {}

PUD_UNIFIER_TEMPLATE
std::optional<std::vector<ubs_class>> PUD_UNIFIER::unify(moduled_expr lhs, moduled_expr rhs) {
    std::vector<pair_t> pending{{lhs, rhs}};
    std::unordered_set<pair_t, pair_hash> unified_functors;
    std::vector<ubs_class> edited;
    std::vector<uint32_t> touched_class_roots;
    typename IDetectCycle::visitation binding_cycle_visitation;
    while (!pending.empty()) {
        const auto [left, right] = pending.back();
        pending.pop_back();
        if (left == right)
            continue;
        const bool left_is_var = std::holds_alternative<expr::var>(left.skeleton->content);
        const bool right_is_var = std::holds_alternative<expr::var>(right.skeleton->content);

        if (left_is_var && right_is_var) {
            const ubs_class left_class = find_class(left);
            const ubs_class right_class = find_class(right);
            if (left_class.root == right_class.root)
                continue;
            const std::optional<moduled_functor> displaced = equate_.equate(left_class, right_class);
            edited.push_back(left_class);
            edited.push_back(right_class);
            touched_class_roots.push_back(left_class.root);
            if (displaced.has_value()) {
                const expr::functor& functor = *displaced->functor;
                const expr* reconstructed = make_functor_.make_functor(functor.id, functor.args);
                pending.emplace_back(left, moduled_expr{reconstructed, displaced->frame_offset, displaced->module});
            }
            continue;
        }

        if (left_is_var || right_is_var) {
            const moduled_expr term = left_is_var ? right : left;
            const ubs_class var_class = find_class(left_is_var ? left : right);
            if (var_class.value.has_value()) {
                const expr::functor& functor = *var_class.value->functor;
                const expr* reconstructed = make_functor_.make_functor(functor.id, functor.args);
                pending.emplace_back(
                    moduled_expr{reconstructed, var_class.value->frame_offset, var_class.value->module}, term);
                continue;
            }
            instantiate_.instantiate(
                var_class,
                moduled_functor{std::get_if<expr::functor>(&term.skeleton->content), term.frame_offset, term.module});
            edited.push_back(var_class);
            touched_class_roots.push_back(var_class.root);
            continue;
        }

        const expr* left_global = demodule_.demodule(left);
        const expr* right_global = demodule_.demodule(right);
        const expr::functor& left_global_functor = std::get<expr::functor>(left_global->content);
        const expr::functor& right_global_functor = std::get<expr::functor>(right_global->content);
        const expr::functor& left_functor = std::get<expr::functor>(left.skeleton->content);
        const expr::functor& right_functor = std::get<expr::functor>(right.skeleton->content);
        if (left_global_functor.id != right_global_functor.id || left_functor.args.size() != right_functor.args.size())
            return std::nullopt;
        if (left_functor.args.empty() || !unified_functors.insert({left, right}).second)
            continue;
        for (size_t arg = left_functor.args.size(); arg-- > 0;)
            pending.emplace_back(
                moduled_expr{left_functor.args[arg], left.frame_offset, left.module},
                moduled_expr{right_functor.args[arg], right.frame_offset, right.module});
    }
    for (uint32_t class_root : touched_class_roots) {
        if (detect_cycle_.find_cycle_from_class(class_root, binding_cycle_visitation))
            return std::nullopt;
    }
    return edited;
}

PUD_UNIFIER_TEMPLATE
size_t PUD_UNIFIER::pair_hash::operator()(const pair_t& pair) const noexcept {
    const size_t left_skeleton = std::hash<const expr*>{}(pair.first.skeleton);
    const size_t left_frame = left_skeleton ^ (std::hash<uint32_t>{}(pair.first.frame_offset) + 0x9e3779b97f4a7c15ull
        + (left_skeleton << 6) + (left_skeleton >> 2));
    const size_t left_hash = left_frame ^ (std::hash<pud_module_id>{}(pair.first.module) + 0x9e3779b97f4a7c15ull
        + (left_frame << 6) + (left_frame >> 2));
    const size_t right_skeleton = std::hash<const expr*>{}(pair.second.skeleton);
    const size_t right_frame = right_skeleton ^ (std::hash<uint32_t>{}(pair.second.frame_offset) + 0x9e3779b97f4a7c15ull
        + (right_skeleton << 6) + (right_skeleton >> 2));
    const size_t right_hash = right_frame ^ (std::hash<pud_module_id>{}(pair.second.module) + 0x9e3779b97f4a7c15ull
        + (right_frame << 6) + (right_frame >> 2));
    return left_hash ^ (right_hash + 0x9e3779b97f4a7c15ull + (left_hash << 6) + (left_hash >> 2));
}

PUD_UNIFIER_TEMPLATE
ubs_class PUD_UNIFIER::find_class(moduled_expr var) {
    const uint32_t index = std::get<expr::var>(var.skeleton->content).index;
    return find_.find(globalize_.globalize(var.frame_offset, index));
}

#undef PUD_UNIFIER
#undef PUD_UNIFIER_TEMPLATE

#endif
