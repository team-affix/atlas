#ifndef BT_CDCL_ACTION_HPP
#define BT_CDCL_ACTION_HPP

#include <variant>
#include "value_objects/bt_cdcl_nand_fired.hpp"
#include "value_objects/bt_cdcl_visit_delta.hpp"

using bt_cdcl_action = std::variant<bt_cdcl_visit_delta, bt_cdcl_nand_fired>;

#endif
