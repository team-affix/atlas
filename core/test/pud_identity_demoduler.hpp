#ifndef PUD_IDENTITY_DEMODULER_HPP
#define PUD_IDENTITY_DEMODULER_HPP

#include <string>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"

struct pud_identity_global_functor_ids {
    bool contains_module(pud_module_id module) const;
    uint32_t get_global_functor_id_from_local_functor_id(pud_module_id module, uint32_t local_functor_id) const;
};

inline bool pud_identity_global_functor_ids::contains_module(pud_module_id) const {
    return true;
}

inline uint32_t pud_identity_global_functor_ids::get_global_functor_id_from_local_functor_id(
    pud_module_id, uint32_t local_functor_id) const {
    return local_functor_id;
}

struct pud_unused_functor_module {
    uint32_t append_functor_name(const std::string& name);
    const std::vector<std::string>& functor_names() const;
};

inline uint32_t pud_unused_functor_module::append_functor_name(const std::string&) {
    return 0;
}

inline const std::vector<std::string>& pud_unused_functor_module::functor_names() const {
    static const std::vector<std::string> names;
    return names;
}

struct pud_identity_local_functor_ids {
    pud_identity_local_functor_ids();
    bool contains_global_functor_id(pud_module_id module, uint32_t global_functor_id) const;
    bool contains_named_functor_id(uint32_t global_functor_id) const;
    uint32_t get_local_functor_id_from_global_functor_id(pud_module_id module, uint32_t global_functor_id) const;
    const std::string& get_name_by_global_functor_id(uint32_t global_functor_id) const;
    pud_unused_functor_module& get_module(pud_module_id module);
    void insert_link(pud_module_id module, uint32_t local_functor_id, uint32_t global_functor_id);
private:
    pud_unused_functor_module module_;
    std::string missing_name_;
};

inline pud_identity_local_functor_ids::pud_identity_local_functor_ids()
    : module_()
    , missing_name_() {}

inline bool pud_identity_local_functor_ids::contains_global_functor_id(pud_module_id, uint32_t) const {
    return true;
}

inline bool pud_identity_local_functor_ids::contains_named_functor_id(uint32_t) const {
    return false;
}

inline uint32_t pud_identity_local_functor_ids::get_local_functor_id_from_global_functor_id(
    pud_module_id, uint32_t global_functor_id) const {
    return global_functor_id;
}

inline const std::string& pud_identity_local_functor_ids::get_name_by_global_functor_id(uint32_t) const {
    return missing_name_;
}

inline pud_unused_functor_module& pud_identity_local_functor_ids::get_module(pud_module_id) {
    return module_;
}

inline void pud_identity_local_functor_ids::insert_link(pud_module_id, uint32_t, uint32_t) {}

template<typename IMakeFunctor>
struct pud_copy_functor_demoduler {
    pud_copy_functor_demoduler(IMakeFunctor& make_functor);
    const expr* demodule(moduled_functor functor);
private:
    IMakeFunctor& make_functor_;
};

template<typename IMakeFunctor>
pud_copy_functor_demoduler<IMakeFunctor>::pud_copy_functor_demoduler(IMakeFunctor& make_functor)
    : make_functor_(make_functor) {}

template<typename IMakeFunctor>
const expr* pud_copy_functor_demoduler<IMakeFunctor>::demodule(moduled_functor functor) {
    return make_functor_.make_functor(functor.functor->id, functor.functor->args);
}

#endif
