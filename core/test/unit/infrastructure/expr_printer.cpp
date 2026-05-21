#include <gtest/gtest.h>
#include <sstream>
#include "../../../core/hpp/infrastructure/expr_printer.hpp"
#include "../../../core/hpp/infrastructure/var_names.hpp"

static std::string print_expr(const i_var_names& names, const expr* e) {
    std::ostringstream os;
    expr_printer printer(os, names);
    printer.print(e);
    return os.str();
}

class ExprPrinterTest : public ::testing::Test {
protected:
    var_names names;
    expr var0{expr::var{0}};
    expr atom_a{expr::functor{"a", {}}};
    expr atom_b{expr::functor{"b", {}}};
    expr nil{expr::functor{"nil", {}}};
};

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(ExprPrinterTest, PrintUnnamedVar) {
    EXPECT_EQ(print_expr(names, &var0), "?0");
}

TEST_F(ExprPrinterTest, PrintNamedVar) {
    names.set_name(0, "X");
    EXPECT_EQ(print_expr(names, &var0), "X");
}

// ---------------------------------------------------------------------------
// Atoms and nil
// ---------------------------------------------------------------------------

TEST_F(ExprPrinterTest, PrintNullaryAtom) {
    expr f{expr::functor{"f", {}}};
    EXPECT_EQ(print_expr(names, &f), "f");
}

TEST_F(ExprPrinterTest, PrintNil) {
    EXPECT_EQ(print_expr(names, &nil), "[]");
}

// ---------------------------------------------------------------------------
// Lists
// ---------------------------------------------------------------------------

TEST_F(ExprPrinterTest, PrintConsListSingleElement) {
    expr list{expr::functor{"cons", {&atom_a, &nil}}};
    EXPECT_EQ(print_expr(names, &list), "[a]");
}

TEST_F(ExprPrinterTest, PrintConsListTwoElements) {
    expr tail{expr::functor{"cons", {&atom_b, &nil}}};
    expr list{expr::functor{"cons", {&atom_a, &tail}}};
    EXPECT_EQ(print_expr(names, &list), "[a, b]");
}

TEST_F(ExprPrinterTest, PrintPartialList) {
    expr list{expr::functor{"cons", {&atom_a, &var0}}};
    EXPECT_EQ(print_expr(names, &list), "[a|?0]");
}

// ---------------------------------------------------------------------------
// General functors
// ---------------------------------------------------------------------------

TEST_F(ExprPrinterTest, PrintGeneralFunctor) {
    expr f{expr::functor{"f", {&atom_a, &atom_b}}};
    EXPECT_EQ(print_expr(names, &f), "f(a, b)");
}
