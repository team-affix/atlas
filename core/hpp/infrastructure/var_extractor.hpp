#ifndef VAR_EXTRACTOR_HPP
#define VAR_EXTRACTOR_HPP

#include <unordered_set>
#include "../domain/interfaces/i_var_id_extractor.hpp"

struct var_extractor : i_var_extractor {
    explicit var_extractor(std::unordered_set<const expr*>&);
    void visit(const expr*) override;
private:
    std::unordered_set<const expr*>& vars_;
};

#endif
