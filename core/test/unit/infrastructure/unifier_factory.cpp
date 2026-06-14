// unifier_factory: constructs a unifier wired to the supplied bind map. Unification
// must delegate WHNF lookups to that bind map collaborator.

#include <unordered_set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/unifier_factory.hpp"
#include "interfaces/i_bind_map.hpp"
#include "functor_fixture.hpp"

using ::testing::Return;
using ::testing::_;

namespace {

bool run_unify(i_unifier& u, const expr* lhs, const expr* rhs,
               std::unordered_set<uint32_t>& vars_touched) {
    auto task = u.unify(lhs, rhs);
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            vars_touched.insert(task.consume_yield());
    }
    return task.result();
}

} // namespace

struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void, bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*), (override));
};

struct UnifierFactoryTest : public ::testing::Test {
    
    test_functors functors;unifier_factory factory;
    MockBindMap bind_map;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func_f{expr::functor{functors.id("f"), {}}};
    expr func_g{expr::functor{functors.id("g"), {}}};
};

TEST_F(UnifierFactoryTest, MakeProducesUnifierThatUnifiesViaBindMap) {
    EXPECT_CALL(bind_map, whnf).WillRepeatedly([](const expr* e) { return e; });
    EXPECT_CALL(bind_map, bind(1u, &var0)).Times(1);
    std::unique_ptr<i_unifier> u = factory.make(bind_map);
    ASSERT_NE(u, nullptr);
    std::unordered_set<uint32_t> vars_touched;
    EXPECT_TRUE(run_unify(*u, &var0, &var1, vars_touched));
}

TEST_F(UnifierFactoryTest, MakeProducesUnifierThatReportsUnifyFailure) {
    EXPECT_CALL(bind_map, whnf).WillRepeatedly([](const expr* e) { return e; });
    EXPECT_CALL(bind_map, bind(_, _)).Times(0);
    std::unique_ptr<i_unifier> u = factory.make(bind_map);
    ASSERT_NE(u, nullptr);
    std::unordered_set<uint32_t> vars_touched;
    EXPECT_FALSE(run_unify(*u, &func_f, &func_g, vars_touched));
}
