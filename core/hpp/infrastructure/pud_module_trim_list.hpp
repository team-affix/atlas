#ifndef PUD_MODULE_TRIM_LIST_HPP
#define PUD_MODULE_TRIM_LIST_HPP

#include <vector>

// The registered modules whose caches are trimmed together, once per search step.
template<typename ITrimModule>
struct pud_module_trim_list {
    pud_module_trim_list();
    void add(ITrimModule& module);
    void trim();
private:
    std::vector<ITrimModule*> modules_;
};

template<typename ITrimModule>
pud_module_trim_list<ITrimModule>::pud_module_trim_list()
    : modules_() {}

template<typename ITrimModule>
void pud_module_trim_list<ITrimModule>::add(ITrimModule& module) {
    modules_.push_back(&module);
}

template<typename ITrimModule>
void pud_module_trim_list<ITrimModule>::trim() {
    for (ITrimModule* module : modules_)
        module->trim();
}

#endif
