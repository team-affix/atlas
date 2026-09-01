#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include "infrastructure/db.hpp"
#include "infrastructure/db_printer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

using ::testing::NiceMock;

struct MockPrintRule {
    MOCK_METHOD(void, print, (const rule&));
};

using test_db_printer_t = db_printer<NiceMock<MockPrintRule>>;

struct DbPrinterTest : public ::testing::Test {
    NiceMock<MockPrintRule> mock_print_rule;
    std::ostringstream os;
    test_db_printer_t printer{os, mock_print_rule};
    db database;

    expr atom_a{expr::functor{10, {}}};
    expr atom_b{expr::functor{11, {}}};
    expr atom_c{expr::functor{12, {}}};
};

TEST_F(DbPrinterTest, EmptyDatabaseCallsPrintRuleZeroTimes) {
    EXPECT_CALL(mock_print_rule, print).Times(0);
    printer.print(database);
    EXPECT_EQ(os.str(), "");
}

TEST_F(DbPrinterTest, SingleRuleCallsPrintRuleOnceAndEmitsBlankLine) {
    database.push(rule{&atom_a, {}, 0});
    EXPECT_CALL(mock_print_rule, print).Times(1);
    printer.print(database);
    EXPECT_EQ(os.str(), "\n\n");
}

TEST_F(DbPrinterTest, TwoRulesCallsPrintRuleTwiceAndEmitsTwoBlankLines) {
    database.push(rule{&atom_a, {}, 0});
    database.push(rule{&atom_b, {}, 0});
    EXPECT_CALL(mock_print_rule, print).Times(2);
    printer.print(database);
    EXPECT_EQ(os.str(), "\n\n\n\n");
}

TEST_F(DbPrinterTest, ThreeRulesCallsPrintRuleThreeTimes) {
    database.push(rule{&atom_a, {}, 0});
    database.push(rule{&atom_b, {}, 0});
    database.push(rule{&atom_c, {}, 0});
    EXPECT_CALL(mock_print_rule, print).Times(3);
    printer.print(database);
    EXPECT_EQ(os.str(), "\n\n\n\n\n\n");
}
