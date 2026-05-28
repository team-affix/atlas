#include <stdexcept>
#include "../../hpp/infrastructure/copier.hpp"

copier::copier(i_var_sequencer& var_seq, i_make_functor& make_functor, i_make_var& make_var) :
    var_seq_ref(var_seq),
    make_functor_ref(make_functor),
    make_var_ref(make_var) {
}

const expr* copier::copy(const expr* e, translation_map& variable_map) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        if (!variable_map.contains(v->index))
            variable_map.emplace(v->index, var_seq_ref.next());
        return make_var_ref.make(variable_map.at(v->index));
    }

    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> copied_args;
        copied_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            copied_args.push_back(copy(arg, variable_map));
        return make_functor_ref.make(f->name, copied_args);
    }

    throw std::runtime_error("Unsupported expression type");
}
