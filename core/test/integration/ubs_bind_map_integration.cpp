#include <gtest/gtest.h>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "pud_unifier_bind_map_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

struct immer_parent_map {
    immer::map<uint32_t, uint32_t>::transient_type& map;

    const uint32_t* get_parent(uint32_t var) const {
        return map.find(var);
    }

    void set_parent(uint32_t child, uint32_t parent) {
        map.set(child, parent);
    }
};

struct immer_size_map {
    immer::map<uint32_t, uint32_t>::transient_type& map;

    const uint32_t* get_size(uint32_t root) const {
        return map.find(root);
    }

    void set_size(uint32_t root, uint32_t class_size) {
        map.set(root, class_size);
    }
};

struct immer_value_map {
    immer::map<uint32_t, const expr*>::transient_type& map;

    const expr* const* get_value(uint32_t root) const {
        return map.find(root);
    }

    void set_value(uint32_t root, const expr* value) {
        map.set(root, value);
    }
};

} // namespace

struct UbsBindMapIntegrationTest : public ::testing::Test {
    ubs_bind_map_expr_pool_backing backing;
    ubs_bind_map_expr_pool_backing::bind_map_t store{backing.make_bind_map()};
};

TEST_F(UbsBindMapIntegrationTest, ResolveUsesExprPoolHashConsForIsolatedVar) {
    const expr* expected = backing.pool.make_var(7u);
    EXPECT_EQ(store.resolve(7u), expected);
    EXPECT_EQ(store.resolve(7u), backing.pool.make_var(7u));
}

TEST_F(UbsBindMapIntegrationTest, EquateAndInstantiateShareCanonicalPointers) {
    const expr* bound = backing.pool.make_functor(11u, {});
    store.equate(0u, 1u);
    store.instantiate(0u, bound);
    EXPECT_EQ(store.resolve(1u), bound);
    EXPECT_EQ(store.resolve(0u), backing.pool.make_functor(11u, {}));
}

TEST(UbsBindMapImmerIntegrationTest, EquateAndInstantiateOnTransientMapsWithPool) {
    expr_pool pool;
    ubs_pool_var_factory vars{pool};
    auto parents = immer::map<uint32_t, uint32_t>{}.transient();
    auto sizes = immer::map<uint32_t, uint32_t>{}.transient();
    auto values = immer::map<uint32_t, const expr*>{}.transient();

    immer_parent_map parent_access{parents};
    immer_size_map size_access{sizes};
    immer_value_map value_access{values};

    ubs_bind_map<
        ubs_pool_var_factory,
        immer_parent_map,
        immer_parent_map,
        immer_size_map,
        immer_size_map,
        immer_value_map,
        immer_value_map> store{vars, parent_access, parent_access, size_access, size_access, value_access, value_access};

    const expr* bound = pool.make_functor(11u, {});
    store.equate(0u, 1u);
    store.instantiate(0u, bound);
    EXPECT_EQ(store.resolve(1u), bound);
}
