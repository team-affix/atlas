#include <gtest/gtest.h>
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/bind_map.hpp"

struct FactoriesIntegrationTest : public ::testing::Test {
protected:
    bind_map_factory bmf;
    overlay_bind_map_factory obmf;
    unifier_factory uf;

    bind_map local;
    bind_map remote;
    expr var0{expr::var{0}};
    expr func{expr::functor{"f", {}}};
};

TEST_F(FactoriesIntegrationTest, UnifierFromFactoriesUnifiesThroughOverlay) {
    auto overlay = obmf.make(local, remote);
    auto u = uf.make(*overlay);

    std::unordered_set<uint32_t> rep_changed;
    EXPECT_TRUE(u->unify(&var0, &func, rep_changed));
    EXPECT_EQ(rep_changed, (std::unordered_set<uint32_t>{0}));
    EXPECT_EQ(local.whnf(&var0), &func);
}

TEST_F(FactoriesIntegrationTest, UnifyThroughOverlayDoesNotMutateCommon) {
    auto overlay = obmf.make(local, remote);
    auto u = uf.make(*overlay);

    std::unordered_set<uint32_t> rep_changed;
    EXPECT_TRUE(u->unify(&var0, &func, rep_changed));

    EXPECT_EQ(remote.whnf(&var0), &var0);
    EXPECT_EQ(local.whnf(&var0), &func);
}
