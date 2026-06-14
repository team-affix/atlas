#include <gtest/gtest.h>
#include <optional>
#include "locator_fixture.hpp"
#include "infrastructure/copier.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/var_sequencer.hpp"
#include "infrastructure/trail.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"

struct CopierIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        loc.bind_as<i_log_to_current_trail_frame>(t);
        vs.emplace(loc, 0);
        loc.bind_as<i_var_sequencer>(*vs);
        vs->next();
        vs->next();
        pool.emplace();
        loc.bind_as<i_make_functor, i_make_var>(*pool);
        cp.emplace(loc);
    }

    locator loc;
    trail t;
    std::optional<var_sequencer> vs;
    std::optional<expr_pool> pool;
    std::optional<copier> cp;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
};

static uint32_t var_index(const expr* e) {
    return std::get<expr::var>(e->content).index;
}

TEST_F(CopierIntegrationTest, CopyNestedFunctorRemapsAllVarsInRealPool) {
    expr inner{expr::functor{"g", {&var0}}};
    expr outer{expr::functor{"f", {&inner, &var1}}};
    translation_map map;
    const expr* p = cp->copy(&outer, map);
    const expr::functor& f = std::get<expr::functor>(p->content);
    const expr::functor& g = std::get<expr::functor>(f.args[0]->content);
    EXPECT_EQ(var_index(g.args[0]), 2);
    EXPECT_EQ(var_index(f.args[1]), 3);
    EXPECT_EQ(map.at(0), 2);
    EXPECT_EQ(map.at(1), 3);
    EXPECT_EQ(p, pool->make("f", {pool->make("g", {pool->make(2)}), pool->make(3)}));
}

TEST_F(CopierIntegrationTest, CopySeparateCallsAdvanceSequencer) {
    translation_map map1;
    translation_map map2;
    const expr* p1 = cp->copy(&var0, map1);
    const expr* p2 = cp->copy(&var0, map2);
    EXPECT_EQ(var_index(p1), 2);
    EXPECT_EQ(var_index(p2), 3);
}
