// import_database_from_file: end-to-end file → i_push_db_rule. Invariants: fact/rule
// classification, head structure from fixtures, missing file throws.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "parser_fixture.hpp"
#include "infrastructure/functor_names.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include "infrastructure/db.hpp"
#include "value_objects/lineage.hpp"

using ::testing::IsEmpty;
using ::testing::SizeIs;

static const rule* rule_at(const db& database, rule_id id) {
    return database.get_rule(id);
}

struct ImportDatabaseFromFileTest : ParserCoreFixture {};

TEST_F(ImportDatabaseFromFileTest, FactsFixture) {
    db database;
    import_database_from_file("parser/fixtures/facts.chc", *pool, *pool, database, functor_map, next_functor_id);

    for (rule_id id = 0; id < 3; ++id) {
        EXPECT_THAT(rule_at(database, id)->body, IsEmpty());
    }
    EXPECT_EQ(rule_at(database, rule_id{0})->head, pool->make_functor(functor_map.at("base"), {pool->make_functor(functor_map.at("0"), {})}));
    EXPECT_EQ(rule_at(database, rule_id{1})->head, pool->make_functor(functor_map.at("base"), {pool->make_functor(functor_map.at("1"), {})}));
    EXPECT_EQ(rule_at(database, rule_id{2})->head, pool->make_functor(functor_map.at("base"), {pool->make_functor(functor_map.at("2"), {})}));
    EXPECT_THROW({ rule_at(database, rule_id{3}); }, std::out_of_range);
}

TEST_F(ImportDatabaseFromFileTest, RulesFixture) {
    db database;
    import_database_from_file("parser/fixtures/rules.chc", *pool, *pool, database, functor_map, next_functor_id);

    EXPECT_THAT(rule_at(database, rule_id{0})->body, SizeIs(2));
    EXPECT_THAT(rule_at(database, rule_id{1})->body, SizeIs(2));

    const expr* x0 = pool->make_var(0);
    const expr* y0 = pool->make_var(1);
    EXPECT_EQ(rule_at(database, rule_id{0})->head, pool->make_functor(functor_map.at("step"), {x0, y0}));
    EXPECT_EQ(rule_at(database, rule_id{0})->body[0], pool->make_functor(functor_map.at("base"), {x0}));
    EXPECT_EQ(rule_at(database, rule_id{0})->body[1], pool->make_functor(functor_map.at("next"), {x0, y0}));
    EXPECT_THROW({ rule_at(database, rule_id{2}); }, std::out_of_range);
}

TEST_F(ImportDatabaseFromFileTest, MixedFixture) {
    db database;
    import_database_from_file("parser/fixtures/mixed.chc", *pool, *pool, database, functor_map, next_functor_id);

    EXPECT_THAT(rule_at(database, rule_id{0})->body, IsEmpty());
    EXPECT_THAT(rule_at(database, rule_id{1})->body, IsEmpty());
    EXPECT_THAT(rule_at(database, rule_id{2})->body, IsEmpty());
    EXPECT_THAT(rule_at(database, rule_id{3})->body, SizeIs(2));
    EXPECT_THAT(rule_at(database, rule_id{4})->body, SizeIs(2));
    EXPECT_THROW({ rule_at(database, rule_id{5}); }, std::out_of_range);
}

TEST_F(ImportDatabaseFromFileTest, BadPathThrows) {
    db database;
    EXPECT_THROW(import_database_from_file("parser/fixtures/nonexistent.chc", *pool, *pool, database, functor_map, next_functor_id),
                 std::runtime_error);
}
