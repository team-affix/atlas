#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include "infrastructure/rule_printer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

using ::testing::NiceMock;

struct MockPrintExpr {
    MOCK_METHOD(void, print, (const expr*));
};

using test_rule_printer_t = rule_printer<NiceMock<MockPrintExpr>>;

struct RulePrinterTest : public ::testing::Test {
    NiceMock<MockPrintExpr> mock_print_expr;
    std::ostringstream os;
    test_rule_printer_t printer{os, mock_print_expr};

    expr atom_a{expr::functor{10, {}}};
    expr atom_b{expr::functor{11, {}}};
    expr var0{expr::var{0}};
};

TEST_F(RulePrinterTest, FactCallsPrintExprOnHeadThenDot) {
    rule r{&atom_a, {}, 0};
    EXPECT_CALL(mock_print_expr, print(&atom_a));
    printer.print(r);
    EXPECT_EQ(os.str(), ".");
}

TEST_F(RulePrinterTest, RuleWithOneBodyCallsPrintExprTwiceAndFormatsNeck) {
    rule r{&atom_a, {&atom_b}, 0};
    EXPECT_CALL(mock_print_expr, print(&atom_a));
    EXPECT_CALL(mock_print_expr, print(&atom_b));
    printer.print(r);
    EXPECT_EQ(os.str(), " :- .");
}

TEST_F(RulePrinterTest, RuleWithTwoBodyCallsPrintExprThreeTimesAndFormatsComma) {
    rule r{&atom_a, {&atom_b, &var0}, 1};
    EXPECT_CALL(mock_print_expr, print(&atom_a));
    EXPECT_CALL(mock_print_expr, print(&atom_b));
    EXPECT_CALL(mock_print_expr, print(&var0));
    printer.print(r);
    EXPECT_EQ(os.str(), " :- , .");
}

TEST_F(RulePrinterTest, PrintDoesNotAppendNewline) {
    rule r{&atom_a, {}, 0};
    printer.print(r);
    EXPECT_EQ(os.str().back(), '.');
}

TEST_F(RulePrinterTest, ThreeBodyGoalsTwoCommas) {
    expr atom_c{expr::functor{12, {}}};
    rule r{&atom_a, {&atom_b, &var0, &atom_c}, 1};
    EXPECT_CALL(mock_print_expr, print(&atom_a));
    EXPECT_CALL(mock_print_expr, print(&atom_b));
    EXPECT_CALL(mock_print_expr, print(&var0));
    EXPECT_CALL(mock_print_expr, print(&atom_c));
    printer.print(r);
    EXPECT_EQ(os.str(), " :- , , .");
}
