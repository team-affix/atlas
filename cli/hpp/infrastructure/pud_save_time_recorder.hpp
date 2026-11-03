#ifndef PUD_SAVE_TIME_RECORDER_HPP
#define PUD_SAVE_TIME_RECORDER_HPP

#include <optional>

// Wraps the save slot: forwards the save, then remembers when it finished, so the progress line can say
// how long ago the modules were last written. Before the first save there is no time to report.
template<typename INow, typename ISaveAllModules>
struct pud_save_time_recorder {
    using time_point = typename INow::time_point;
    using duration = typename INow::duration;
    pud_save_time_recorder(INow& now, ISaveAllModules& save_all_modules);
    void save_all();
    std::optional<duration> time_since_last_save() const;
private:
    INow& now_;
    ISaveAllModules& save_all_modules_;
    std::optional<time_point> last_save_;
};

template<typename INow, typename ISaveAllModules>
pud_save_time_recorder<INow, ISaveAllModules>::pud_save_time_recorder(INow& now, ISaveAllModules& save_all_modules)
    : now_(now)
    , save_all_modules_(save_all_modules)
    , last_save_() {}

template<typename INow, typename ISaveAllModules>
void pud_save_time_recorder<INow, ISaveAllModules>::save_all() {
    save_all_modules_.save_all();
    last_save_ = now_.now();
}

template<typename INow, typename ISaveAllModules>
std::optional<typename pud_save_time_recorder<INow, ISaveAllModules>::duration>
pud_save_time_recorder<INow, ISaveAllModules>::time_since_last_save() const {
    if (!last_save_.has_value())
        return std::nullopt;
    return now_.now() - last_save_.value();
}

#endif
