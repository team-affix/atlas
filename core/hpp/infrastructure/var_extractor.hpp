#ifndef VAR_EXTRACTOR_HPP
#define VAR_EXTRACTOR_HPP

#include "../domain/interfaces/i_var_id_extractor.hpp"
#include "../domain/interfaces/i_set.hpp"

struct var_extractor : i_var_extractor {
    explicit var_extractor(i_set<uint32_t>&);
    void visit(const expr*) override;
private:
    i_set<uint32_t>& vars_;
};

#endif
